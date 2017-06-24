// Copyright (c) 2017 Nuxi (https://nuxi.nl/) and contributors.
//
// This file is distributed under a 2-clause BSD license.
// See the LICENSE file for details.

#include <cstring>
#include <memory>

#include <arpc++/arpc++.h>

#include "arpc_protocol.h"

using namespace arpc;

Status Channel::BlockingUnaryCall(const RpcMethod& method,
                                  ClientContext* context,
                                  const Message& request, Message* response) {
  // Send the request.
  arpc_protocol::ClientMessage client_message;
  arpc_protocol::UnaryRequest* unary_request =
      client_message.mutable_unary_request();
  arpc_protocol::RpcMethod* rpc_method = unary_request->mutable_rpc_method();
  rpc_method->set_service(method.first);
  rpc_method->set_rpc(method.second);
  ArgdataBuilder argdata_builder;
  unary_request->set_request(request.Build(&argdata_builder));

  std::unique_ptr<argdata_writer_t> writer = argdata_writer_t::create();
  writer->set(client_message.Build(&argdata_builder));
  int error = writer->push(fd_->get());
  if (error != 0)
    return Status(StatusCode::INTERNAL, strerror(error));

  // Process the response.
  return FinishUnaryResponse(response);
}

Status Channel::FinishUnaryResponse(Message* response) {
  // TODO(ed): Make message size configurable.
  std::unique_ptr<argdata_reader_t> reader = argdata_reader_t::create(4096, 16);
  int error = reader->pull(fd_->get());
  if (error != 0)
    return Status(StatusCode::INTERNAL, strerror(error));
  const argdata_t* server_response = reader->get();
  if (server_response == nullptr)
    return Status(StatusCode::INTERNAL, "Channel closed by server");

  ArgdataParser argdata_parser(reader.get());
  arpc_protocol::ServerMessage server_message;
  server_message.Parse(*server_response, &argdata_parser);
  if (!server_message.has_unary_response())
    return Status(StatusCode::INTERNAL, "Server sent invalid response");
  const arpc_protocol::UnaryResponse& unary_response =
      server_message.unary_response();
  // TODO(ed): Only do the parsing upon success!
  response->Clear();
  response->Parse(*unary_response.response(), &argdata_parser);
  const arpc_protocol::Status& status = unary_response.status();
  return Status(StatusCode(status.code()), status.message());
}

std::shared_ptr<Channel> arpc::CreateChannel(
    const std::shared_ptr<FileDescriptor>& fd) {
  return std::make_shared<Channel>(fd);
}
