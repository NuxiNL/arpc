#include <thread>

#include <arpc++/arpc++.h>
#include <argdata.hpp>

#include "argdata_reader_file_descriptor_parser.h"
#include "arpc_protocol.h"

using namespace arpc;

int Server::HandleRequest() {
  // Attempt to read the next message from the socket.
  std::unique_ptr<argdata_reader_t> reader;
  {
    std::lock_guard<std::mutex> guard(reader_lock_);
    // TODO(ed): Make maximum message size configurable.
    if (!reader_)
      reader_ = argdata_reader_t::create(4096, 16);
    int error = reader_->pull(fd_);
    if (error != 0)
      return error;
    reader = std::move(reader_);
  }

  // Parse the received message.
  ArgdataReaderFileDescriptorParser releaser(reader.get());
  arpc_protocol::ClientMessage message;
  message.Parse(*reader->get(), &releaser);

  // TODO(ed): Process the incoming message!

  return 0;
}
