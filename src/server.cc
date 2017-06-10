#include <thread>

#include <arpc++/arpc++.h>
#include <argdata.hpp>

using namespace arpc;

int Server::HandleRequests() {
  std::lock_guard<std::mutex> guard(lock_);
  // TODO(ed): Make this configurable.
  if (!reader_)
    reader_ = argdata_reader_t::create(4096, 16);

  // TODO(ed): Implement.
  return ENOSYS;
}
