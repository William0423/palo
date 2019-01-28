// Copyright (c) 2017, Baidu.com, Inc. All Rights Reserved

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#ifndef BDG_PALO_BE_SRC_QUERY_EXEC_OLAP_SCANNER_H
#define BDG_PALO_BE_SRC_QUERY_EXEC_OLAP_SCANNER_H

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <list>
#include <vector>
#include <string>
#include <utility>

#include "common/status.h"
#include "exec/olap_common.h"
#include "exprs/expr.h"
#include "gen_cpp/PaloInternalService_types.h"
#include "gen_cpp/PlanNodes_types.h"
#include "runtime/descriptors.h"
#include "runtime/tuple.h"
#include "runtime/vectorized_row_batch.h"
#include "olap/olap_reader.h"

namespace palo {

class OlapScanNode;
class OLAPReader;
class RuntimeProfile;

/**
 * @brief   ����engine_reader��ȡolap����
 *          ֧�ֶ�ȡ���scan_range
 *          �����Զ��ڸ������л�
 */
class OlapScanner {
public:
    /**
     * @brief   ��ʼ������.
     *
     * @param   scan_range      ɨ�跶Χ
     */
    OlapScanner(
        RuntimeState* runtime_state,
        const boost::shared_ptr<PaloScanRange> scan_range,
        const std::vector<OlapScanRange>& key_ranges,
        const std::vector<TCondition>& olap_filter,
        const TupleDescriptor& tuple_desc,
        RuntimeProfile* profile,
        const std::vector<TCondition> is_null_vector);

    virtual ~OlapScanner();

    Status open();

    Status get_next(Tuple* tuple, int64_t* raw_rows_read, bool* eof);

    Status close(RuntimeState* state);

    RuntimeState* runtime_state() {
        return _runtime_state;
    }

    std::vector<ExprContext*>* row_conjunct_ctxs() {
        return &_row_conjunct_ctxs;
    }

    std::vector<ExprContext*>* vec_conjunct_ctxs() {
        return &_vec_conjunct_ctxs;
    }

    void set_aggregation(bool aggregation) {
        _aggregation = aggregation;
    }

    void set_id(int id) {
        _id = id;
    }
    int id() {
        return _id;
    }

    bool is_open();
    void set_opened();


    uint64_t scan_row_total_count(){
        return _reader->scan_row_total_count();
    }

    uint64_t scan_row_current_index() {
        return _reader->scan_row_current_index();

    }

private:
    RuntimeState* _runtime_state;
    const TupleDescriptor& _tuple_desc;      /**< tuple descripter */

    const boost::shared_ptr<PaloScanRange> _scan_range;     /**< ����Ĳ�����Ϣ */
    const std::vector<OlapScanRange> _key_ranges;
    const std::vector<TCondition> _olap_filter;
    RuntimeProfile* _profile;

    std::vector<ExprContext*> _row_conjunct_ctxs;
    std::vector<ExprContext*> _vec_conjunct_ctxs;

    std::shared_ptr<OLAPReader> _reader;

    bool _aggregation;
    int _id;
    bool _is_open;
    std::vector<TCondition> _is_null_vector;
};

} // namespace palo

#endif
