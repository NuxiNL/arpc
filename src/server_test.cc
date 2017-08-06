// Copyright (c) 2017 Nuxi (https://nuxi.nl/) and contributors.
//
// This file is distributed under a 2-clause BSD license.
// See the LICENSE file for details.

#include <sys/socket.h>

#include <errno.h>
#include <unistd.h>

#include <cstdint>
#include <memory>
#include <thread>

#include <arpc++/arpc++.h>
#include <gtest/gtest.h>
#include <argdata.hpp>

#include "server_test_proto.h"

TEST(Server, EndOfFile) {
  // Close one half of a socket pair. Reading requests should return
  // end-of-file, which is encoded as -1.
  int fds[2];
  EXPECT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
  EXPECT_EQ(0, close(fds[0]));

  arpc::ServerBuilder builder(std::make_shared<arpc::FileDescriptor>(fds[1]));
  EXPECT_EQ(-1, builder.Build()->HandleRequest());
}

TEST(Server, BadMessage) {
  // A single byte does not correspond with a single message. The error
  // EBADMSG generated by the Argdata reader should propagate.
  int fds[2];
  EXPECT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
  EXPECT_EQ(1, write(fds[0], "a", 1));
  EXPECT_EQ(0, close(fds[0]));

  arpc::ServerBuilder builder(std::make_shared<arpc::FileDescriptor>(fds[1]));
  EXPECT_EQ(EBADMSG, builder.Build()->HandleRequest());
}

TEST(Server, InvalidOperation) {
  // Writing some garbage Argdata should make the server return EOPNOTSUPP.
  // TODO(ed): Should this just return an error to the client?
  int fds[2];
  EXPECT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
  std::unique_ptr<argdata_writer_t> writer = argdata_writer_t::create();
  writer->set(argdata_t::null());
  EXPECT_EQ(0, writer->push(fds[0]));
  EXPECT_EQ(0, close(fds[0]));

  arpc::ServerBuilder builder(std::make_shared<arpc::FileDescriptor>(fds[1]));
  EXPECT_EQ(EOPNOTSUPP, builder.Build()->HandleRequest());
}

TEST(Server, ServiceNotRegistered) {
  // Invoke an RPC on a server that has no services registered. Any RPC
  // should fail with UNIMPLEMENTED to indicate the service's absence.
  int fds[2];
  EXPECT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
  std::shared_ptr<arpc::Channel> channel =
      arpc::CreateChannel(std::make_shared<arpc::FileDescriptor>(fds[0]));
  std::unique_ptr<server_test_proto::UnaryService::Stub> stub =
      server_test_proto::UnaryService::NewStub(channel);
  std::thread caller([&stub]() {
    arpc::ClientContext context;
    server_test_proto::UnaryInput input;
    server_test_proto::UnaryOutput output;
    arpc::Status status = stub->UnaryCall(&context, input, &output);
    EXPECT_EQ(arpc::StatusCode::UNIMPLEMENTED, status.error_code());
    EXPECT_EQ("Service not registered", status.error_message());
  });

  arpc::ServerBuilder builder(std::make_shared<arpc::FileDescriptor>(fds[1]));
  EXPECT_EQ(0, builder.Build()->HandleRequest());
  caller.join();
}

// Simple service that does nothing more than echoing responses.
namespace {
class EchoService final : public server_test_proto::UnaryService::Service {
 public:
  arpc::Status UnaryCall(arpc::ServerContext* context,
                         const server_test_proto::UnaryInput* request,
                         server_test_proto::UnaryOutput* response) override {
    response->set_text(request->text());
    response->set_file_descriptor(request->file_descriptor());
    return arpc::Status::OK;
  }
};
}

TEST(Server, UnaryEcho) {
  // Invoke RPCs on the EchoService and check whether the input text
  // properly ends up in the output.
  int fds[2];
  EXPECT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
  std::shared_ptr<arpc::Channel> channel =
      arpc::CreateChannel(std::make_shared<arpc::FileDescriptor>(fds[0]));
  std::unique_ptr<server_test_proto::UnaryService::Stub> stub =
      server_test_proto::UnaryService::NewStub(channel);
  std::thread caller([&stub]() {
    arpc::ClientContext context;
    server_test_proto::UnaryInput input;
    server_test_proto::UnaryOutput output;

    input.set_text("Hello, world!");
    EXPECT_TRUE(stub->UnaryCall(&context, input, &output).ok());
    EXPECT_EQ("Hello, world!", output.text());

    input.set_text("Goodbye, world!");
    EXPECT_TRUE(stub->UnaryCall(&context, input, &output).ok());
    EXPECT_EQ("Goodbye, world!", output.text());
  });

  arpc::ServerBuilder builder(std::make_shared<arpc::FileDescriptor>(fds[1]));
  EchoService service;
  builder.RegisterService(&service);
  std::shared_ptr<arpc::Server> server = builder.Build();
  EXPECT_EQ(0, server->HandleRequest());
  EXPECT_EQ(0, server->HandleRequest());
  caller.join();
  EXPECT_EQ(0, close(fds[0]));
}

TEST(Server, UnaryFileDesciptorPassing) {
  // Use the EchoService to pass a file descriptor back to us.
  int fds[2];
  EXPECT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
  std::shared_ptr<arpc::Channel> channel =
      arpc::CreateChannel(std::make_shared<arpc::FileDescriptor>(fds[0]));
  std::unique_ptr<server_test_proto::UnaryService::Stub> stub =
      server_test_proto::UnaryService::NewStub(channel);
  std::thread caller([&stub]() {
    arpc::ClientContext context;
    server_test_proto::UnaryInput input;
    server_test_proto::UnaryOutput output;

    // Write something into the pipe and send the read side to the
    // EchoService.
    int pfds[2];
    EXPECT_EQ(0, pipe(pfds));
    EXPECT_EQ(5, write(pfds[1], "Hello", 5));
    EXPECT_EQ(0, close(pfds[1]));
    input.set_file_descriptor(std::make_shared<arpc::FileDescriptor>(pfds[0]));
    EXPECT_TRUE(stub->UnaryCall(&context, input, &output).ok());

    // Original message should still be contained in the pipe.
    char buf[6];
    EXPECT_EQ(5, read(output.file_descriptor()->get(), buf, sizeof(buf)));
    EXPECT_EQ("Hello", std::string_view(buf, 5));
  });

  arpc::ServerBuilder builder(std::make_shared<arpc::FileDescriptor>(fds[1]));
  EchoService service;
  builder.RegisterService(&service);
  std::shared_ptr<arpc::Server> server = builder.Build();
  EXPECT_EQ(0, server->HandleRequest());
  caller.join();
}

// Service that adds a stream of numbers.
namespace {
class AdderService final
    : public server_test_proto::ClientStreamAdderService::Service {
 public:
  arpc::Status Add(arpc::ServerContext* context,
                   arpc::ServerReader<server_test_proto::AdderInput>* reader,
                   server_test_proto::AdderOutput* response) override {
    server_test_proto::AdderInput input;
    std::int32_t sum = 0;
    while (reader->Read(&input))
      sum += input.value();
    response->set_sum(sum);
    return arpc::Status::OK;
  }
};
}

TEST(Server, ClientStreamAdder) {
  // Use the AdderService to add some numbers together.
  int fds[2];
  EXPECT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
  std::shared_ptr<arpc::Channel> channel =
      arpc::CreateChannel(std::make_shared<arpc::FileDescriptor>(fds[0]));
  std::unique_ptr<server_test_proto::ClientStreamAdderService::Stub> stub =
      server_test_proto::ClientStreamAdderService::NewStub(channel);
  std::thread caller([&stub]() {
    arpc::ClientContext context;
    server_test_proto::AdderInput input;
    server_test_proto::AdderOutput output;

    // Write numbers and extract the sum.
    std::unique_ptr<arpc::ClientWriter<server_test_proto::AdderInput>> writer(
        stub->Add(&context, &output));
    input.set_value(237);
    EXPECT_TRUE(writer->Write(input));
    input.set_value(7845);
    EXPECT_TRUE(writer->Write(input));
    input.set_value(57592);
    EXPECT_TRUE(writer->Write(input));
    input.set_value(3);
    EXPECT_TRUE(writer->Write(input));
    input.set_value(7284);
    EXPECT_TRUE(writer->Write(input));
    EXPECT_TRUE(writer->WritesDone());
    EXPECT_TRUE(writer->Finish().ok());
    EXPECT_EQ(72961, output.sum());
  });

  arpc::ServerBuilder builder(std::make_shared<arpc::FileDescriptor>(fds[1]));
  AdderService service;
  builder.RegisterService(&service);
  std::shared_ptr<arpc::Server> server = builder.Build();
  EXPECT_EQ(0, server->HandleRequest());
  caller.join();
}

namespace {
class FibonacciService final
    : public server_test_proto::ServerStreamFibonacciService::Service {
 public:
  arpc::Status GetSequence(
      arpc::ServerContext* context,
      const server_test_proto::FibonacciInput* request,
      arpc::ServerWriter<server_test_proto::FibonacciOutput>* writer) override {
    std::uint64_t a = request->a();
    std::uint64_t b = request->b();
    for (std::uint32_t i = 0; i < request->terms(); ++i) {
      server_test_proto::FibonacciOutput output;
      output.set_term(a);
      if (!writer->Write(output))
        break;
      std::tie(a, b) = std::make_pair(b, a + b);
    }
    return arpc::Status::OK;
  }
};
}

TEST(Server, ServerStreamFibonacci) {
  // Use the FibonacciService to stream a sequence of messages to a client.
  int fds[2];
  EXPECT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
  std::shared_ptr<arpc::Channel> channel =
      arpc::CreateChannel(std::make_shared<arpc::FileDescriptor>(fds[0]));
  EXPECT_EQ(ARPC_CHANNEL_READY, channel->GetState(false));
  std::unique_ptr<server_test_proto::ServerStreamFibonacciService::Stub> stub =
      server_test_proto::ServerStreamFibonacciService::NewStub(channel);
  std::thread caller([&stub]() {
    arpc::ClientContext context;
    server_test_proto::FibonacciInput input;
    server_test_proto::FibonacciOutput output;

    // Request the first five Brady numbers (OEIS A247698).
    input.set_a(2308);
    input.set_b(4261);
    input.set_terms(5);
    std::unique_ptr<arpc::ClientReader<server_test_proto::FibonacciOutput>>
        reader(stub->GetSequence(&context, input));
    EXPECT_TRUE(reader->Read(&output));
    EXPECT_EQ(2308, output.term());
    EXPECT_TRUE(reader->Read(&output));
    EXPECT_EQ(4261, output.term());
    EXPECT_TRUE(reader->Read(&output));
    EXPECT_EQ(6569, output.term());
    EXPECT_TRUE(reader->Read(&output));
    EXPECT_EQ(10830, output.term());
    EXPECT_TRUE(reader->Read(&output));
    EXPECT_EQ(17399, output.term());
    EXPECT_FALSE(reader->Read(&output));
    EXPECT_TRUE(reader->Finish().ok());
  });

  // Process a single request from a client. The channel should still be
  // in the ready state after the RPC completes.
  {
    arpc::ServerBuilder builder(std::make_shared<arpc::FileDescriptor>(fds[1]));
    FibonacciService service;
    builder.RegisterService(&service);
    std::shared_ptr<arpc::Server> server = builder.Build();
    EXPECT_EQ(0, server->HandleRequest());
    caller.join();
    EXPECT_EQ(ARPC_CHANNEL_READY, channel->GetState(false));
  }

  // Destroying the server immediately causes the channel to be switched
  // to the shut down state.
  EXPECT_EQ(ARPC_CHANNEL_SHUTDOWN, channel->GetState(false));

  // Sending another RPC after the server has terminated should cause
  // the RPC to fail. The channel should remain in the shut down state.
  {
    arpc::ClientContext context;
    server_test_proto::FibonacciInput input;
    server_test_proto::FibonacciOutput output;
    input.set_a(1);
    input.set_b(1);
    input.set_terms(5);
    std::unique_ptr<arpc::ClientReader<server_test_proto::FibonacciOutput>>
        reader(stub->GetSequence(&context, input));
    EXPECT_FALSE(reader->Read(&output));
    EXPECT_FALSE(reader->Finish().ok());
    EXPECT_EQ(ARPC_CHANNEL_SHUTDOWN, channel->GetState(false));
  }
}
