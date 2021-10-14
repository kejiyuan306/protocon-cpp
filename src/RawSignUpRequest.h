#pragma once

#include <cstdint>

namespace Protocon {

struct RawSignUpRequest {
    uint16_t cmdId;
    uint64_t gatewayId;
};

}  // namespace Protocon
