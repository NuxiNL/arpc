// Copyright (c) 2017 Nuxi (https://nuxi.nl/) and contributors.
//
// This file is distributed under a 2-clause BSD license.
// See the LICENSE file for details.

#include <cassert>

#include <arpc++/arpc++.h>
#include <argdata.hpp>

#include "arpc_protocol.h"

using namespace arpc;

ClientWriterImpl::ClientWriterImpl(Channel* channel, const RpcMethod& method,
                                   ClientContext* context, Message* response)
    : channel_(channel), response_(response), writes_done_(false) {
  // Send the start request.
  arpc_protocol::ClientMessage client_message;
  arpc_protocol::StreamingRequestStart* streaming_request_start =
      client_message.mutable_streaming_request_start();
  arpc_protocol::RpcMethod* rpc_method =
      streaming_request_start->mutable_rpc_method();
  rpc_method->set_service(method.GetService());
  rpc_method->set_rpc(method.GetRpc());

  std::unique_ptr<argdata_writer_t> writer = argdata_writer_t::create();
  ArgdataBuilder argdata_builder;
  writer->set(client_message.Build(&argdata_builder));
  int error = writer->push(channel_->GetFileDescriptor()->get());
  if (error != 0)
    status_ = Status(StatusCode::INTERNAL, strerror(error));
}

ClientWriterImpl::~ClientWriterImpl() {
  assert((writes_done_ || !status_.ok()) && "RPC only completed partially");
}

Status ClientWriterImpl::Finish() {
  assert(writes_done_ && "WritesDone() not called before Finish()");
  if (status_.ok())
    status_ = channel_->FinishUnaryResponse(response_);
  return status_;
}

bool ClientWriterImpl::Write(const Message& msg) {
  assert(!writes_done_ && "Cannot call Write() after WritesDone()");
  if (!status_.ok())
    return false;

  arpc_protocol::ClientMessage client_message;
  arpc_protocol::StreamingRequestData* streaming_request_data =
      client_message.mutable_streaming_request_data();
  ArgdataBuilder argdata_builder;
  streaming_request_data->set_request(msg.Build(&argdata_builder));

  std::unique_ptr<argdata_writer_t> writer = argdata_writer_t::create();
  writer->set(client_message.Build(&argdata_builder));
  int error = writer->push(channel_->GetFileDescriptor()->get());
  if (error != 0) {
    status_ = Status(StatusCode::INTERNAL, strerror(error));
    return false;
  }
  return true;
}

bool ClientWriterImpl::WritesDone() {
  assert(!writes_done_ && "Called WritesDone() twice");
  writes_done_ = true;
  if (!status_.ok())
    return false;

  arpc_protocol::ClientMessage client_message;
  arpc_protocol::StreamingRequestFinish* streaming_request_finish =
      client_message.mutable_streaming_request_finish();

  std::unique_ptr<argdata_writer_t> writer = argdata_writer_t::create();
  ArgdataBuilder argdata_builder;
  writer->set(client_message.Build(&argdata_builder));
  int error = writer->push(channel_->GetFileDescriptor()->get());
  if (error != 0) {
    status_ = Status(StatusCode::INTERNAL, strerror(error));
    return false;
  }
  return true;
}
