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

#include "service/backend_service.h"

#include <boost/shared_ptr.hpp>
#include <gperftools/heap-profiler.h>
#include <thrift/protocol/TDebugProtocol.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <dirent.h>
#include "service/backend_options.h"
#include "util/network_util.h"
#include "util/thrift_util.h"
#include "util/thrift_server.h"
#include "util/debug_util.h"
#include "util/palo_metrics.h"
#include "runtime/fragment_mgr.h"
#include "runtime/data_stream_mgr.h"
#include "runtime/pull_load_task_mgr.h"
#include "runtime/export_task_mgr.h"
#include "runtime/result_buffer_mgr.h"
#include "service/receiver_dispatcher.h"

namespace palo {

using apache::thrift::TException;
using apache::thrift::TProcessor;
using apache::thrift::transport::TTransportException;
using apache::thrift::concurrency::ThreadFactory;
using apache::thrift::concurrency::PosixThreadFactory;


BackendService::BackendService(ExecEnv* exec_env) :
        _exec_env(exec_env),
        _agent_server(new AgentServer(exec_env, *exec_env->master_info())) {
#ifndef ADDRESS_SANITIZER
    // tcmalloc and address sanitizer can not be used together
    if (!config::heap_profile_dir.empty()) {
        HeapProfilerStart(config::heap_profile_dir.c_str());
    }
#endif
    // Initialize Palo metrics
    PaloMetrics::create_metrics(exec_env->metrics());
    char buf[64];
    DateTimeValue value = DateTimeValue::local_time();
    value.to_string(buf);
    PaloMetrics::palo_be_start_time()->update(buf);
}

Status BackendService::create_service(ExecEnv* exec_env, int port, ThriftServer** server) {
    boost::shared_ptr<BackendService> handler(new BackendService(exec_env));

    // TODO: do we want a BoostThreadFactory?
    // TODO: we want separate thread factories here, so that fe requests can't starve
    // be requests
    boost::shared_ptr<ThreadFactory> thread_factory(new PosixThreadFactory());

    boost::shared_ptr<TProcessor> be_processor(new BackendServiceProcessor(handler));
    *server = new ThriftServer("PaloBackend",
                               be_processor, port,
                               exec_env->metrics(),
                               config::be_service_threads);

    LOG(INFO) << "PaloInternalService listening on " << port;

    return Status::OK;
}

Status BackendService::create_rpc_service(ExecEnv* exec_env) {
    ReactorFactory::initialize(config::rpc_reactor_threads);

    struct sockaddr_in addr;
    InetAddr::initialize(&addr, BackendOptions::get_localhost().c_str(), config::be_rpc_port);
    Comm* comm = Comm::instance();

    DispatchHandlerPtr dhp = std::make_shared<Dispatcher>(exec_env, comm, nullptr);
    ConnectionHandlerFactoryPtr handler_factory = std::make_shared<HandlerFactory>(dhp);

    Status status = Status::OK;
    int error = comm->listen(addr, handler_factory, dhp);
    if (error != error::OK) {
        status = Status("create rpc server failed.");
    }

    return status;
}

/**
 *
 * 通过thrift接口，接受从Fe传过来的查询请求。
 *
 *
 * @param return_val
 * @param params
 */

void BackendService::exec_plan_fragment(TExecPlanFragmentResult& return_val,
                                        const TExecPlanFragmentParams& params) {
    /**
     * 此处打印的信息是：
     * I0724 09:05:34.784127  2384 backend_service.cpp:107]
     * exec_plan_fragment() instance_id=TUniqueId(hi=1055662697585069723, lo=-8788081665675827573) coord=TNetworkAddress(hostname=172.17.0.3, port=9020) backend#=0
     *
     */
    LOG(INFO) << "exec_plan_fragment() instance_id=" << params.params.fragment_instance_id
        << " coord=" << params.coord << " backend#=" << params.backend_num;

    VLOG_ROW << "exec_plan_fragment params is "
            << apache::thrift::ThriftDebugString(params).c_str();

    start_plan_fragment_execution(params).set_t_status(&return_val);


}

/**
 *
 * @param exec_params 内存地址作为参数传递：
 * @return
 */
Status BackendService::start_plan_fragment_execution(const TExecPlanFragmentParams& exec_params) {
    if (!exec_params.fragment.__isset.output_sink) {      //jungle comment : output_sink must be set ,then plan get next and send data in PlanFragmentExecutor::open()
        return Status("missing sink in plan fragment");
    }
    return _exec_env->fragment_mgr()->exec_plan_fragment(exec_params);
}



void BackendService::cancel_plan_fragment(TCancelPlanFragmentResult& return_val,
                                          const TCancelPlanFragmentParams& params) {
    LOG(INFO) << "cancel_plan_fragment(): instance_id=" << params.fragment_instance_id;
    _exec_env->fragment_mgr()->cancel(params.fragment_instance_id).set_t_status(&return_val);
}

void BackendService::transmit_data(TTransmitDataResult& return_val,
                                   const TTransmitDataParams& params) {
    VLOG_ROW << "transmit_data(): instance_id=" << params.dest_fragment_instance_id
             << " node_id=" << params.dest_node_id
             << " #rows=" << params.row_batch.num_rows
             << " eos=" << (params.eos ? "true" : "false");
    // VLOG_ROW << "transmit_data params: " << apache::thrift::ThriftDebugString(params).c_str();

    if (params.__isset.packet_seq) {
        return_val.__set_packet_seq(params.packet_seq);
        return_val.__set_dest_fragment_instance_id(params.dest_fragment_instance_id);
        return_val.__set_dest_node_id(params.dest_node_id);
    }

    // TODO: fix Thrift so we can simply take ownership of thrift_batch instead
    // of having to copy its data
    if (params.row_batch.num_rows > 0) {
        // Status status = _exec_env->stream_mgr()->add_data(
        //         params.dest_fragment_instance_id,
        //         params.dest_node_id,
        //         params.row_batch,
        //         params.sender_id);
        // status.set_t_status(&return_val);

        // if (!status.ok()) {
        //     // should we close the channel here as well?
        //     return;
        // }
    }

    if (params.eos) {
        Status status = _exec_env->stream_mgr()->close_sender(
                params.dest_fragment_instance_id,
                params.dest_node_id,
                params.sender_id,
                params.be_number);
        VLOG_ROW << "params.eos: " << (params.eos ? "true" : "false")
                << " close_sender status: " << status.get_error_msg();
        status.set_t_status(&return_val);
    }
}

void BackendService::fetch_data(TFetchDataResult& return_val,
                                const TFetchDataParams& params) {
    // maybe hang in this function
    Status status = _exec_env->result_mgr()->fetch_data(params.fragment_instance_id, &return_val);
    status.set_t_status(&return_val);
}

void BackendService::register_pull_load_task(
        TStatus& t_status, const TUniqueId& id, int num_senders) {
    Status status = _exec_env->pull_load_task_mgr()->register_task(id, num_senders);
    status.to_thrift(&t_status);
}

void BackendService::deregister_pull_load_task(TStatus& t_status, const TUniqueId& id) {
    Status status = _exec_env->pull_load_task_mgr()->deregister_task(id);
    status.to_thrift(&t_status);
}

void BackendService::report_pull_load_sub_task_info(
        TStatus& t_status, const TPullLoadSubTaskInfo& task_info) {
    Status status = _exec_env->pull_load_task_mgr()->report_sub_task_info(task_info);
    status.to_thrift(&t_status);
}

void BackendService::fetch_pull_load_task_info(
        TFetchPullLoadTaskInfoResult& result, const TUniqueId& id) {
    Status status = _exec_env->pull_load_task_mgr()->fetch_task_info(id, &result);
    status.to_thrift(&result.status);
}

void BackendService::fetch_all_pull_load_task_infos(
        TFetchAllPullLoadTaskInfosResult& result) {
    Status status = _exec_env->pull_load_task_mgr()->fetch_all_task_infos(&result);
    status.to_thrift(&result.status);
}

void BackendService::submit_export_task(TStatus& t_status, const TExportTaskRequest& request) {
//    VLOG_ROW << "submit_export_task. request  is "
//            << apache::thrift::ThriftDebugString(request).c_str();
//
//    Status status = _exec_env->export_task_mgr()->start_task(request);
//    if (status.ok()) {
//        VLOG_RPC << "start export task successfull id="
//            << request.params.params.fragment_instance_id;
//    } else {
//        VLOG_RPC << "start export task failed id="
//            << request.params.params.fragment_instance_id
//            << " and err_msg=" << status.get_error_msg();
//    }
//    status.to_thrift(&t_status);
}

void BackendService::get_export_status(TExportStatusResult& result, const TUniqueId& task_id) {
//    VLOG_ROW << "get_export_status. task_id  is " << task_id;
//    Status status = _exec_env->export_task_mgr()->get_task_state(task_id, &result);
//    if (!status.ok()) {
//        LOG(WARNING) << "get export task state failed. [id=" << task_id << "]";
//    } else {
//        VLOG_RPC << "get export task state successful. [id=" << task_id
//            << ",status=" << result.status.status_code
//            << ",state=" << result.state
//            << ",files=";
//        for (auto& item : result.files) {
//            VLOG_RPC << item << ", ";
//        }
//        VLOG_RPC << "]";
//    }
//    status.to_thrift(&result.status);
//    result.__set_state(TExportState::RUNNING);
}

void BackendService::erase_export_task(TStatus& t_status, const TUniqueId& task_id) {
//    VLOG_ROW << "erase_export_task. task_id  is " << task_id;
//    Status status = _exec_env->export_task_mgr()->erase_task(task_id);
//    if (!status.ok()) {
//        LOG(WARNING) << "delete export task failed. because "
//            << status.get_error_msg() << " with task_id " << task_id;
//    } else {
//        VLOG_RPC << "delete export task successful with task_id " << task_id;
//    }
//    status.to_thrift(&t_status);
}

void BackendService::get_streaming_etl_file_path(std::vector<std::string> & result ,const std::string & file_path,const int  cnt){


    LOG(INFO)<<" call get_streaming_etl_file_path : " << file_path;
    std::string file_dir = file_path;
    file_dir = file_dir.append("/");
    DIR *p_dir;
    const char* str = file_dir.c_str();

    p_dir = opendir(str);
    if( p_dir == NULL)
    {
        LOG(INFO)<< "can't open :" << file_dir ;
        return;
    }

    struct dirent *p_dirent;
    while ( p_dirent = readdir(p_dir))
    {
        std::string tmpFileName = p_dirent->d_name;
        LOG(INFO)<<"tmpFileName : " << tmpFileName ;
        if( tmpFileName == "." || tmpFileName == ".." || tmpFileName.find("done")  != std::string::npos)
        {
            continue;
        }
        else
        {

            if(result.size() < cnt ){
                result.push_back(file_dir + tmpFileName);
                LOG(INFO)<<"file path result : " << result[result.size()-1];
            }

        }
    }
    closedir(p_dir);


}

void BackendService::mark_etl_file_done( TStatus& result, const std::vector<std::string> & file_done){
    LOG(INFO)<<"call mark_etl_file_done " ;
    for(std::string  file: file_done){
        if(access(file.c_str(),0) == 0) {
            if (rename(file.c_str(), (file + ".done").c_str()) == 0) {
                LOG(INFO)<<"rename file success"<< file;
                result.status_code = TStatusCode::OK;
            } else{
                LOG(INFO)<<"rename file failed"<< file;
                result.status_code = TStatusCode::RUNTIME_ERROR;
            }
        }
    }


}

} // namespace palo
