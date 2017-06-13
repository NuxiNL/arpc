#include <sys/socket.h>

#include <errno.h>
#include <unistd.h>
#include <argdata.hpp>

#include <arpc++/arpc++.h>
#include <gtest/gtest.h>

TEST(Server, BadFileDescriptor) {
  // Attempting to use a bad file descriptor should trigger EBADF.
  arpc::ServerBuilder builder(-1);
  EXPECT_EQ(EBADF, builder.Build()->HandleRequest());
}

TEST(Server, EndOfFile) {
  // Close one half of a socket pair. Reading requests should return
  // end-of-file, which is encoded as -1.
  int fds[2];
  EXPECT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
  EXPECT_EQ(0, close(fds[0]));

  arpc::ServerBuilder builder(fds[1]);
  EXPECT_EQ(-1, builder.Build()->HandleRequest());
  EXPECT_EQ(0, close(fds[1]));
}

TEST(Server, BadMessage) {
  // A single byte does not correspond with a single message. The error
  // EBADMSG generated by the Argdata reader should propagate.
  int fds[2];
  EXPECT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
  EXPECT_EQ(1, write(fds[0], "a", 1));
  EXPECT_EQ(0, close(fds[0]));

  arpc::ServerBuilder builder(fds[1]);
  EXPECT_EQ(EBADMSG, builder.Build()->HandleRequest());
  EXPECT_EQ(0, close(fds[1]));
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

  arpc::ServerBuilder builder(fds[1]);
  EXPECT_EQ(EOPNOTSUPP, builder.Build()->HandleRequest());
  EXPECT_EQ(0, close(fds[1]));
}
