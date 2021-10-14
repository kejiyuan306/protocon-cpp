#pragma once

#include <cstdint>
#include <string>

namespace Protocon {

struct Response {
    uint64_t time;
    uint8_t status;
    std::u8string data;
};

}  // namespace Protocon
