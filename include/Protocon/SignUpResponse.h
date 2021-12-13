#pragma once

#include <cstdint>

namespace Protocon {

struct SignUpResponse {
    uint64_t clientId;
    uint8_t status;
};

}  // namespace Protocon
