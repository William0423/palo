/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef BackendService_TYPES_H
#define BackendService_TYPES_H

#include <iosfwd>

#include <thrift/Thrift.h>
#include <thrift/TApplicationException.h>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TTransport.h>

#include <thrift/cxxfunctional.h>
#include "Status_types.h"
#include "Types_types.h"
#include "AgentService_types.h"
#include "PaloInternalService_types.h"


namespace palo {

class TPullLoadSubTaskInfo;

class TPullLoadTaskInfo;

class TFetchPullLoadTaskInfoResult;

class TFetchAllPullLoadTaskInfosResult;

class TExportTaskRequest;

typedef struct _TPullLoadSubTaskInfo__isset {
  _TPullLoadSubTaskInfo__isset() : tracking_url(false) {}
  bool tracking_url :1;
} _TPullLoadSubTaskInfo__isset;

class TPullLoadSubTaskInfo {
 public:

  TPullLoadSubTaskInfo(const TPullLoadSubTaskInfo&);
  TPullLoadSubTaskInfo& operator=(const TPullLoadSubTaskInfo&);
  TPullLoadSubTaskInfo() : sub_task_id(0), tracking_url() {
  }

  virtual ~TPullLoadSubTaskInfo() throw();
   ::palo::TUniqueId id;
  int32_t sub_task_id;
  std::map<std::string, int64_t>  file_map;
  std::map<std::string, std::string>  counters;
  std::string tracking_url;

  _TPullLoadSubTaskInfo__isset __isset;

  void __set_id(const  ::palo::TUniqueId& val);

  void __set_sub_task_id(const int32_t val);

  void __set_file_map(const std::map<std::string, int64_t> & val);

  void __set_counters(const std::map<std::string, std::string> & val);

  void __set_tracking_url(const std::string& val);

  bool operator == (const TPullLoadSubTaskInfo & rhs) const
  {
    if (!(id == rhs.id))
      return false;
    if (!(sub_task_id == rhs.sub_task_id))
      return false;
    if (!(file_map == rhs.file_map))
      return false;
    if (!(counters == rhs.counters))
      return false;
    if (__isset.tracking_url != rhs.__isset.tracking_url)
      return false;
    else if (__isset.tracking_url && !(tracking_url == rhs.tracking_url))
      return false;
    return true;
  }
  bool operator != (const TPullLoadSubTaskInfo &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const TPullLoadSubTaskInfo & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(TPullLoadSubTaskInfo &a, TPullLoadSubTaskInfo &b);

inline std::ostream& operator<<(std::ostream& out, const TPullLoadSubTaskInfo& obj)
{
  obj.printTo(out);
  return out;
}

typedef struct _TPullLoadTaskInfo__isset {
  _TPullLoadTaskInfo__isset() : file_map(false), counters(false), tracking_urls(false) {}
  bool file_map :1;
  bool counters :1;
  bool tracking_urls :1;
} _TPullLoadTaskInfo__isset;

class TPullLoadTaskInfo {
 public:

  TPullLoadTaskInfo(const TPullLoadTaskInfo&);
  TPullLoadTaskInfo& operator=(const TPullLoadTaskInfo&);
  TPullLoadTaskInfo() : etl_state(( ::palo::TEtlState::type)0) {
  }

  virtual ~TPullLoadTaskInfo() throw();
   ::palo::TUniqueId id;
   ::palo::TEtlState::type etl_state;
  std::map<std::string, int64_t>  file_map;
  std::map<std::string, std::string>  counters;
  std::vector<std::string>  tracking_urls;

  _TPullLoadTaskInfo__isset __isset;

  void __set_id(const  ::palo::TUniqueId& val);

  void __set_etl_state(const  ::palo::TEtlState::type val);

  void __set_file_map(const std::map<std::string, int64_t> & val);

  void __set_counters(const std::map<std::string, std::string> & val);

  void __set_tracking_urls(const std::vector<std::string> & val);

  bool operator == (const TPullLoadTaskInfo & rhs) const
  {
    if (!(id == rhs.id))
      return false;
    if (!(etl_state == rhs.etl_state))
      return false;
    if (__isset.file_map != rhs.__isset.file_map)
      return false;
    else if (__isset.file_map && !(file_map == rhs.file_map))
      return false;
    if (__isset.counters != rhs.__isset.counters)
      return false;
    else if (__isset.counters && !(counters == rhs.counters))
      return false;
    if (__isset.tracking_urls != rhs.__isset.tracking_urls)
      return false;
    else if (__isset.tracking_urls && !(tracking_urls == rhs.tracking_urls))
      return false;
    return true;
  }
  bool operator != (const TPullLoadTaskInfo &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const TPullLoadTaskInfo & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(TPullLoadTaskInfo &a, TPullLoadTaskInfo &b);

inline std::ostream& operator<<(std::ostream& out, const TPullLoadTaskInfo& obj)
{
  obj.printTo(out);
  return out;
}


class TFetchPullLoadTaskInfoResult {
 public:

  TFetchPullLoadTaskInfoResult(const TFetchPullLoadTaskInfoResult&);
  TFetchPullLoadTaskInfoResult& operator=(const TFetchPullLoadTaskInfoResult&);
  TFetchPullLoadTaskInfoResult() {
  }

  virtual ~TFetchPullLoadTaskInfoResult() throw();
   ::palo::TStatus status;
  TPullLoadTaskInfo task_info;

  void __set_status(const  ::palo::TStatus& val);

  void __set_task_info(const TPullLoadTaskInfo& val);

  bool operator == (const TFetchPullLoadTaskInfoResult & rhs) const
  {
    if (!(status == rhs.status))
      return false;
    if (!(task_info == rhs.task_info))
      return false;
    return true;
  }
  bool operator != (const TFetchPullLoadTaskInfoResult &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const TFetchPullLoadTaskInfoResult & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(TFetchPullLoadTaskInfoResult &a, TFetchPullLoadTaskInfoResult &b);

inline std::ostream& operator<<(std::ostream& out, const TFetchPullLoadTaskInfoResult& obj)
{
  obj.printTo(out);
  return out;
}


class TFetchAllPullLoadTaskInfosResult {
 public:

  TFetchAllPullLoadTaskInfosResult(const TFetchAllPullLoadTaskInfosResult&);
  TFetchAllPullLoadTaskInfosResult& operator=(const TFetchAllPullLoadTaskInfosResult&);
  TFetchAllPullLoadTaskInfosResult() {
  }

  virtual ~TFetchAllPullLoadTaskInfosResult() throw();
   ::palo::TStatus status;
  std::vector<TPullLoadTaskInfo>  task_infos;

  void __set_status(const  ::palo::TStatus& val);

  void __set_task_infos(const std::vector<TPullLoadTaskInfo> & val);

  bool operator == (const TFetchAllPullLoadTaskInfosResult & rhs) const
  {
    if (!(status == rhs.status))
      return false;
    if (!(task_infos == rhs.task_infos))
      return false;
    return true;
  }
  bool operator != (const TFetchAllPullLoadTaskInfosResult &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const TFetchAllPullLoadTaskInfosResult & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(TFetchAllPullLoadTaskInfosResult &a, TFetchAllPullLoadTaskInfosResult &b);

inline std::ostream& operator<<(std::ostream& out, const TFetchAllPullLoadTaskInfosResult& obj)
{
  obj.printTo(out);
  return out;
}


class TExportTaskRequest {
 public:

  TExportTaskRequest(const TExportTaskRequest&);
  TExportTaskRequest& operator=(const TExportTaskRequest&);
  TExportTaskRequest() {
  }

  virtual ~TExportTaskRequest() throw();
   ::palo::TExecPlanFragmentParams params;

  void __set_params(const  ::palo::TExecPlanFragmentParams& val);

  bool operator == (const TExportTaskRequest & rhs) const
  {
    if (!(params == rhs.params))
      return false;
    return true;
  }
  bool operator != (const TExportTaskRequest &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const TExportTaskRequest & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(TExportTaskRequest &a, TExportTaskRequest &b);

inline std::ostream& operator<<(std::ostream& out, const TExportTaskRequest& obj)
{
  obj.printTo(out);
  return out;
}

} // namespace

#endif
