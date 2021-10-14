#pragma once

#include <Protocon/Response.h>

#include <cstdint>

namespace Protocon {

struct RawResponse {
    uint16_t cmdId;
    Response response;
};

}  // namespace Protocon
