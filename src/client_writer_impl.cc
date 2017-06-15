#include <cassert>

#include <arpc++/arpc++.h>
#include <argdata.hpp>

#include "arpc_protocol.h"

using namespace arpc;

ClientWriterImpl::ClientWriterImpl(Channel* channel, const RpcMethod& method,
                                   ClientContext* context, Message* response)
    : fd_(channel->GetFileDescriptor()),
      response_(response),
      writes_done_(false) {
  // Send the start request.
  {
    arpc_protocol::ClientMessage client_message;
    arpc_protocol::ClientStreamingStartRequest* client_streaming_start_request =
        client_message.mutable_client_streaming_start_request();
    arpc_protocol::RpcMethod* rpc_method =
        client_streaming_start_request->mutable_rpc_method();
    rpc_method->set_service(method.GetService());
    rpc_method->set_rpc(method.GetRpc());

    std::unique_ptr<argdata_writer_t> writer = argdata_writer_t::create();
    ArgdataBuilder argdata_builder;
    writer->set(client_message.Build(&argdata_builder));
    int error = writer->push(fd_->get());
    if (error != 0) {
      status_ = Status(StatusCode::INTERNAL, strerror(error));
      return;
    }
  }

  // Process the start response.
  // TODO(ed): Move this into WaitForInitialMetadata()?
  {
    // TODO(ed): Make message size configurable.
    std::unique_ptr<argdata_reader_t> reader =
        argdata_reader_t::create(4096, 16);
    int error = reader->pull(fd_->get());
    if (error != 0) {
      status_ = Status(StatusCode::INTERNAL, strerror(error));
      return;
    }
    const argdata_t* server_response = reader->get();
    if (server_response == nullptr) {
      status_ = Status(StatusCode::INTERNAL, "Channel closed by server");
      return;
    }

    ArgdataParser argdata_parser(reader.get());
    arpc_protocol::ServerMessage server_message;
    server_message.Parse(*server_response, &argdata_parser);
    if (!server_message.has_client_streaming_start_response()) {
      status_ = Status(StatusCode::INTERNAL, "Server sent invalid response");
      return;
    }
    const arpc_protocol::ClientStreamingStartResponse&
        client_streaming_start_response =
            server_message.client_streaming_start_response();
    const arpc_protocol::Status& status =
        client_streaming_start_response.status();
    status_ = Status(StatusCode(status.code()), status.message());
  }
}

ClientWriterImpl::~ClientWriterImpl() {
  assert((writes_done_ || !status_.ok()) && "RPC only completed partially");
}

Status ClientWriterImpl::Finish() {
  assert(writes_done_ && "WritesDone() not called before Finish()");
  if (!status_.ok())
    return status_;

  // TODO(ed): Implement.
  return Status(StatusCode::UNIMPLEMENTED, "TODO(ed): Implement.");
}

bool ClientWriterImpl::Write(const Message& msg) {
  assert(!writes_done_ && "Cannot call Write() after WritesDone()");
  if (!status_.ok())
    return false;

  // TODO(ed): Implement.
  return false;
}

bool ClientWriterImpl::WritesDone() {
  assert(!writes_done_ && "Called WritesDone() twice");
  writes_done_ = true;
  return status_.ok();
}
