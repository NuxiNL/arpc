// Copyright (c) 2017 Nuxi (https://nuxi.nl/) and contributors.
//
// SPDX-License-Identifier: BSD-2-Clause

#include <arpc++/arpc++.h>
#include <argdata.hpp>

#include "arpc_protocol.h"

using namespace arpc;

bool ServerWriterImpl::Write(const Message& msg) {
  if (finished_)
    return false;

  arpc_protocol::ServerMessage server_message;
  arpc_protocol::StreamingResponseData* streaming_response_data =
      server_message.mutable_streaming_response_data();
  ArgdataBuilder argdata_builder;
  streaming_response_data->set_response(msg.Build(&argdata_builder));

  std::unique_ptr<argdata_writer_t> writer = argdata_writer_t::create();
  writer->set(server_message.Build(&argdata_builder));
  int error = writer->push(fd_->get());
  if (error != 0) {
    finished_ = true;
    return false;
  }
  return true;
}
