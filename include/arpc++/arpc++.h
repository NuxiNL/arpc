#ifndef ARPCXX_ARPCXX_H
#define ARPCXX_ARPCXX_H

#include <map>
#include <memory>
#include <string>
#include <string_view>

#include <argdata.h>

namespace arpc {

class FileDescriptor {
 private:
  FileDescriptor(FileDescriptor const&) = delete;
  void operator=(FileDescriptor const& x) = delete;
};

class FileDescriptorParser {
 public:
  virtual ~FileDescriptorParser() {
  }

  virtual std::shared_ptr<FileDescriptor> Get(const argdata_t& ad) = 0;
};

class Message {
 public:
  virtual ~Message() {
  }

  virtual void Parse(const argdata_t& ad,
                     FileDescriptorParser* file_descriptor_parser) = 0;
};

class Service {
 public:
  virtual std::string_view GetName() = 0;
};

enum class StatusCode {
  OK,
  UNIMPLEMENTED,
};

class Status {
 public:
  Status(StatusCode code, std::string_view message)
      : code_(code), message_(message) {
  }

  bool ok() const {
    return code_ == StatusCode::OK;
  }

 private:
  StatusCode code_;
  std::string message_;
};

class Channel {};

class ClientContext {};

class ClientReaderImpl {
 public:
  ClientReaderImpl(Channel* channel, std::string_view service,
                   std::string_view rpc, ClientContext* context,
                   const Message& request);

  Status Finish();
  bool Read(Message* msg);
};

template <typename R>
class ClientReader {
 public:
  template <typename W>
  ClientReader(Channel* channel, std::string_view service, std::string_view rpc,
               ClientContext* context, const W& request)
      : impl_(channel, service, rpc, context, request) {
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
  ClientWriterImpl(Channel* channel, std::string_view service,
                   std::string_view rpc, ClientContext* context,
                   Message* response);

  Status Finish();
  bool Write(const Message& msg);
  bool WritesDone();
};

template <typename W>
class ClientWriter {
 public:
  template <typename R>
  ClientWriter(Channel* channel, std::string_view service, std::string_view rpc,
               ClientContext* context, R* response)
      : impl_(channel, service, rpc, context, response) {
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
  ClientReaderWriterImpl(Channel* channel, std::string_view service,
                         std::string_view rpc, ClientContext* context);

  Status Finish();
  bool Read(Message* msg);
  bool Write(const Message& msg);
  bool WritesDone();
};

template <typename W, typename R>
class ClientReaderWriter {
 public:
  ClientReaderWriter(Channel* channel, std::string_view service,
                     std::string_view rpc, ClientContext* context)
      : impl_(channel, service, rpc, context) {
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

  void HandleRequests();

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

class ServerContext {};

template <typename R>
class ServerReader {};

template <typename W>
class ServerWriter {};

template <typename W, typename R>
class ServerReaderWriter {};

}  // namespace arpc

#endif
