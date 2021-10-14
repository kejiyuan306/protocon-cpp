#pragma once

#include <cstdint>

namespace Protocon {

struct RawSignUpResponse {
    uint16_t cmdId;
    uint64_t clientId;
    uint8_t status;
};

}  // namespace Protocon
