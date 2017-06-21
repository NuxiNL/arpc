#include <cassert>

#include <arpc++/arpc++.h>
#include <argdata.hpp>

#include "arpc_protocol.h"

using namespace arpc;

ClientReaderImpl::ClientReaderImpl(Channel* channel, const RpcMethod& method,
                                   ClientContext* context,
                                   const Message& request)
    : fd_(channel->GetFileDescriptor()), reads_done_(false) {
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
  assert(reads_done_ && "RPC only completed partially");
}

Status ClientReaderImpl::Finish() {
  assert(reads_done_ && "RPC only completed partially");
  return status_;
}

bool ClientReaderImpl::Read(Message* msg) {
  // TODO(ed): Implement.
  reads_done_ = true;
  return false;
}
