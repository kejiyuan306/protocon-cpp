#pragma once

#include <Protocon/Request.h>

#include <cstdint>

namespace Protocon {

struct RawRequest {
    uint16_t cmdId;
    uint64_t gatewayId;
    uint64_t clientId;
    uint16_t apiVersion;
    Request request;
};

}  // namespace Protocon
