/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef Partitions_TYPES_H
#define Partitions_TYPES_H

#include <iosfwd>

#include <thrift/Thrift.h>
#include <thrift/TApplicationException.h>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TTransport.h>

#include <thrift/cxxfunctional.h>
#include "Exprs_types.h"
#include "Types_types.h"


namespace palo {

struct TPartitionType {
  enum type {
    UNPARTITIONED = 0,
    RANDOM = 1,
    HASH_PARTITIONED = 2,
    RANGE_PARTITIONED = 3
  };
};

extern const std::map<int, const char*> _TPartitionType_VALUES_TO_NAMES;

struct TDistributionType {
  enum type {
    UNPARTITIONED = 0,
    RANDOM = 1,
    HASH_PARTITIONED = 2
  };
};

extern const std::map<int, const char*> _TDistributionType_VALUES_TO_NAMES;

class TPartitionKey;

class TPartitionRange;

class TRangePartition;

class TDataPartition;

typedef struct _TPartitionKey__isset {
  _TPartitionKey__isset() : type(false), key(false) {}
  bool type :1;
  bool key :1;
} _TPartitionKey__isset;

class TPartitionKey {
 public:

  TPartitionKey(const TPartitionKey&);
  TPartitionKey& operator=(const TPartitionKey&);
  TPartitionKey() : sign(0), type(( ::palo::TPrimitiveType::type)0), key() {
  }

  virtual ~TPartitionKey() throw();
  int16_t sign;
   ::palo::TPrimitiveType::type type;
  std::string key;

  _TPartitionKey__isset __isset;

  void __set_sign(const int16_t val);

  void __set_type(const  ::palo::TPrimitiveType::type val);

  void __set_key(const std::string& val);

  bool operator == (const TPartitionKey & rhs) const
  {
    if (!(sign == rhs.sign))
      return false;
    if (__isset.type != rhs.__isset.type)
      return false;
    else if (__isset.type && !(type == rhs.type))
      return false;
    if (__isset.key != rhs.__isset.key)
      return false;
    else if (__isset.key && !(key == rhs.key))
      return false;
    return true;
  }
  bool operator != (const TPartitionKey &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const TPartitionKey & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(TPartitionKey &a, TPartitionKey &b);

inline std::ostream& operator<<(std::ostream& out, const TPartitionKey& obj)
{
  obj.printTo(out);
  return out;
}


class TPartitionRange {
 public:

  TPartitionRange(const TPartitionRange&);
  TPartitionRange& operator=(const TPartitionRange&);
  TPartitionRange() : include_start_key(0), include_end_key(0) {
  }

  virtual ~TPartitionRange() throw();
  TPartitionKey start_key;
  TPartitionKey end_key;
  bool include_start_key;
  bool include_end_key;

  void __set_start_key(const TPartitionKey& val);

  void __set_end_key(const TPartitionKey& val);

  void __set_include_start_key(const bool val);

  void __set_include_end_key(const bool val);

  bool operator == (const TPartitionRange & rhs) const
  {
    if (!(start_key == rhs.start_key))
      return false;
    if (!(end_key == rhs.end_key))
      return false;
    if (!(include_start_key == rhs.include_start_key))
      return false;
    if (!(include_end_key == rhs.include_end_key))
      return false;
    return true;
  }
  bool operator != (const TPartitionRange &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const TPartitionRange & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(TPartitionRange &a, TPartitionRange &b);

inline std::ostream& operator<<(std::ostream& out, const TPartitionRange& obj)
{
  obj.printTo(out);
  return out;
}

typedef struct _TRangePartition__isset {
  _TRangePartition__isset() : distributed_exprs(false), distribute_bucket(false) {}
  bool distributed_exprs :1;
  bool distribute_bucket :1;
} _TRangePartition__isset;

class TRangePartition {
 public:

  TRangePartition(const TRangePartition&);
  TRangePartition& operator=(const TRangePartition&);
  TRangePartition() : partition_id(0), distribute_bucket(0) {
  }

  virtual ~TRangePartition() throw();
  int64_t partition_id;
  TPartitionRange range;
  std::vector< ::palo::TExpr>  distributed_exprs;
  int32_t distribute_bucket;

  _TRangePartition__isset __isset;

  void __set_partition_id(const int64_t val);

  void __set_range(const TPartitionRange& val);

  void __set_distributed_exprs(const std::vector< ::palo::TExpr> & val);

  void __set_distribute_bucket(const int32_t val);

  bool operator == (const TRangePartition & rhs) const
  {
    if (!(partition_id == rhs.partition_id))
      return false;
    if (!(range == rhs.range))
      return false;
    if (__isset.distributed_exprs != rhs.__isset.distributed_exprs)
      return false;
    else if (__isset.distributed_exprs && !(distributed_exprs == rhs.distributed_exprs))
      return false;
    if (__isset.distribute_bucket != rhs.__isset.distribute_bucket)
      return false;
    else if (__isset.distribute_bucket && !(distribute_bucket == rhs.distribute_bucket))
      return false;
    return true;
  }
  bool operator != (const TRangePartition &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const TRangePartition & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(TRangePartition &a, TRangePartition &b);

inline std::ostream& operator<<(std::ostream& out, const TRangePartition& obj)
{
  obj.printTo(out);
  return out;
}

typedef struct _TDataPartition__isset {
  _TDataPartition__isset() : partition_exprs(false), partition_infos(false) {}
  bool partition_exprs :1;
  bool partition_infos :1;
} _TDataPartition__isset;

class TDataPartition {
 public:

  TDataPartition(const TDataPartition&);
  TDataPartition& operator=(const TDataPartition&);
  TDataPartition() : type((TPartitionType::type)0) {
  }

  virtual ~TDataPartition() throw();
  TPartitionType::type type;
  std::vector< ::palo::TExpr>  partition_exprs;
  std::vector<TRangePartition>  partition_infos;

  _TDataPartition__isset __isset;

  void __set_type(const TPartitionType::type val);

  void __set_partition_exprs(const std::vector< ::palo::TExpr> & val);

  void __set_partition_infos(const std::vector<TRangePartition> & val);

  bool operator == (const TDataPartition & rhs) const
  {
    if (!(type == rhs.type))
      return false;
    if (__isset.partition_exprs != rhs.__isset.partition_exprs)
      return false;
    else if (__isset.partition_exprs && !(partition_exprs == rhs.partition_exprs))
      return false;
    if (__isset.partition_infos != rhs.__isset.partition_infos)
      return false;
    else if (__isset.partition_infos && !(partition_infos == rhs.partition_infos))
      return false;
    return true;
  }
  bool operator != (const TDataPartition &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const TDataPartition & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(TDataPartition &a, TDataPartition &b);

inline std::ostream& operator<<(std::ostream& out, const TDataPartition& obj)
{
  obj.printTo(out);
  return out;
}

} // namespace

#endif
