#include <arpc++/arpc++.h>

#include "arpc_protocol.h"

// arpc::StatusCode is an enumeration that is declared in public headers, but
// depends on status code constants that are part of arpc_protocol.proto. Add
// static assertions to ensure these remain in sync.
static_assert(int(arpc::StatusCode::UNKNOWN) ==
                  arpc_protocol::StatusCode::UNKNOWN,
              "Status codes must match at the message and API level.");
static_assert(int(arpc::StatusCode::ABORTED) ==
                  arpc_protocol::StatusCode::ABORTED,
              "Status codes must match at the message and API level.");
static_assert(int(arpc::StatusCode::ALREADY_EXISTS) ==
                  arpc_protocol::StatusCode::ALREADY_EXISTS,
              "Status codes must match at the message and API level.");
static_assert(int(arpc::StatusCode::CANCELLED) ==
                  arpc_protocol::StatusCode::CANCELLED,
              "Status codes must match at the message and API level.");
static_assert(int(arpc::StatusCode::DATA_LOSS) ==
                  arpc_protocol::StatusCode::DATA_LOSS,
              "Status codes must match at the message and API level.");
static_assert(int(arpc::StatusCode::DEADLINE_EXCEEDED) ==
                  arpc_protocol::StatusCode::DEADLINE_EXCEEDED,
              "Status codes must match at the message and API level.");
static_assert(int(arpc::StatusCode::FAILED_PRECONDITION) ==
                  arpc_protocol::StatusCode::FAILED_PRECONDITION,
              "Status codes must match at the message and API level.");
static_assert(int(arpc::StatusCode::INTERNAL) ==
                  arpc_protocol::StatusCode::INTERNAL,
              "Status codes must match at the message and API level.");
static_assert(int(arpc::StatusCode::INVALID_ARGUMENT) ==
                  arpc_protocol::StatusCode::INVALID_ARGUMENT,
              "Status codes must match at the message and API level.");
static_assert(int(arpc::StatusCode::NOT_FOUND) ==
                  arpc_protocol::StatusCode::NOT_FOUND,
              "Status codes must match at the message and API level.");
static_assert(int(arpc::StatusCode::OK) == arpc_protocol::StatusCode::OK,
              "Status codes must match at the message and API level.");
static_assert(int(arpc::StatusCode::OUT_OF_RANGE) ==
                  arpc_protocol::StatusCode::OUT_OF_RANGE,
              "Status codes must match at the message and API level.");
static_assert(int(arpc::StatusCode::PERMISSION_DENIED) ==
                  arpc_protocol::StatusCode::PERMISSION_DENIED,
              "Status codes must match at the message and API level.");
static_assert(int(arpc::StatusCode::RESOURCE_EXHAUSTED) ==
                  arpc_protocol::StatusCode::RESOURCE_EXHAUSTED,
              "Status codes must match at the message and API level.");
static_assert(int(arpc::StatusCode::UNAUTHENTICATED) ==
                  arpc_protocol::StatusCode::UNAUTHENTICATED,
              "Status codes must match at the message and API level.");
static_assert(int(arpc::StatusCode::UNAVAILABLE) ==
                  arpc_protocol::StatusCode::UNAVAILABLE,
              "Status codes must match at the message and API level.");
static_assert(int(arpc::StatusCode::UNIMPLEMENTED) ==
                  arpc_protocol::StatusCode::UNIMPLEMENTED,
              "Status codes must match at the message and API level.");
