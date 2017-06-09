#ifndef ARPCXX_ARPCXX_H
#define ARPCXX_ARPCXX_H

#include <memory>
#include <string_view>

#include <argdata.h>

namespace arpc {

class Message {
 public:
  virtual ~Message() {
  }

  virtual void Parse(const argdata_t& ad) = 0;
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

class ServerContext {};

template <typename R>
class ServerReader {};

template <typename W>
class ServerWriter {};

template <typename W, typename R>
class ServerReaderWriter {};

}  // namespace arpc

#endif
