#pragma once

#include <cstdint>

namespace Protocon {

struct RawSignInResponse {
    uint16_t cmdId;
    uint8_t status;
};

}  // namespace Protocon
