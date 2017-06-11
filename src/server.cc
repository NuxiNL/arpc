#include <thread>

#include <arpc++/arpc++.h>
#include <argdata.hpp>

#include "argdata_reader_file_descriptor_parser.h"
#include "arpc_protocol.h"

using namespace arpc;

int Server::HandleRequest() {
  // Read the next message from the socket.
  // TODO(ed): Make configurable!
  std::unique_ptr<argdata_reader_t> reader = argdata_reader_t::create(4096, 16);
  {
    int error = reader->pull(fd_);
    if (error != 0)
      return error;
  }

  // Parse the received message.
  ArgdataReaderFileDescriptorParser file_descriptor_parser(reader.get());
  arpc_protocol::ClientMessage client_message;
  client_message.Parse(*reader->get(), &file_descriptor_parser);

  if (client_message.has_unary_request()) {
    // Simple unary call. Dispatch the right service.
    const arpc_protocol::UnaryRequest& unary_request =
        client_message.unary_request();
    const arpc_protocol::RpcMethod& rpc_method = unary_request.rpc_method();
    arpc_protocol::ServerMessage server_message;
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
      Status rpc_status = service->second->BlockingUnaryCall(
          rpc_method.rpc(), &context, *argdata_t::null(), &file_descriptor_parser);
      arpc_protocol::Status* status = unary_response->mutable_status();
      // TODO(ed): Fix!
      // status->set_code(rpc_status.error_code());
      status->set_message(rpc_status.error_message());
    }
  } else {
  }
  return 0;
}
