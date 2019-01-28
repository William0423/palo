/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef MetricDefs_TYPES_H
#define MetricDefs_TYPES_H

#include <iosfwd>

#include <thrift/Thrift.h>
#include <thrift/TApplicationException.h>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TTransport.h>

#include <thrift/cxxfunctional.h>
#include "Metrics_types.h"


namespace palo {

class TMetricDef;

typedef struct _TMetricDef__isset {
  _TMetricDef__isset() : key(false), kind(false), units(false), contexts(false), label(false), description(false) {}
  bool key :1;
  bool kind :1;
  bool units :1;
  bool contexts :1;
  bool label :1;
  bool description :1;
} _TMetricDef__isset;

class TMetricDef {
 public:

  TMetricDef(const TMetricDef&);
  TMetricDef& operator=(const TMetricDef&);
  TMetricDef() : key(), kind(( ::palo::TMetricKind::type)0), units(( ::palo::TUnit::type)0), label(), description() {
  }

  virtual ~TMetricDef() throw();
  std::string key;
   ::palo::TMetricKind::type kind;
   ::palo::TUnit::type units;
  std::vector<std::string>  contexts;
  std::string label;
  std::string description;

  _TMetricDef__isset __isset;

  void __set_key(const std::string& val);

  void __set_kind(const  ::palo::TMetricKind::type val);

  void __set_units(const  ::palo::TUnit::type val);

  void __set_contexts(const std::vector<std::string> & val);

  void __set_label(const std::string& val);

  void __set_description(const std::string& val);

  bool operator == (const TMetricDef & rhs) const
  {
    if (__isset.key != rhs.__isset.key)
      return false;
    else if (__isset.key && !(key == rhs.key))
      return false;
    if (__isset.kind != rhs.__isset.kind)
      return false;
    else if (__isset.kind && !(kind == rhs.kind))
      return false;
    if (__isset.units != rhs.__isset.units)
      return false;
    else if (__isset.units && !(units == rhs.units))
      return false;
    if (__isset.contexts != rhs.__isset.contexts)
      return false;
    else if (__isset.contexts && !(contexts == rhs.contexts))
      return false;
    if (__isset.label != rhs.__isset.label)
      return false;
    else if (__isset.label && !(label == rhs.label))
      return false;
    if (__isset.description != rhs.__isset.description)
      return false;
    else if (__isset.description && !(description == rhs.description))
      return false;
    return true;
  }
  bool operator != (const TMetricDef &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const TMetricDef & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(TMetricDef &a, TMetricDef &b);

inline std::ostream& operator<<(std::ostream& out, const TMetricDef& obj)
{
  obj.printTo(out);
  return out;
}

} // namespace

#endif
