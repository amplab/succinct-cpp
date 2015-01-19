/**
 * Autogenerated by Thrift Compiler (0.9.1)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef SuccinctService_H
#define SuccinctService_H

#include <thrift/TDispatchProcessor.h>
#include "thrift/succinct_types.h"



class SuccinctServiceIf {
 public:
  virtual ~SuccinctServiceIf() {}
  virtual int32_t connect_to_handlers() = 0;
  virtual int32_t disconnect_from_handlers() = 0;
  virtual int32_t connect_to_local_servers() = 0;
  virtual int32_t disconnect_from_local_servers() = 0;
  virtual int32_t start_servers() = 0;
  virtual int32_t initialize(const int32_t mode) = 0;
  virtual void get(std::string& _return, const int64_t key) = 0;
  virtual void get_local(std::string& _return, const int64_t key) = 0;
  virtual int32_t get_num_hosts() = 0;
  virtual int32_t get_num_shards(const int32_t host_id) = 0;
  virtual int32_t get_num_keys(const int32_t shard_id) = 0;
};

class SuccinctServiceIfFactory {
 public:
  typedef SuccinctServiceIf Handler;

  virtual ~SuccinctServiceIfFactory() {}

  virtual SuccinctServiceIf* getHandler(const ::apache::thrift::TConnectionInfo& connInfo) = 0;
  virtual void releaseHandler(SuccinctServiceIf* /* handler */) = 0;
};

class SuccinctServiceIfSingletonFactory : virtual public SuccinctServiceIfFactory {
 public:
  SuccinctServiceIfSingletonFactory(const boost::shared_ptr<SuccinctServiceIf>& iface) : iface_(iface) {}
  virtual ~SuccinctServiceIfSingletonFactory() {}

  virtual SuccinctServiceIf* getHandler(const ::apache::thrift::TConnectionInfo&) {
    return iface_.get();
  }
  virtual void releaseHandler(SuccinctServiceIf* /* handler */) {}

 protected:
  boost::shared_ptr<SuccinctServiceIf> iface_;
};

class SuccinctServiceNull : virtual public SuccinctServiceIf {
 public:
  virtual ~SuccinctServiceNull() {}
  int32_t connect_to_handlers() {
    int32_t _return = 0;
    return _return;
  }
  int32_t disconnect_from_handlers() {
    int32_t _return = 0;
    return _return;
  }
  int32_t connect_to_local_servers() {
    int32_t _return = 0;
    return _return;
  }
  int32_t disconnect_from_local_servers() {
    int32_t _return = 0;
    return _return;
  }
  int32_t start_servers() {
    int32_t _return = 0;
    return _return;
  }
  int32_t initialize(const int32_t /* mode */) {
    int32_t _return = 0;
    return _return;
  }
  void get(std::string& /* _return */, const int64_t /* key */) {
    return;
  }
  void get_local(std::string& /* _return */, const int64_t /* key */) {
    return;
  }
  int32_t get_num_hosts() {
    int32_t _return = 0;
    return _return;
  }
  int32_t get_num_shards(const int32_t /* host_id */) {
    int32_t _return = 0;
    return _return;
  }
  int32_t get_num_keys(const int32_t /* shard_id */) {
    int32_t _return = 0;
    return _return;
  }
};


class SuccinctService_connect_to_handlers_args {
 public:

  SuccinctService_connect_to_handlers_args() {
  }

  virtual ~SuccinctService_connect_to_handlers_args() throw() {}


  bool operator == (const SuccinctService_connect_to_handlers_args & /* rhs */) const
  {
    return true;
  }
  bool operator != (const SuccinctService_connect_to_handlers_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_connect_to_handlers_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SuccinctService_connect_to_handlers_pargs {
 public:


  virtual ~SuccinctService_connect_to_handlers_pargs() throw() {}


  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_connect_to_handlers_result__isset {
  _SuccinctService_connect_to_handlers_result__isset() : success(false) {}
  bool success;
} _SuccinctService_connect_to_handlers_result__isset;

class SuccinctService_connect_to_handlers_result {
 public:

  SuccinctService_connect_to_handlers_result() : success(0) {
  }

  virtual ~SuccinctService_connect_to_handlers_result() throw() {}

  int32_t success;

  _SuccinctService_connect_to_handlers_result__isset __isset;

  void __set_success(const int32_t val) {
    success = val;
  }

  bool operator == (const SuccinctService_connect_to_handlers_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const SuccinctService_connect_to_handlers_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_connect_to_handlers_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_connect_to_handlers_presult__isset {
  _SuccinctService_connect_to_handlers_presult__isset() : success(false) {}
  bool success;
} _SuccinctService_connect_to_handlers_presult__isset;

class SuccinctService_connect_to_handlers_presult {
 public:


  virtual ~SuccinctService_connect_to_handlers_presult() throw() {}

  int32_t* success;

  _SuccinctService_connect_to_handlers_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};


class SuccinctService_disconnect_from_handlers_args {
 public:

  SuccinctService_disconnect_from_handlers_args() {
  }

  virtual ~SuccinctService_disconnect_from_handlers_args() throw() {}


  bool operator == (const SuccinctService_disconnect_from_handlers_args & /* rhs */) const
  {
    return true;
  }
  bool operator != (const SuccinctService_disconnect_from_handlers_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_disconnect_from_handlers_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SuccinctService_disconnect_from_handlers_pargs {
 public:


  virtual ~SuccinctService_disconnect_from_handlers_pargs() throw() {}


  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_disconnect_from_handlers_result__isset {
  _SuccinctService_disconnect_from_handlers_result__isset() : success(false) {}
  bool success;
} _SuccinctService_disconnect_from_handlers_result__isset;

class SuccinctService_disconnect_from_handlers_result {
 public:

  SuccinctService_disconnect_from_handlers_result() : success(0) {
  }

  virtual ~SuccinctService_disconnect_from_handlers_result() throw() {}

  int32_t success;

  _SuccinctService_disconnect_from_handlers_result__isset __isset;

  void __set_success(const int32_t val) {
    success = val;
  }

  bool operator == (const SuccinctService_disconnect_from_handlers_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const SuccinctService_disconnect_from_handlers_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_disconnect_from_handlers_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_disconnect_from_handlers_presult__isset {
  _SuccinctService_disconnect_from_handlers_presult__isset() : success(false) {}
  bool success;
} _SuccinctService_disconnect_from_handlers_presult__isset;

class SuccinctService_disconnect_from_handlers_presult {
 public:


  virtual ~SuccinctService_disconnect_from_handlers_presult() throw() {}

  int32_t* success;

  _SuccinctService_disconnect_from_handlers_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};


class SuccinctService_connect_to_local_servers_args {
 public:

  SuccinctService_connect_to_local_servers_args() {
  }

  virtual ~SuccinctService_connect_to_local_servers_args() throw() {}


  bool operator == (const SuccinctService_connect_to_local_servers_args & /* rhs */) const
  {
    return true;
  }
  bool operator != (const SuccinctService_connect_to_local_servers_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_connect_to_local_servers_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SuccinctService_connect_to_local_servers_pargs {
 public:


  virtual ~SuccinctService_connect_to_local_servers_pargs() throw() {}


  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_connect_to_local_servers_result__isset {
  _SuccinctService_connect_to_local_servers_result__isset() : success(false) {}
  bool success;
} _SuccinctService_connect_to_local_servers_result__isset;

class SuccinctService_connect_to_local_servers_result {
 public:

  SuccinctService_connect_to_local_servers_result() : success(0) {
  }

  virtual ~SuccinctService_connect_to_local_servers_result() throw() {}

  int32_t success;

  _SuccinctService_connect_to_local_servers_result__isset __isset;

  void __set_success(const int32_t val) {
    success = val;
  }

  bool operator == (const SuccinctService_connect_to_local_servers_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const SuccinctService_connect_to_local_servers_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_connect_to_local_servers_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_connect_to_local_servers_presult__isset {
  _SuccinctService_connect_to_local_servers_presult__isset() : success(false) {}
  bool success;
} _SuccinctService_connect_to_local_servers_presult__isset;

class SuccinctService_connect_to_local_servers_presult {
 public:


  virtual ~SuccinctService_connect_to_local_servers_presult() throw() {}

  int32_t* success;

  _SuccinctService_connect_to_local_servers_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};


class SuccinctService_disconnect_from_local_servers_args {
 public:

  SuccinctService_disconnect_from_local_servers_args() {
  }

  virtual ~SuccinctService_disconnect_from_local_servers_args() throw() {}


  bool operator == (const SuccinctService_disconnect_from_local_servers_args & /* rhs */) const
  {
    return true;
  }
  bool operator != (const SuccinctService_disconnect_from_local_servers_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_disconnect_from_local_servers_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SuccinctService_disconnect_from_local_servers_pargs {
 public:


  virtual ~SuccinctService_disconnect_from_local_servers_pargs() throw() {}


  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_disconnect_from_local_servers_result__isset {
  _SuccinctService_disconnect_from_local_servers_result__isset() : success(false) {}
  bool success;
} _SuccinctService_disconnect_from_local_servers_result__isset;

class SuccinctService_disconnect_from_local_servers_result {
 public:

  SuccinctService_disconnect_from_local_servers_result() : success(0) {
  }

  virtual ~SuccinctService_disconnect_from_local_servers_result() throw() {}

  int32_t success;

  _SuccinctService_disconnect_from_local_servers_result__isset __isset;

  void __set_success(const int32_t val) {
    success = val;
  }

  bool operator == (const SuccinctService_disconnect_from_local_servers_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const SuccinctService_disconnect_from_local_servers_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_disconnect_from_local_servers_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_disconnect_from_local_servers_presult__isset {
  _SuccinctService_disconnect_from_local_servers_presult__isset() : success(false) {}
  bool success;
} _SuccinctService_disconnect_from_local_servers_presult__isset;

class SuccinctService_disconnect_from_local_servers_presult {
 public:


  virtual ~SuccinctService_disconnect_from_local_servers_presult() throw() {}

  int32_t* success;

  _SuccinctService_disconnect_from_local_servers_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};


class SuccinctService_start_servers_args {
 public:

  SuccinctService_start_servers_args() {
  }

  virtual ~SuccinctService_start_servers_args() throw() {}


  bool operator == (const SuccinctService_start_servers_args & /* rhs */) const
  {
    return true;
  }
  bool operator != (const SuccinctService_start_servers_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_start_servers_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SuccinctService_start_servers_pargs {
 public:


  virtual ~SuccinctService_start_servers_pargs() throw() {}


  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_start_servers_result__isset {
  _SuccinctService_start_servers_result__isset() : success(false) {}
  bool success;
} _SuccinctService_start_servers_result__isset;

class SuccinctService_start_servers_result {
 public:

  SuccinctService_start_servers_result() : success(0) {
  }

  virtual ~SuccinctService_start_servers_result() throw() {}

  int32_t success;

  _SuccinctService_start_servers_result__isset __isset;

  void __set_success(const int32_t val) {
    success = val;
  }

  bool operator == (const SuccinctService_start_servers_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const SuccinctService_start_servers_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_start_servers_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_start_servers_presult__isset {
  _SuccinctService_start_servers_presult__isset() : success(false) {}
  bool success;
} _SuccinctService_start_servers_presult__isset;

class SuccinctService_start_servers_presult {
 public:


  virtual ~SuccinctService_start_servers_presult() throw() {}

  int32_t* success;

  _SuccinctService_start_servers_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

typedef struct _SuccinctService_initialize_args__isset {
  _SuccinctService_initialize_args__isset() : mode(false) {}
  bool mode;
} _SuccinctService_initialize_args__isset;

class SuccinctService_initialize_args {
 public:

  SuccinctService_initialize_args() : mode(0) {
  }

  virtual ~SuccinctService_initialize_args() throw() {}

  int32_t mode;

  _SuccinctService_initialize_args__isset __isset;

  void __set_mode(const int32_t val) {
    mode = val;
  }

  bool operator == (const SuccinctService_initialize_args & rhs) const
  {
    if (!(mode == rhs.mode))
      return false;
    return true;
  }
  bool operator != (const SuccinctService_initialize_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_initialize_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SuccinctService_initialize_pargs {
 public:


  virtual ~SuccinctService_initialize_pargs() throw() {}

  const int32_t* mode;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_initialize_result__isset {
  _SuccinctService_initialize_result__isset() : success(false) {}
  bool success;
} _SuccinctService_initialize_result__isset;

class SuccinctService_initialize_result {
 public:

  SuccinctService_initialize_result() : success(0) {
  }

  virtual ~SuccinctService_initialize_result() throw() {}

  int32_t success;

  _SuccinctService_initialize_result__isset __isset;

  void __set_success(const int32_t val) {
    success = val;
  }

  bool operator == (const SuccinctService_initialize_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const SuccinctService_initialize_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_initialize_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_initialize_presult__isset {
  _SuccinctService_initialize_presult__isset() : success(false) {}
  bool success;
} _SuccinctService_initialize_presult__isset;

class SuccinctService_initialize_presult {
 public:


  virtual ~SuccinctService_initialize_presult() throw() {}

  int32_t* success;

  _SuccinctService_initialize_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

typedef struct _SuccinctService_get_args__isset {
  _SuccinctService_get_args__isset() : key(false) {}
  bool key;
} _SuccinctService_get_args__isset;

class SuccinctService_get_args {
 public:

  SuccinctService_get_args() : key(0) {
  }

  virtual ~SuccinctService_get_args() throw() {}

  int64_t key;

  _SuccinctService_get_args__isset __isset;

  void __set_key(const int64_t val) {
    key = val;
  }

  bool operator == (const SuccinctService_get_args & rhs) const
  {
    if (!(key == rhs.key))
      return false;
    return true;
  }
  bool operator != (const SuccinctService_get_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_get_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SuccinctService_get_pargs {
 public:


  virtual ~SuccinctService_get_pargs() throw() {}

  const int64_t* key;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_get_result__isset {
  _SuccinctService_get_result__isset() : success(false) {}
  bool success;
} _SuccinctService_get_result__isset;

class SuccinctService_get_result {
 public:

  SuccinctService_get_result() : success() {
  }

  virtual ~SuccinctService_get_result() throw() {}

  std::string success;

  _SuccinctService_get_result__isset __isset;

  void __set_success(const std::string& val) {
    success = val;
  }

  bool operator == (const SuccinctService_get_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const SuccinctService_get_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_get_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_get_presult__isset {
  _SuccinctService_get_presult__isset() : success(false) {}
  bool success;
} _SuccinctService_get_presult__isset;

class SuccinctService_get_presult {
 public:


  virtual ~SuccinctService_get_presult() throw() {}

  std::string* success;

  _SuccinctService_get_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

typedef struct _SuccinctService_get_local_args__isset {
  _SuccinctService_get_local_args__isset() : key(false) {}
  bool key;
} _SuccinctService_get_local_args__isset;

class SuccinctService_get_local_args {
 public:

  SuccinctService_get_local_args() : key(0) {
  }

  virtual ~SuccinctService_get_local_args() throw() {}

  int64_t key;

  _SuccinctService_get_local_args__isset __isset;

  void __set_key(const int64_t val) {
    key = val;
  }

  bool operator == (const SuccinctService_get_local_args & rhs) const
  {
    if (!(key == rhs.key))
      return false;
    return true;
  }
  bool operator != (const SuccinctService_get_local_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_get_local_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SuccinctService_get_local_pargs {
 public:


  virtual ~SuccinctService_get_local_pargs() throw() {}

  const int64_t* key;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_get_local_result__isset {
  _SuccinctService_get_local_result__isset() : success(false) {}
  bool success;
} _SuccinctService_get_local_result__isset;

class SuccinctService_get_local_result {
 public:

  SuccinctService_get_local_result() : success() {
  }

  virtual ~SuccinctService_get_local_result() throw() {}

  std::string success;

  _SuccinctService_get_local_result__isset __isset;

  void __set_success(const std::string& val) {
    success = val;
  }

  bool operator == (const SuccinctService_get_local_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const SuccinctService_get_local_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_get_local_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_get_local_presult__isset {
  _SuccinctService_get_local_presult__isset() : success(false) {}
  bool success;
} _SuccinctService_get_local_presult__isset;

class SuccinctService_get_local_presult {
 public:


  virtual ~SuccinctService_get_local_presult() throw() {}

  std::string* success;

  _SuccinctService_get_local_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};


class SuccinctService_get_num_hosts_args {
 public:

  SuccinctService_get_num_hosts_args() {
  }

  virtual ~SuccinctService_get_num_hosts_args() throw() {}


  bool operator == (const SuccinctService_get_num_hosts_args & /* rhs */) const
  {
    return true;
  }
  bool operator != (const SuccinctService_get_num_hosts_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_get_num_hosts_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SuccinctService_get_num_hosts_pargs {
 public:


  virtual ~SuccinctService_get_num_hosts_pargs() throw() {}


  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_get_num_hosts_result__isset {
  _SuccinctService_get_num_hosts_result__isset() : success(false) {}
  bool success;
} _SuccinctService_get_num_hosts_result__isset;

class SuccinctService_get_num_hosts_result {
 public:

  SuccinctService_get_num_hosts_result() : success(0) {
  }

  virtual ~SuccinctService_get_num_hosts_result() throw() {}

  int32_t success;

  _SuccinctService_get_num_hosts_result__isset __isset;

  void __set_success(const int32_t val) {
    success = val;
  }

  bool operator == (const SuccinctService_get_num_hosts_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const SuccinctService_get_num_hosts_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_get_num_hosts_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_get_num_hosts_presult__isset {
  _SuccinctService_get_num_hosts_presult__isset() : success(false) {}
  bool success;
} _SuccinctService_get_num_hosts_presult__isset;

class SuccinctService_get_num_hosts_presult {
 public:


  virtual ~SuccinctService_get_num_hosts_presult() throw() {}

  int32_t* success;

  _SuccinctService_get_num_hosts_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

typedef struct _SuccinctService_get_num_shards_args__isset {
  _SuccinctService_get_num_shards_args__isset() : host_id(false) {}
  bool host_id;
} _SuccinctService_get_num_shards_args__isset;

class SuccinctService_get_num_shards_args {
 public:

  SuccinctService_get_num_shards_args() : host_id(0) {
  }

  virtual ~SuccinctService_get_num_shards_args() throw() {}

  int32_t host_id;

  _SuccinctService_get_num_shards_args__isset __isset;

  void __set_host_id(const int32_t val) {
    host_id = val;
  }

  bool operator == (const SuccinctService_get_num_shards_args & rhs) const
  {
    if (!(host_id == rhs.host_id))
      return false;
    return true;
  }
  bool operator != (const SuccinctService_get_num_shards_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_get_num_shards_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SuccinctService_get_num_shards_pargs {
 public:


  virtual ~SuccinctService_get_num_shards_pargs() throw() {}

  const int32_t* host_id;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_get_num_shards_result__isset {
  _SuccinctService_get_num_shards_result__isset() : success(false) {}
  bool success;
} _SuccinctService_get_num_shards_result__isset;

class SuccinctService_get_num_shards_result {
 public:

  SuccinctService_get_num_shards_result() : success(0) {
  }

  virtual ~SuccinctService_get_num_shards_result() throw() {}

  int32_t success;

  _SuccinctService_get_num_shards_result__isset __isset;

  void __set_success(const int32_t val) {
    success = val;
  }

  bool operator == (const SuccinctService_get_num_shards_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const SuccinctService_get_num_shards_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_get_num_shards_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_get_num_shards_presult__isset {
  _SuccinctService_get_num_shards_presult__isset() : success(false) {}
  bool success;
} _SuccinctService_get_num_shards_presult__isset;

class SuccinctService_get_num_shards_presult {
 public:


  virtual ~SuccinctService_get_num_shards_presult() throw() {}

  int32_t* success;

  _SuccinctService_get_num_shards_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

typedef struct _SuccinctService_get_num_keys_args__isset {
  _SuccinctService_get_num_keys_args__isset() : shard_id(false) {}
  bool shard_id;
} _SuccinctService_get_num_keys_args__isset;

class SuccinctService_get_num_keys_args {
 public:

  SuccinctService_get_num_keys_args() : shard_id(0) {
  }

  virtual ~SuccinctService_get_num_keys_args() throw() {}

  int32_t shard_id;

  _SuccinctService_get_num_keys_args__isset __isset;

  void __set_shard_id(const int32_t val) {
    shard_id = val;
  }

  bool operator == (const SuccinctService_get_num_keys_args & rhs) const
  {
    if (!(shard_id == rhs.shard_id))
      return false;
    return true;
  }
  bool operator != (const SuccinctService_get_num_keys_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_get_num_keys_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SuccinctService_get_num_keys_pargs {
 public:


  virtual ~SuccinctService_get_num_keys_pargs() throw() {}

  const int32_t* shard_id;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_get_num_keys_result__isset {
  _SuccinctService_get_num_keys_result__isset() : success(false) {}
  bool success;
} _SuccinctService_get_num_keys_result__isset;

class SuccinctService_get_num_keys_result {
 public:

  SuccinctService_get_num_keys_result() : success(0) {
  }

  virtual ~SuccinctService_get_num_keys_result() throw() {}

  int32_t success;

  _SuccinctService_get_num_keys_result__isset __isset;

  void __set_success(const int32_t val) {
    success = val;
  }

  bool operator == (const SuccinctService_get_num_keys_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const SuccinctService_get_num_keys_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SuccinctService_get_num_keys_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _SuccinctService_get_num_keys_presult__isset {
  _SuccinctService_get_num_keys_presult__isset() : success(false) {}
  bool success;
} _SuccinctService_get_num_keys_presult__isset;

class SuccinctService_get_num_keys_presult {
 public:


  virtual ~SuccinctService_get_num_keys_presult() throw() {}

  int32_t* success;

  _SuccinctService_get_num_keys_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

class SuccinctServiceClient : virtual public SuccinctServiceIf {
 public:
  SuccinctServiceClient(boost::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) :
    piprot_(prot),
    poprot_(prot) {
    iprot_ = prot.get();
    oprot_ = prot.get();
  }
  SuccinctServiceClient(boost::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, boost::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) :
    piprot_(iprot),
    poprot_(oprot) {
    iprot_ = iprot.get();
    oprot_ = oprot.get();
  }
  boost::shared_ptr< ::apache::thrift::protocol::TProtocol> getInputProtocol() {
    return piprot_;
  }
  boost::shared_ptr< ::apache::thrift::protocol::TProtocol> getOutputProtocol() {
    return poprot_;
  }
  int32_t connect_to_handlers();
  void send_connect_to_handlers();
  int32_t recv_connect_to_handlers();
  int32_t disconnect_from_handlers();
  void send_disconnect_from_handlers();
  int32_t recv_disconnect_from_handlers();
  int32_t connect_to_local_servers();
  void send_connect_to_local_servers();
  int32_t recv_connect_to_local_servers();
  int32_t disconnect_from_local_servers();
  void send_disconnect_from_local_servers();
  int32_t recv_disconnect_from_local_servers();
  int32_t start_servers();
  void send_start_servers();
  int32_t recv_start_servers();
  int32_t initialize(const int32_t mode);
  void send_initialize(const int32_t mode);
  int32_t recv_initialize();
  void get(std::string& _return, const int64_t key);
  void send_get(const int64_t key);
  void recv_get(std::string& _return);
  void get_local(std::string& _return, const int64_t key);
  void send_get_local(const int64_t key);
  void recv_get_local(std::string& _return);
  int32_t get_num_hosts();
  void send_get_num_hosts();
  int32_t recv_get_num_hosts();
  int32_t get_num_shards(const int32_t host_id);
  void send_get_num_shards(const int32_t host_id);
  int32_t recv_get_num_shards();
  int32_t get_num_keys(const int32_t shard_id);
  void send_get_num_keys(const int32_t shard_id);
  int32_t recv_get_num_keys();
 protected:
  boost::shared_ptr< ::apache::thrift::protocol::TProtocol> piprot_;
  boost::shared_ptr< ::apache::thrift::protocol::TProtocol> poprot_;
  ::apache::thrift::protocol::TProtocol* iprot_;
  ::apache::thrift::protocol::TProtocol* oprot_;
};

class SuccinctServiceProcessor : public ::apache::thrift::TDispatchProcessor {
 protected:
  boost::shared_ptr<SuccinctServiceIf> iface_;
  virtual bool dispatchCall(::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, const std::string& fname, int32_t seqid, void* callContext);
 private:
  typedef  void (SuccinctServiceProcessor::*ProcessFunction)(int32_t, ::apache::thrift::protocol::TProtocol*, ::apache::thrift::protocol::TProtocol*, void*);
  typedef std::map<std::string, ProcessFunction> ProcessMap;
  ProcessMap processMap_;
  void process_connect_to_handlers(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_disconnect_from_handlers(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_connect_to_local_servers(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_disconnect_from_local_servers(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_start_servers(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_initialize(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_get(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_get_local(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_get_num_hosts(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_get_num_shards(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_get_num_keys(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
 public:
  SuccinctServiceProcessor(boost::shared_ptr<SuccinctServiceIf> iface) :
    iface_(iface) {
    processMap_["connect_to_handlers"] = &SuccinctServiceProcessor::process_connect_to_handlers;
    processMap_["disconnect_from_handlers"] = &SuccinctServiceProcessor::process_disconnect_from_handlers;
    processMap_["connect_to_local_servers"] = &SuccinctServiceProcessor::process_connect_to_local_servers;
    processMap_["disconnect_from_local_servers"] = &SuccinctServiceProcessor::process_disconnect_from_local_servers;
    processMap_["start_servers"] = &SuccinctServiceProcessor::process_start_servers;
    processMap_["initialize"] = &SuccinctServiceProcessor::process_initialize;
    processMap_["get"] = &SuccinctServiceProcessor::process_get;
    processMap_["get_local"] = &SuccinctServiceProcessor::process_get_local;
    processMap_["get_num_hosts"] = &SuccinctServiceProcessor::process_get_num_hosts;
    processMap_["get_num_shards"] = &SuccinctServiceProcessor::process_get_num_shards;
    processMap_["get_num_keys"] = &SuccinctServiceProcessor::process_get_num_keys;
  }

  virtual ~SuccinctServiceProcessor() {}
};

class SuccinctServiceProcessorFactory : public ::apache::thrift::TProcessorFactory {
 public:
  SuccinctServiceProcessorFactory(const ::boost::shared_ptr< SuccinctServiceIfFactory >& handlerFactory) :
      handlerFactory_(handlerFactory) {}

  ::boost::shared_ptr< ::apache::thrift::TProcessor > getProcessor(const ::apache::thrift::TConnectionInfo& connInfo);

 protected:
  ::boost::shared_ptr< SuccinctServiceIfFactory > handlerFactory_;
};

class SuccinctServiceMultiface : virtual public SuccinctServiceIf {
 public:
  SuccinctServiceMultiface(std::vector<boost::shared_ptr<SuccinctServiceIf> >& ifaces) : ifaces_(ifaces) {
  }
  virtual ~SuccinctServiceMultiface() {}
 protected:
  std::vector<boost::shared_ptr<SuccinctServiceIf> > ifaces_;
  SuccinctServiceMultiface() {}
  void add(boost::shared_ptr<SuccinctServiceIf> iface) {
    ifaces_.push_back(iface);
  }
 public:
  int32_t connect_to_handlers() {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->connect_to_handlers();
    }
    return ifaces_[i]->connect_to_handlers();
  }

  int32_t disconnect_from_handlers() {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->disconnect_from_handlers();
    }
    return ifaces_[i]->disconnect_from_handlers();
  }

  int32_t connect_to_local_servers() {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->connect_to_local_servers();
    }
    return ifaces_[i]->connect_to_local_servers();
  }

  int32_t disconnect_from_local_servers() {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->disconnect_from_local_servers();
    }
    return ifaces_[i]->disconnect_from_local_servers();
  }

  int32_t start_servers() {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->start_servers();
    }
    return ifaces_[i]->start_servers();
  }

  int32_t initialize(const int32_t mode) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->initialize(mode);
    }
    return ifaces_[i]->initialize(mode);
  }

  void get(std::string& _return, const int64_t key) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->get(_return, key);
    }
    ifaces_[i]->get(_return, key);
    return;
  }

  void get_local(std::string& _return, const int64_t key) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->get_local(_return, key);
    }
    ifaces_[i]->get_local(_return, key);
    return;
  }

  int32_t get_num_hosts() {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->get_num_hosts();
    }
    return ifaces_[i]->get_num_hosts();
  }

  int32_t get_num_shards(const int32_t host_id) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->get_num_shards(host_id);
    }
    return ifaces_[i]->get_num_shards(host_id);
  }

  int32_t get_num_keys(const int32_t shard_id) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->get_num_keys(shard_id);
    }
    return ifaces_[i]->get_num_keys(shard_id);
  }

};



#endif
