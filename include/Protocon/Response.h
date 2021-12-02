#pragma once

#include <cstdint>
#include <string>

namespace Protocon {

struct Response {
    uint64_t time;
    uint8_t status;
    std::string data;
};

}  // namespace Protocon
