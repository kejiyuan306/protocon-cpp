#pragma once

#include <cstdint>
#include <string>

namespace Protocon {

struct RequestHeader {
    uint16_t commandId;
    uint64_t gatewayId;
    uint64_t clientId;
    uint64_t time;
    uint16_t apiVersion;
    uint16_t type;
};

struct ResponseHeader {
    uint16_t commandId;
    uint64_t time;
    uint8_t status;
};

struct Request {
    uint16_t type;
    std::string data;
};

struct Response {
    uint8_t status;
    std::string data;
};

}  // namespace Protocon
