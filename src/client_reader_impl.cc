#include <cassert>
#include <cstring>

#include <arpc++/arpc++.h>
#include <argdata.hpp>

#include "arpc_protocol.h"

using namespace arpc;

ClientReaderImpl::ClientReaderImpl(Channel* channel, const RpcMethod& method,
                                   ClientContext* context,
                                   const Message& request)
    : fd_(channel->GetFileDescriptor()), finished_(false) {
  // Send the request.
  arpc_protocol::ClientMessage client_message;
  arpc_protocol::UnaryRequest* unary_request =
      client_message.mutable_unary_request();
  arpc_protocol::RpcMethod* rpc_method = unary_request->mutable_rpc_method();
  rpc_method->set_service(method.GetService());
  rpc_method->set_rpc(method.GetRpc());
  ArgdataBuilder argdata_builder;
  unary_request->set_request(request.Build(&argdata_builder));
  unary_request->set_server_streaming(true);

  std::unique_ptr<argdata_writer_t> writer = argdata_writer_t::create();
  writer->set(client_message.Build(&argdata_builder));
  int error = writer->push(fd_->get());
  if (error != 0)
    status_ = Status(StatusCode::INTERNAL, strerror(error));
}

ClientReaderImpl::~ClientReaderImpl() {
  assert(finished_ && "RPC only completed partially");
}

Status ClientReaderImpl::Finish() {
  assert(finished_ && "RPC only completed partially");
  return status_;
}

bool ClientReaderImpl::Read(Message* msg) {
  if (finished_)
    return false;

  // TODO(ed): Make buffer size configurable!
  std::unique_ptr<argdata_reader_t> reader = argdata_reader_t::create(4096, 16);
  {
    int error = reader->pull(fd_->get());
    if (error != 0) {
      status_ = Status(StatusCode::INTERNAL, strerror(error));
      finished_ = true;
      return false;
    }
  }
  const argdata_t* input = reader->get();
  if (input == nullptr) {
    status_ = Status(StatusCode::INTERNAL, "Unexpected end-of-file");
    finished_ = true;
    return false;
  }

  // Parse the received message.
  ArgdataParser argdata_parser(reader.get());
  arpc_protocol::ServerMessage server_message;
  server_message.Parse(*input, &argdata_parser);

  if (server_message.has_streaming_response_data()) {
    // Server has sent an additional streamed message.
    const arpc_protocol::StreamingResponseData& streaming_response_data =
        server_message.streaming_response_data();
    // TODO(ed): Clear the message prior to parsing!
    msg->Parse(*streaming_response_data.response(), &argdata_parser);
    return true;
  } else if (server_message.has_streaming_response_finish()) {
    // Server has indicated no more messages are available for reading.
    const arpc_protocol::Status& status =
        server_message.streaming_response_finish().status();
    status_ = Status(StatusCode(status.code()), status.message());
    finished_ = true;
    return false;
  } else {
    status_ = Status(StatusCode::INTERNAL, "Unexpected response from server");
    finished_ = true;
    return false;
  }
}
