#include <arpc++/arpc++.h>

using namespace arpc;

bool ServerContext::IsCancelled() const {
  return true;
}
