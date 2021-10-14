#pragma once

#include <cstdint>
#include <string>

namespace Protocon {

struct Request {
    uint64_t time;
    uint16_t type;
    std::u8string data;
};

}  // namespace Protocon
