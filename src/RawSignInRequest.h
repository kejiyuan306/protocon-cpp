#pragma once

#include <cstdint>

namespace Protocon {

struct RawSignInRequest {
    uint16_t cmdId;
    uint64_t gatewayId;
    uint64_t clientId;
};

}  // namespace Protocon
