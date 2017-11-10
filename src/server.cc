// Copyright (c) 2017 Nuxi (https://nuxi.nl/) and contributors.
//
// SPDX-License-Identifier: BSD-2-Clause

#include <thread>

#include <arpc++/arpc++.h>
#include <argdata.hpp>

#include "arpc_protocol.h"

using namespace arpc;

int Server::HandleRequest() {
  // Read the next message from the socket. Return end-of-file as -1.
  // TODO(ed): Make buffer size configurable!
  std::unique_ptr<argdata_reader_t> reader = argdata_reader_t::create(4096, 16);
  {
    int error = reader->pull(fd_->get());
    if (error != 0)
      return error;
  }
  const argdata_t* input = reader->get();
  if (input == nullptr)
    return -1;

  // Parse the received message.
  ArgdataParser argdata_parser(reader.get());
  arpc_protocol::ClientMessage client_message;
  client_message.Parse(*input, &argdata_parser);

  if (client_message.has_unary_request()) {
    const arpc_protocol::UnaryRequest& unary_request =
        client_message.unary_request();
    const arpc_protocol::RpcMethod& rpc_method = unary_request.rpc_method();

    ArgdataBuilder argdata_builder;
    arpc_protocol::ServerMessage server_message;
    if (unary_request.server_streaming()) {
      // Server-streaming call.
      arpc_protocol::StreamingResponseFinish* streaming_response_finish =
          server_message.mutable_streaming_response_finish();
      auto service = services_.find(rpc_method.service());
      if (service == services_.end()) {
        // Service not found.
        arpc_protocol::Status* status =
            streaming_response_finish->mutable_status();
        status->set_code(arpc_protocol::StatusCode::UNIMPLEMENTED);
        status->set_message("Service not registered");
      } else {
        // Service found. Invoke call.
        ServerContext context;
        const argdata_t* response = argdata_t::null();
        ServerWriterImpl writer(fd_);
        Status rpc_status = service->second->BlockingServerStreamingCall(
            rpc_method.rpc(), &context, *unary_request.request(),
            &argdata_parser, &writer);
        arpc_protocol::Status* status =
            streaming_response_finish->mutable_status();
        status->set_code(arpc_protocol::StatusCode(rpc_status.error_code()));
        status->set_message(rpc_status.error_message());
      }
    } else {
      // Simple unary call.
      arpc_protocol::UnaryResponse* unary_response =
          server_message.mutable_unary_response();
      auto service = services_.find(rpc_method.service());
      if (service == services_.end()) {
        // Service not found.
        arpc_protocol::Status* status = unary_response->mutable_status();
        status->set_code(arpc_protocol::StatusCode::UNIMPLEMENTED);
        status->set_message("Service not registered");
      } else {
        // Service found. Invoke call.
        ServerContext context;
        const argdata_t* response = argdata_t::null();
        Status rpc_status = service->second->BlockingUnaryCall(
            rpc_method.rpc(), &context, *unary_request.request(),
            &argdata_parser, &response, &argdata_builder);
        arpc_protocol::Status* status = unary_response->mutable_status();
        status->set_code(arpc_protocol::StatusCode(rpc_status.error_code()));
        status->set_message(rpc_status.error_message());
        unary_response->set_response(response);
      }
    }

    std::unique_ptr<argdata_writer_t> writer = argdata_writer_t::create();
    writer->set(server_message.Build(&argdata_builder));
    return writer->push(fd_->get());
  } else if (client_message.has_streaming_request_start()) {
    // Client-streaming call.
    // TODO(ed): Implement bidirectional streaming calls?
    const arpc_protocol::StreamingRequestStart& streaming_request_start =
        client_message.streaming_request_start();
    const arpc_protocol::RpcMethod& rpc_method =
        streaming_request_start.rpc_method();
    arpc_protocol::ServerMessage server_message;
    arpc_protocol::UnaryResponse* unary_response =
        server_message.mutable_unary_response();

    // Find corresponding service.
    ArgdataBuilder argdata_builder;
    auto service = services_.find(rpc_method.service());
    if (service == services_.end()) {
      // Service not found.
      arpc_protocol::Status* status = unary_response->mutable_status();
      status->set_code(arpc_protocol::StatusCode::UNIMPLEMENTED);
      status->set_message("Service not registered");
    } else {
      // Service found. Invoke call.
      ServerContext context;
      ServerReaderImpl reader(fd_);
      const argdata_t* response = argdata_t::null();
      Status rpc_status = service->second->BlockingClientStreamingCall(
          rpc_method.rpc(), &context, &reader, &response, &argdata_builder);
      arpc_protocol::Status* status = unary_response->mutable_status();
      status->set_code(arpc_protocol::StatusCode(rpc_status.error_code()));
      status->set_message(rpc_status.error_message());
      unary_response->set_response(response);
    }

    std::unique_ptr<argdata_writer_t> writer = argdata_writer_t::create();
    writer->set(server_message.Build(&argdata_builder));
    return writer->push(fd_->get());
  } else {
    // Invalid operation.
    return EOPNOTSUPP;
  }
}
