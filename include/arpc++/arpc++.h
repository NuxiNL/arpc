#ifndef ARPCXX_ARPCXX_H
#define ARPCXX_ARPCXX_H

#include <string_view>

namespace arpc {

enum class StatusCode {
  UNIMPLEMENTED,
};

class Status {
 public:
  Status(StatusCode code, std::string_view message) {
  }
};

class ServerContext {};

template <typename TI>
class ServerReader {};

template <typename TO>
class ServerWriter {};

template <typename TI, typename TO>
class ServerReaderWriter {};

}  // namespace arpc

#endif
