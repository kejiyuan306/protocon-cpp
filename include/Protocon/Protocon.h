#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Protocon {

struct ReceivedRequest {
    uint16_t commandId;
    uint64_t gatewayId;
    uint64_t clientId;
    uint64_t time;
    uint16_t apiVersion;
    uint16_t type;
    std::string data;
};

struct ReceivedResponse {
    uint16_t commandId;
    uint64_t time;
    uint8_t status;
    std::string data;
};

struct Request {
    uint16_t type;
    std::string data;
};

struct Response {
    uint8_t status;
    std::string data;
};

class RequestHandler {
  public:
    virtual ~RequestHandler(){};

    virtual uint16_t type() = 0;
    virtual Response handle(const ReceivedRequest& request) = 0;
};

class ResponseHandler {
  public:
    virtual ~ResponseHandler(){};

    virtual uint16_t type() = 0;
    virtual void handle(const ReceivedResponse& response) = 0;
};

}  // namespace Protocon
