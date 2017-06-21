// Copyright (c) 2017 Nuxi (https://nuxi.nl/) and contributors.
//
// This file is distributed under a 2-clause BSD license.
// See the LICENSE file for details.

#include <arpc++/arpc++.h>

#include "arpc_protocol.h"

using namespace arpc;

// StatusCode is an enumeration that is declared in public headers, but
// depends on status code constants that are part of arpc_protocol.proto.
// Add static assertions to ensure these remain in sync.
static_assert(int(StatusCode::UNKNOWN) == arpc_protocol::StatusCode::UNKNOWN,
              "Status codes must match at the message and API level.");
static_assert(int(StatusCode::ABORTED) == arpc_protocol::StatusCode::ABORTED,
              "Status codes must match at the message and API level.");
static_assert(int(StatusCode::ALREADY_EXISTS) ==
                  arpc_protocol::StatusCode::ALREADY_EXISTS,
              "Status codes must match at the message and API level.");
static_assert(int(StatusCode::CANCELLED) ==
                  arpc_protocol::StatusCode::CANCELLED,
              "Status codes must match at the message and API level.");
static_assert(int(StatusCode::DATA_LOSS) ==
                  arpc_protocol::StatusCode::DATA_LOSS,
              "Status codes must match at the message and API level.");
static_assert(int(StatusCode::DEADLINE_EXCEEDED) ==
                  arpc_protocol::StatusCode::DEADLINE_EXCEEDED,
              "Status codes must match at the message and API level.");
static_assert(int(StatusCode::FAILED_PRECONDITION) ==
                  arpc_protocol::StatusCode::FAILED_PRECONDITION,
              "Status codes must match at the message and API level.");
static_assert(int(StatusCode::INTERNAL) == arpc_protocol::StatusCode::INTERNAL,
              "Status codes must match at the message and API level.");
static_assert(int(StatusCode::INVALID_ARGUMENT) ==
                  arpc_protocol::StatusCode::INVALID_ARGUMENT,
              "Status codes must match at the message and API level.");
static_assert(int(StatusCode::NOT_FOUND) ==
                  arpc_protocol::StatusCode::NOT_FOUND,
              "Status codes must match at the message and API level.");
static_assert(int(StatusCode::OK) == arpc_protocol::StatusCode::OK,
              "Status codes must match at the message and API level.");
static_assert(int(StatusCode::OUT_OF_RANGE) ==
                  arpc_protocol::StatusCode::OUT_OF_RANGE,
              "Status codes must match at the message and API level.");
static_assert(int(StatusCode::PERMISSION_DENIED) ==
                  arpc_protocol::StatusCode::PERMISSION_DENIED,
              "Status codes must match at the message and API level.");
static_assert(int(StatusCode::RESOURCE_EXHAUSTED) ==
                  arpc_protocol::StatusCode::RESOURCE_EXHAUSTED,
              "Status codes must match at the message and API level.");
static_assert(int(StatusCode::UNAUTHENTICATED) ==
                  arpc_protocol::StatusCode::UNAUTHENTICATED,
              "Status codes must match at the message and API level.");
static_assert(int(StatusCode::UNAVAILABLE) ==
                  arpc_protocol::StatusCode::UNAVAILABLE,
              "Status codes must match at the message and API level.");
static_assert(int(StatusCode::UNIMPLEMENTED) ==
                  arpc_protocol::StatusCode::UNIMPLEMENTED,
              "Status codes must match at the message and API level.");
