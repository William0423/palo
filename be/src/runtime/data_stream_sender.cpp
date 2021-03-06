// Modifications copyright (C) 2017, Baidu.com, Inc.
// Copyright 2017 The Apache Software Foundation

// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include "runtime/data_stream_sender.h"

#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <thrift/protocol/TDebugProtocol.h>

#include "common/logging.h"
#include "exprs/expr.h"
#include "runtime/descriptors.h"
#include "runtime/tuple_row.h"
#include "runtime/row_batch.h"
#include "runtime/raw_value.h"
#include "runtime/runtime_state.h"
#include "runtime/client_cache.h"
#include "runtime/dpp_sink_internal.h"
#include "runtime/mem_tracker.h"
#include "util/debug_util.h"
#include "util/network_util.h"
#include "util/thrift_client.h"
#include "util/thrift_util.h"

#include "gen_cpp/Types_types.h"
#include "gen_cpp/PaloInternalService_types.h"
#include "gen_cpp/BackendService.h"

#include "rpc/dispatch_handler_synchronizer.h"
#include "rpc/event.h"
#include "rpc/protocol.h"
#include "rpc/error.h"
#include "rpc/serialization.h"
#include <arpa/inet.h>

#include "util/thrift_util.h"

namespace palo {

// A channel sends data asynchronously via calls to transmit_data
// to a single destination ipaddress/node.
// It has a fixed-capacity buffer and allows the caller either to add rows to
// that buffer individually (AddRow()), or circumvent the buffer altogether and send
// TRowBatches directly (SendBatch()). Either way, there can only be one in-flight RPC
// at any one time (ie, sending will block if the most recent rpc hasn't finished,
// which allows the receiver node to throttle the sender by withholding acks).
// *Not* thread-safe.
class DataStreamSender::Channel : public DispatchHandler {
public:
    // Create channel to send data to particular ipaddress/port/query/node
    // combination. buffer_size is specified in bytes and a soft limit on
    // how much tuple data is getting accumulated before being sent; it only applies
    // when data is added via add_row() and not sent directly via send_batch().
    Channel(DataStreamSender* parent, const RowDescriptor& row_desc,
            const TNetworkAddress& destination, const TUniqueId& fragment_instance_id,
            PlanNodeId dest_node_id, int buffer_size) :
        _parent(parent),
        _buffer_size(buffer_size),
        _row_desc(row_desc),
        _fragment_instance_id(fragment_instance_id),
        _dest_node_id(dest_node_id),
        _num_data_bytes_sent(0),
        _packet_seq(0),
        _rpc_in_flight(false),
        _is_closed(false),
        _thrift_serializer(false, 1024) {

        _comm = Comm::instance();

        // Initialize InetAddr
        struct sockaddr_in sockaddr_in;
        InetAddr::initialize(&sockaddr_in, destination.hostname.c_str(), destination.port);
        _addr.set_inet(sockaddr_in);
    }

    virtual ~Channel() { }

    // Initialize channel.
    // Returns OK if successful, error indication otherwise.
    Status init(RuntimeState* state);

    // Copies a single row into this channel's output buffer and flushes buffer
    // if it reaches capacity.
    // Returns error status if any of the preceding rpcs failed, OK otherwise.
    Status add_row(TupleRow* row);

    // Asynchronously sends a row batch.
    // Returns the status of the most recently finished transmit_data
    // rpc (or OK if there wasn't one that hasn't been reported yet).
    // if batch is nullptr, send the eof packet
    Status send_batch(TRowBatch* batch);

    // Flush buffered rows and close channel.
    // Returns error status if any of the preceding rpcs failed, OK otherwise.
    void close(RuntimeState* state);

    // Called when event has happened
    void on_event(EventPtr& event);

    int64_t num_data_bytes_sent() const {
        return _num_data_bytes_sent;
    }

    TRowBatch* thrift_batch() { 
        return &_thrift_batch;
    }

    // DispatchHandler handle, used to handle request event
    void handle(EventPtr &event_ptr) override;

private:
    // finish last send, this function may retry last sent if there is error when wait
    // for response
    Status _finish_last_sent();
    // Serialize _batch into _thrift_batch and send via send_batch().
    // Returns send_batch() status.
    Status send_current_batch();
    Status close_internal();
    // send message to remote, this function will reopen connect in ConnectionManager
    Status _send_message();

    DataStreamSender* _parent;
    int _buffer_size;

    const RowDescriptor& _row_desc;
    TUniqueId _fragment_instance_id;
    PlanNodeId _dest_node_id;

    // the number of TRowBatch.data bytes sent successfully
    int64_t _num_data_bytes_sent;
    int64_t _packet_seq;

    // we're accumulating rows into this batch
    boost::scoped_ptr<RowBatch> _batch;
    TRowBatch _thrift_batch;

    // We want to reuse the rpc thread to prevent creating a thread per rowbatch.
    // TODO: currently we only have one batch in flight, but we should buffer more
    // batches. This is a bit tricky since the channels share the outgoing batch
    // pointer we need some mechanism to coordinate when the batch is all done.
    // TODO: if the order of row batches does not matter, we can consider increasing
    // the number of threads.
    bool _rpc_in_flight;  // true if the rpc in sending.

    Status _rpc_status;  // status of most recently finished transmit_data rpc

    bool _is_closed;
    int _be_number;

    CommAddress _addr;
    CommBufPtr _cbp;
    Comm* _comm;
    ConnectionManagerPtr _conn_mgr;

    ThriftSerializer _thrift_serializer;

    // lock, protect variables
    std::mutex _lock;
    std::condition_variable _cond;
    std::deque<EventPtr> _events;

    uint8_t* _serialized_buf = nullptr;
    uint32_t _serialized_buf_bytes = 0;

    uint32_t _connect_timeout_ms = 500;
    uint32_t _rpc_timeout_ms = 1000;
};

Status DataStreamSender::Channel::init(RuntimeState* state) {
    _be_number = state->be_number();

    // TODO: figure out how to size _batch
    int capacity = std::max(1, _buffer_size / std::max(_row_desc.get_row_size(), 1));
    _batch.reset(new RowBatch(_row_desc, capacity, _parent->_mem_tracker.get()));

    _conn_mgr = state->exec_env()->get_conn_manager();
    _conn_mgr->add(_addr, _connect_timeout_ms, NULL);
    // One hour is max rpc timeout
    _rpc_timeout_ms = std::min(3600, std::max(1, state->query_options().query_timeout / 2)) * 1000;
    return Status::OK;
}

Status DataStreamSender::Channel::send_batch(TRowBatch* batch) {
    VLOG_ROW << "Channel::send_batch() instance_id=" << _fragment_instance_id
             << " dest_node=" << _dest_node_id;

    RETURN_IF_ERROR(_finish_last_sent());

    TTransmitDataParams params;
    params.protocol_version = PaloInternalServiceVersion::V1;
    params.__set_dest_fragment_instance_id(_fragment_instance_id);
    params.__set_dest_node_id(_dest_node_id);
    params.__set_be_number(_be_number);
    params.__set_sender_id(_parent->_sender_id);

    if (batch != nullptr) {
        batch->be_number = _be_number;
        batch->packet_seq = _packet_seq++;
        params.__set_row_batch(*batch);  // yet another copy
        params.__set_packet_seq(batch->packet_seq);
        params.__set_eos(false);
    } else {
        params.__set_packet_seq(_packet_seq++);
        params.__set_eos(true);
    }

    _thrift_serializer.serialize(&params, &_serialized_buf_bytes, &_serialized_buf);

    return _send_message();
}

void DataStreamSender::Channel::handle(EventPtr& event) {
    {
        std::lock_guard<std::mutex> l(_lock);
        _events.push_back(event);
    }
    _cond.notify_one();
}

Status DataStreamSender::Channel::_finish_last_sent() {
    if (!_rpc_in_flight) {
        return _rpc_status;
    }
    int retry_times = 1;
    while (true) {
        EventPtr event;
        {
            std::unique_lock<std::mutex> l(_lock);
            auto duration = std::chrono::milliseconds(2 * _rpc_timeout_ms);
            if (_cond.wait_for(l, duration, [this]() { return !this->_events.empty(); })) {
                event = _events.front();
                _events.pop_front();
            }
        }
        if (event == nullptr) {
            LOG(WARNING) << "it's so weird, wait reponse event timeout, request="
                << _cbp->header.id << ", addr=" << _addr.to_str();
            _rpc_in_flight = false;
            if (retry_times-- > 0) {
                // timeout to receive response
                RETURN_IF_ERROR(_send_message());
            } else {
                LOG(WARNING) << "fail to send batch, _add=" << _addr.to_str()
                    << ", request_id="<< _cbp->header.id;
                _rpc_status = Status(TStatusCode::THRIFT_RPC_ERROR, "fail to send batch");
                break;
            }
            continue;
        }
        if (event->type == Event::MESSAGE) {
            if (event->header.id != _cbp->header.id) {
                LOG(WARNING) << "receive event id not equal with in-flight request, request_id="
                    << _cbp->header.id << ", event=" << event->to_str();
                continue;
            }
            // response recept
            _rpc_in_flight = false;
            return Status::OK;
        } else if (event->type == Event::DISCONNECT || event->type == Event::ERROR) {
            if (event->header.id != 0 && event->header.id != _cbp->header.id) {
                LOG(WARNING) << "receive event id not equal with in-flight request, request_id="
                    << _cbp->header.id << ", event=" << event->to_str();
                continue;
            }
            LOG(WARNING) << "receive response failed, request_id=" << _cbp->header.id
                << ", event=" << event->to_str();
            _rpc_in_flight = false;
            // error happend when receving response, we need to retry last request
            if (retry_times-- > 0) {
                // timeout to receive response
                RETURN_IF_ERROR(_send_message());
            } else {
                LOG(WARNING) << "fail to send batch, request_id="<< _cbp->header.id
                    << ", event=" << event->to_str();
                _rpc_status = Status(TStatusCode::THRIFT_RPC_ERROR, "fail to send batch");
                break;
            }
        } else {
            _rpc_in_flight = false;
            LOG(ERROR) << "recevie unexpect event, event=" << event->to_str();
            _rpc_status = Status(TStatusCode::THRIFT_RPC_ERROR, "fail to send batch");
            break;
        }
    }

    return _rpc_status;
}

Status DataStreamSender::Channel::_send_message() {
    DCHECK(!_rpc_in_flight);

    CommHeader header;
    CommBufPtr new_comm_buf = std::make_shared<CommBuf>(header, _serialized_buf_bytes);
    new_comm_buf->append_bytes(_serialized_buf, _serialized_buf_bytes);

    auto res = _comm->send_request(_addr, _rpc_timeout_ms, new_comm_buf, this);
    if (res != error::OK) {
        LOG(WARNING) << "fail to send_request, addr=" << _addr.to_str()
            << ", res=" << res << ", message=" << error::get_text(res);
        // sleep 10ms to wait ConnectionManager to be notify
        usleep(10 * 1000);
        _conn_mgr->add(_addr, _connect_timeout_ms, "PaloBeDataStreamMgr");
        bool is_connected = _conn_mgr->wait_for_connection(_addr, _connect_timeout_ms);
        if (!is_connected) {
            LOG(WARNING) << "fail to wait_for_connection, addr=" << _addr.to_str();
            _conn_mgr->remove(_addr);
            _rpc_status = Status(TStatusCode::THRIFT_RPC_ERROR, "connection to remote PaloBe failed");
            return _rpc_status;
        }
        res = _comm->send_request(_addr, _rpc_timeout_ms, new_comm_buf, this);
        if (res != error::OK) {
            LOG(WARNING) << "fail to send_request, addr=" << _addr.to_str()
                << ", res=" << res << ", message=" << error::get_text(res);
            _rpc_status = Status(TStatusCode::THRIFT_RPC_ERROR, "fail to send_request");
            return _rpc_status;
        }
    }
    _cbp = new_comm_buf;
    _rpc_in_flight = true;
    return Status::OK;
}

Status DataStreamSender::Channel::add_row(TupleRow* row) {
    int row_num = _batch->add_row();

    if (row_num == RowBatch::INVALID_ROW_INDEX) {
        // _batch is full, let's send it; but first wait for an ongoing
        // transmission to finish before modifying _thrift_batch
        RETURN_IF_ERROR(send_current_batch());
        row_num = _batch->add_row();
        DCHECK_NE(row_num, RowBatch::INVALID_ROW_INDEX);
    }

    TupleRow* dest = _batch->get_row(row_num);
    _batch->copy_row(row, dest);
    const std::vector<TupleDescriptor*>& descs = _row_desc.tuple_descriptors();

    for (int i = 0; i < descs.size(); ++i) {
        if (UNLIKELY(row->get_tuple(i) == NULL)) {
            dest->set_tuple(i, NULL);
        } else {
            dest->set_tuple(i, row->get_tuple(i)->deep_copy(*descs[i],
                           _batch->tuple_data_pool()));
        }
    }

    _batch->commit_last_row();
    return Status::OK;
}

Status DataStreamSender::Channel::send_current_batch() {
    {
        SCOPED_TIMER(_parent->_serialize_batch_timer);
        int uncompressed_bytes = _batch->serialize(&_thrift_batch);
        COUNTER_UPDATE(_parent->_bytes_sent_counter, RowBatch::get_batch_size(_thrift_batch));
        COUNTER_UPDATE(_parent->_uncompressed_bytes_counter, uncompressed_bytes);
    }
    _batch->reset();
    RETURN_IF_ERROR(send_batch(&_thrift_batch));
    return Status::OK;
}

Status DataStreamSender::Channel::close_internal() {
    if (_is_closed) {
        return Status::OK;
    }
    VLOG_RPC << "Channel::close() instance_id=" << _fragment_instance_id
             << " dest_node=" << _dest_node_id
             << " #rows= " << _batch->num_rows();
    if (_batch != NULL && _batch->num_rows() > 0) {
        RETURN_IF_ERROR(send_current_batch());
    }

    RETURN_IF_ERROR(send_batch(nullptr));
    RETURN_IF_ERROR(_finish_last_sent());
    _is_closed = true;
    return Status::OK;
}

void DataStreamSender::Channel::close(RuntimeState* state) {
    state->log_error(close_internal().get_error_msg());
    _batch.reset();
}

DataStreamSender::DataStreamSender(
            ObjectPool* pool, int sender_id,
            const RowDescriptor& row_desc, const TDataStreamSink& sink,
            const std::vector<TPlanFragmentDestination>& destinations,
            int per_channel_buffer_size) :
        _sender_id(sender_id),
        _pool(pool),
        _row_desc(row_desc),
        _current_channel_idx(0),
        _part_type(sink.output_partition.type),
        _ignore_not_found(sink.__isset.ignore_not_found ? sink.ignore_not_found : true),
        _current_thrift_batch(&_thrift_batch1),
        _profile(NULL),
        _serialize_batch_timer(NULL),
        _thrift_transmit_timer(NULL),
        _bytes_sent_counter(NULL),
        _dest_node_id(sink.dest_node_id) {
    DCHECK_GT(destinations.size(), 0);
    DCHECK(sink.output_partition.type == TPartitionType::UNPARTITIONED
            || sink.output_partition.type == TPartitionType::HASH_PARTITIONED
            || sink.output_partition.type == TPartitionType::RANDOM
            || sink.output_partition.type == TPartitionType::RANGE_PARTITIONED);
    // TODO: use something like google3's linked_ptr here (scoped_ptr isn't copyable)
    for (int i = 0; i < destinations.size(); ++i) {
        _channel_shared_ptrs.emplace_back(
            new Channel(this, row_desc, destinations[i].server,
                        destinations[i].fragment_instance_id,
                        sink.dest_node_id, per_channel_buffer_size));
        _channels.push_back(_channel_shared_ptrs[i].get());
    }
}

// We use the ParttitionRange to compare here. It should not be a member function of PartitionInfo
// class becaurce there are some other member in it.
static bool compare_part_use_range(const PartitionInfo* v1, const PartitionInfo* v2) {
    return v1->range() < v2->range();
}

Status DataStreamSender::init(const TDataSink& tsink) {
    RETURN_IF_ERROR(DataSink::init(tsink));
    const TDataStreamSink& t_stream_sink = tsink.stream_sink;
    if (_part_type == TPartitionType::HASH_PARTITIONED) {
        RETURN_IF_ERROR(Expr::create_expr_trees(
                _pool, t_stream_sink.output_partition.partition_exprs, &_partition_expr_ctxs));
    } else if (_part_type == TPartitionType::RANGE_PARTITIONED) {
        // Range partition
        // Partition Exprs
        RETURN_IF_ERROR(Expr::create_expr_trees(
                _pool, t_stream_sink.output_partition.partition_exprs, &_partition_expr_ctxs));
        // Partition infos
        int num_parts = t_stream_sink.output_partition.partition_infos.size();
        if (num_parts == 0) {
            return Status("Empty partition info.");
        }
        for (int i = 0; i < num_parts; ++i) {
            PartitionInfo* info = _pool->add(new PartitionInfo());
            RETURN_IF_ERROR(PartitionInfo::from_thrift(
                    _pool, t_stream_sink.output_partition.partition_infos[i], info));
            _partition_infos.push_back(info);
        }
        // partitions should be in ascending order
        std::sort(_partition_infos.begin(), _partition_infos.end(), compare_part_use_range);
    } else {
    }

    return Status::OK;
}

Status DataStreamSender::prepare(RuntimeState* state) {
    RETURN_IF_ERROR(DataSink::prepare(state));
    _state = state;
    std::stringstream title;
    title << "DataStreamSender (dst_id=" << _dest_node_id << ")";
    _profile = _pool->add(new RuntimeProfile(_pool, title.str()));
    SCOPED_TIMER(_profile->total_time_counter());
    _mem_tracker.reset(
            new MemTracker(-1, "DataStreamSender", state->instance_mem_tracker()));

    if (_part_type == TPartitionType::UNPARTITIONED 
            || _part_type == TPartitionType::RANDOM) {
        // Randomize the order we open/transmit to channels to avoid thundering herd problems.
        srand(reinterpret_cast<uint64_t>(this));
        random_shuffle(_channels.begin(), _channels.end());
    } else if (_part_type == TPartitionType::HASH_PARTITIONED) {
        RETURN_IF_ERROR(Expr::prepare(
                _partition_expr_ctxs, state, _row_desc, _expr_mem_tracker.get()));
    } else {
        RETURN_IF_ERROR(Expr::prepare(
                _partition_expr_ctxs, state, _row_desc, _expr_mem_tracker.get()));
        for (auto iter : _partition_infos) {
            RETURN_IF_ERROR(iter->prepare(state, _row_desc, _expr_mem_tracker.get()));
        }
    }

    _bytes_sent_counter =
        ADD_COUNTER(profile(), "BytesSent", TUnit::BYTES);
    _uncompressed_bytes_counter =
        ADD_COUNTER(profile(), "UncompressedRowBatchSize", TUnit::BYTES);
    _ignore_rows =
        ADD_COUNTER(profile(), "IgnoreRows", TUnit::UNIT);
    _serialize_batch_timer =
        ADD_TIMER(profile(), "SerializeBatchTime");
    _thrift_transmit_timer = ADD_TIMER(profile(), "ThriftTransmitTime(*)");
    _network_throughput =
        profile()->add_derived_counter("NetworkThroughput(*)", TUnit::BYTES_PER_SECOND,
                boost::bind<int64_t>(&RuntimeProfile::units_per_second, _bytes_sent_counter,
                _thrift_transmit_timer), "");
    _overall_throughput =
        profile()->add_derived_counter("OverallThroughput", TUnit::BYTES_PER_SECOND,
        boost::bind<int64_t>(&RuntimeProfile::units_per_second, _bytes_sent_counter,
                                             profile()->total_time_counter()), "");

    for (int i = 0; i < _channels.size(); ++i) {
        RETURN_IF_ERROR(_channels[i]->init(state));
    }

    return Status::OK;
}

DataStreamSender::~DataStreamSender() {
    // TODO: check that sender was either already closed() or there was an error
    // on some channel
    _channel_shared_ptrs.clear();
}

Status DataStreamSender::open(RuntimeState* state) {
    DCHECK(state != NULL);
    RETURN_IF_ERROR(Expr::open(_partition_expr_ctxs, state));
    for (auto iter : _partition_infos) {
        RETURN_IF_ERROR(iter->open(state));
    }
    return Status::OK;
}

Status DataStreamSender::send(RuntimeState* state, RowBatch* batch) {
    SCOPED_TIMER(_profile->total_time_counter());

    // Unpartition or _channel size
    if (_part_type == TPartitionType::UNPARTITIONED || _channels.size() == 1) {
        // _current_thrift_batch is *not* the one that was written by the last call
        // to Serialize()
        RETURN_IF_ERROR(serialize_batch(batch, _current_thrift_batch, _channels.size()));
        // SendBatch() will block if there are still in-flight rpcs (and those will
        // reference the previously written thrift batch)
        for (int i = 0; i < _channels.size(); ++i) {
            RETURN_IF_ERROR(_channels[i]->send_batch(_current_thrift_batch));
        }
        _current_thrift_batch =
            (_current_thrift_batch == &_thrift_batch1 ? &_thrift_batch2 : &_thrift_batch1);
    } else if (_part_type == TPartitionType::RANDOM) {
        // Round-robin batches among channels. Wait for the current channel to finish its
        // rpc before overwriting its batch.
        Channel* current_channel = _channels[_current_channel_idx];
        RETURN_IF_ERROR(serialize_batch(batch, current_channel->thrift_batch()));
        RETURN_IF_ERROR(current_channel->send_batch(current_channel->thrift_batch()));
        _current_channel_idx = (_current_channel_idx + 1) % _channels.size();
    } else if (_part_type == TPartitionType::HASH_PARTITIONED) {
        // hash-partition batch's rows across channels
        int num_channels = _channels.size();

        for (int i = 0; i < batch->num_rows(); ++i) {
            TupleRow* row = batch->get_row(i);
            size_t hash_val = 0;

            for (auto ctx : _partition_expr_ctxs) {
                void* partition_val = ctx->get_value(row);
                // We can't use the crc hash function here because it does not result
                // in uncorrelated hashes with different seeds.  Instead we must use
                // fvn hash.
                // TODO: fix crc hash/GetHashValue()
                hash_val = RawValue::get_hash_value_fvn(
                    partition_val, ctx->root()->type(), hash_val);
            }
            RETURN_IF_ERROR(_channels[hash_val % num_channels]->add_row(row));
        }
    } else {
        // Range partition
        int num_channels = _channels.size();
        int ignore_rows = 0;
        for (int i = 0; i < batch->num_rows(); ++i) {
            TupleRow* row = batch->get_row(i);
            size_t hash_val = 0;
            bool ignore = false;
            RETURN_IF_ERROR(compute_range_part_code(state, row, &hash_val, &ignore));
            if (ignore) {
                // skip this row
                ignore_rows++;
                continue;
            }
            RETURN_IF_ERROR(_channels[hash_val % num_channels]->add_row(row));
        }
        COUNTER_UPDATE(_ignore_rows, ignore_rows);
    }

    return Status::OK;
}

int DataStreamSender::binary_find_partition(const PartRangeKey& key) const {
    int low = 0;
    int high = _partition_infos.size() - 1;

    VLOG_ROW << "range key: " << key.debug_string() << std::endl;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        int cmp = _partition_infos[mid]->range().compare_key(key);
        if (cmp == 0) {
            return mid;
        } else if (cmp < 0) { // current < partition[mid]
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    return -1;
}

Status DataStreamSender::find_partition(
        RuntimeState* state, TupleRow* row, PartitionInfo** info, bool* ignore) {
    if (_partition_expr_ctxs.size() == 0) {
        *info = _partition_infos[0];
        return Status::OK;
    } else {
        *ignore = false;
        // use binary search to get the right partition.
        ExprContext* ctx = _partition_expr_ctxs[0];
        void* partition_val = ctx->get_value(row);
        // construct a PartRangeKey
        PartRangeKey tmpPartKey;
        if (NULL != partition_val) {
            RETURN_IF_ERROR(PartRangeKey::from_value(
                ctx->root()->type().type, partition_val, &tmpPartKey));
        } else {
            tmpPartKey = PartRangeKey::neg_infinite();
        }

        int part_index = binary_find_partition(tmpPartKey);
        if (part_index < 0) {
            if (_ignore_not_found) {
                // TODO(zc): add counter to compute its
                std::stringstream error_log;
                error_log << "there is no corresponding partition for this key: ";
                ctx->print_value(row, &error_log);
                LOG(INFO) << error_log.str();
                *ignore = true;
                return Status::OK;
            } else {
                std::stringstream error_log;
                error_log << "there is no corresponding partition for this key: ";
                ctx->print_value(row, &error_log);
                return Status(error_log.str());
            }
        }
        *info = _partition_infos[part_index];
    }
    return Status::OK;
}

Status DataStreamSender::process_distribute(
        RuntimeState* state, TupleRow* row,
        const PartitionInfo* part, size_t* code) {
    uint32_t hash_val = 0;
    for (auto& ctx: part->distributed_expr_ctxs()) {
        void* partition_val = ctx->get_value(row);
        if (partition_val != NULL) {
            hash_val = RawValue::zlib_crc32(partition_val, ctx->root()->type(), hash_val);
        } else {
            //NULL is treat as 0 when hash
            static const int INT_VALUE = 0;
            static const TypeDescriptor INT_TYPE(TYPE_INT);
            hash_val = RawValue::zlib_crc32(&INT_VALUE, INT_TYPE, hash_val);
        }
    }
    hash_val %= part->distributed_bucket();

    int64_t part_id = part->id();
    *code = RawValue::get_hash_value_fvn(&part_id, TypeDescriptor(TYPE_BIGINT), hash_val);

    return Status::OK;
}

Status DataStreamSender::compute_range_part_code(
        RuntimeState* state,
        TupleRow* row,
        size_t* hash_value,
        bool* ignore) {
    // process partition
    PartitionInfo* part = nullptr;
    RETURN_IF_ERROR(find_partition(state, row, &part, ignore));
    if (*ignore) {
        return Status::OK;
    }
    // process distribute
    RETURN_IF_ERROR(process_distribute(state, row, part, hash_value));
    return Status::OK;
}

Status DataStreamSender::close(RuntimeState* state, Status exec_status) {
    // TODO: only close channels that didn't have any errors
    for (int i = 0; i < _channels.size(); ++i) {
        _channels[i]->close(state);
    }
    for (auto iter : _partition_infos) {
        RETURN_IF_ERROR(iter->close(state));
    }
    Expr::close(_partition_expr_ctxs, state);

    return Status::OK;
}

Status DataStreamSender::serialize_batch(RowBatch* src, TRowBatch* dest, int num_receivers) {
    VLOG_ROW << "serializing " << src->num_rows() << " rows";
    {
        // TODO(zc)
        // SCOPED_TIMER(_profile->total_time_counter());
        SCOPED_TIMER(_serialize_batch_timer);
        // TODO(zc)
        // RETURN_IF_ERROR(src->serialize(dest));
        int uncompressed_bytes = src->serialize(dest);
        int bytes = RowBatch::get_batch_size(*dest);
        // TODO(zc)
        // int uncompressed_bytes = bytes - dest->tuple_data.size() + dest->uncompressed_size;
        // The size output_batch would be if we didn't compress tuple_data (will be equal to
        // actual batch size if tuple_data isn't compressed)

        COUNTER_UPDATE(_bytes_sent_counter, bytes * num_receivers);
        COUNTER_UPDATE(_uncompressed_bytes_counter, uncompressed_bytes * num_receivers);
    }

    return Status::OK;
}


int64_t DataStreamSender::get_num_data_bytes_sent() const {
    // TODO: do we need synchronization here or are reads & writes to 8-byte ints
    // atomic?
    int64_t result = 0;

    for (int i = 0; i < _channels.size(); ++i) {
        result += _channels[i]->num_data_bytes_sent();
    }

    return result;
}

}
