#pragma once

#include <cstdint>
#include <string>

namespace Protocon {

struct Request {
    uint64_t time;
    uint16_t type;
    std::string data;
};

}  // namespace Protocon
