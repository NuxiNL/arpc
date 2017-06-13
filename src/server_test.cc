#include <sys/socket.h>

#include <errno.h>
#include <unistd.h>

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
