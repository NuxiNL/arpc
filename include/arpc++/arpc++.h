#ifndef ARPCXX_ARPCXX_H
#define ARPCXX_ARPCXX_H

#include <unistd.h>

#include <forward_list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>

#include <argdata.hpp>

namespace arpc {

class ClientContext;
class ServerContext;

class FileDescriptor {
 public:
  /// Take ownership of the given file descriptor. It can be used, but not
  /// closed.
  explicit FileDescriptor(int fd) : fd_(fd) {
  }

  ~FileDescriptor() {
    close(fd_);
  }

  int get_fd() {
    return fd_;
  }

 private:
  FileDescriptor(FileDescriptor const&) = delete;
  void operator=(FileDescriptor const& x) = delete;

  int fd_;
};

class ArgdataParser {
 public:
  ArgdataParser(argdata_reader_t* reader);
  ~ArgdataParser();

  const argdata_t* ParseAnyFromMap(const argdata_map_iterator_t& it);
  std::shared_ptr<FileDescriptor> ParseFileDescriptor(const argdata_t& ad);

 private:
  // Comparator for finding file descriptors in a set of shared pointers
  // to FileDescriptor objects.
  class FileDescriptorComparator {
   public:
    using is_transparent = std::true_type;

    bool operator()(const std::shared_ptr<FileDescriptor>& a,
                    const std::shared_ptr<FileDescriptor>& b) const {
      return a->get_fd() < b->get_fd();
    }
    bool operator()(int a, const std::shared_ptr<FileDescriptor>& b) const {
      return a < b->get_fd();
    }
    bool operator()(const std::shared_ptr<FileDescriptor>& a, int b) const {
      return a->get_fd() < b;
    }
  };

  argdata_reader_t* const reader_;
  std::set<std::shared_ptr<FileDescriptor>, FileDescriptorComparator>
      file_descriptors_;
};

class ArgdataBuilder {
 public:
  const argdata_t* BuildFileDescriptor(
      const std::shared_ptr<FileDescriptor>& value);
  const argdata_t* BuildMap(std::vector<const argdata_t*> keys,
                            std::vector<const argdata_t*> values);
  const argdata_t* BuildString(std::string_view value);

 private:
  std::vector<std::unique_ptr<argdata_t>> argdatas_;
  std::vector<std::shared_ptr<FileDescriptor>> file_descriptors_;
  std::forward_list<std::string> strings_;
  std::forward_list<std::vector<const argdata_t*>> vectors_;
};

class Message {
 public:
  virtual ~Message() {
  }

  virtual void Parse(const argdata_t& ad, ArgdataParser* argdata_parser) = 0;
  virtual const argdata_t* Build(ArgdataBuilder* argdata_builder) const = 0;
};

enum class StatusCode {
  UNKNOWN,
  ABORTED,
  ALREADY_EXISTS,
  CANCELLED,
  DATA_LOSS,
  DEADLINE_EXCEEDED,
  FAILED_PRECONDITION,
  INTERNAL,
  INVALID_ARGUMENT,
  NOT_FOUND,
  OK,
  OUT_OF_RANGE,
  PERMISSION_DENIED,
  RESOURCE_EXHAUSTED,
  UNAUTHENTICATED,
  UNAVAILABLE,
  UNIMPLEMENTED,
  // Don't use this one. This is to force users to include a default branch.
  DO_NOT_USE = -1
};

class Status {
 public:
  Status() : code_(StatusCode::OK) {
  }

  Status(StatusCode code, std::string_view message)
      : code_(code), message_(message) {
  }

  StatusCode error_code() const {
    return code_;
  }

  const std::string& error_message() const {
    return message_;
  }

  bool ok() const {
    return code_ == StatusCode::OK;
  }

 private:
  const StatusCode code_;
  const std::string message_;
};

class RpcMethod {
 public:
  RpcMethod(std::string_view service, std::string_view rpc)
      : service_(service), rpc_(rpc) {
  }

  std::string_view GetService() const {
    return service_;
  }

  std::string_view GetRpc() const {
    return rpc_;
  }

 private:
  std::string_view service_;
  std::string_view rpc_;
};

class Service {
 public:
  virtual std::string_view GetName() = 0;
  virtual Status BlockingUnaryCall(std::string_view rpc, ServerContext* context,
                                   const argdata_t& request,
                                   ArgdataParser* argdata_parser,
                                   const argdata_t** response,
                                   ArgdataBuilder* argdata_builder) = 0;
};

class Channel {
 public:
  explicit Channel(int fd) : fd_(fd) {
  }

  Status BlockingUnaryCall(const RpcMethod& method, ClientContext* context,
                           const Message& request, Message* response);

 private:
  int fd_;
};

std::shared_ptr<Channel> CreateChannel(int fd);

class ClientContext {};

class ClientReaderImpl {
 public:
  ClientReaderImpl(Channel* channel, const RpcMethod& method,
                   ClientContext* context, const Message& request);

  Status Finish();
  bool Read(Message* msg);
};

template <typename R>
class ClientReader {
 public:
  template <typename W>
  ClientReader(Channel* channel, const RpcMethod& method,
               ClientContext* context, const W& request)
      : impl_(channel, method, context, request) {
  }

  Status Finish() {
    return impl_.Finish();
  }

  bool Read(R* msg) {
    return impl_.Read(msg);
  }

 private:
  ClientReaderImpl impl_;
};

class ClientWriterImpl {
 public:
  ClientWriterImpl(Channel* channel, const RpcMethod& method,
                   ClientContext* context, Message* response);

  Status Finish();
  bool Write(const Message& msg);
  bool WritesDone();
};

template <typename W>
class ClientWriter {
 public:
  template <typename R>
  ClientWriter(Channel* channel, const RpcMethod& method,
               ClientContext* context, R* response)
      : impl_(channel, method, context, response) {
  }

  Status Finish() {
    return impl_.Finish();
  }

  bool Write(const W& msg) {
    return impl_.Write(msg);
  }

  bool WritesDone() {
    return impl_.WritesDone();
  }

 private:
  ClientWriterImpl impl_;
};

class ClientReaderWriterImpl {
 public:
  ClientReaderWriterImpl(Channel* channel, const RpcMethod& method,
                         ClientContext* context);

  Status Finish();
  bool Read(Message* msg);
  bool Write(const Message& msg);
  bool WritesDone();
};

template <typename W, typename R>
class ClientReaderWriter {
 public:
  ClientReaderWriter(Channel* channel, const RpcMethod& method,
                     ClientContext* context)
      : impl_(channel, method, context) {
  }

  Status Finish() {
    return impl_.Finish();
  }

  bool Read(R* msg) {
    return impl_.Read(msg);
  }

  bool Write(const W& msg) {
    return impl_.Write(msg);
  }

  bool WritesDone() {
    return impl_.WritesDone();
  }

 private:
  ClientReaderWriterImpl impl_;
};

class Server {
 public:
  Server(int fd, const std::map<std::string, Service*, std::less<>>& services)
      : fd_(fd), services_(services) {
  }

  int HandleRequest();

 private:
  const int fd_;
  const std::map<std::string, Service*, std::less<>> services_;
};

class ServerBuilder {
 public:
  ServerBuilder(int fd) : fd_(fd) {
  }

  std::unique_ptr<Server> Build() {
    return std::make_unique<Server>(fd_, services_);
  }

  void RegisterService(Service* service) {
    // TODO(ed): operator[] doesn't accept std::string_view?
    // services_[service->GetName()] = service;
    services_.emplace(service->GetName(), nullptr).first->second = service;
  }

 private:
  const int fd_;
  std::map<std::string, Service*, std::less<>> services_;
};

class ServerContext {
 public:
  bool IsCancelled() const;
};

template <typename R>
class ServerReader {};

template <typename W>
class ServerWriter {
 public:
  bool Write(const W& msg) {
    // TODO: implement
  }
};

template <typename W, typename R>
class ServerReaderWriter {};

}  // namespace arpc

#endif
