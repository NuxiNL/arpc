// Copyright (c) 2017 Nuxi (https://nuxi.nl/) and contributors.
//
// This file is distributed under a 2-clause BSD license.
// See the LICENSE file for details.

#include <cassert>

#include <arpc++/arpc++.h>
#include <argdata.hpp>

#include "arpc_protocol.h"

using namespace arpc;

ServerReaderImpl::~ServerReaderImpl() {
  assert(finished_ && "Server returned without processing all messages");
}

bool ServerReaderImpl::Read(Message* msg) {
  if (finished_)
    return false;

  // TODO(ed): Make buffer size configurable!
  std::unique_ptr<argdata_reader_t> reader = argdata_reader_t::create(4096, 16);
  {
    int error = reader->pull(fd_->get());
    if (error != 0)
      return error;
  }
  const argdata_t* input = reader->get();
  if (input == nullptr) {
    finished_ = true;
    return false;
  }

  // Parse the received message.
  ArgdataParser argdata_parser(reader.get());
  arpc_protocol::ClientMessage client_message;
  client_message.Parse(*input, &argdata_parser);

  if (client_message.has_streaming_request_data()) {
    // Client has sent an additional streamed message.
    const arpc_protocol::StreamingRequestData& streaming_request_data =
        client_message.streaming_request_data();
    msg->Clear();
    msg->Parse(*streaming_request_data.request(), &argdata_parser);
    return true;
  } else if (client_message.has_streaming_request_finish()) {
    // Client has indicated no more messages are available for reading.
    finished_ = true;
    return false;
  } else {
    // TODO(ed): What should we do here?
    finished_ = true;
    return false;
  }
}
