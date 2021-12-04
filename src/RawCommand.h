#pragma once

#include <Protocon/Request.h>
#include <Protocon/Response.h>

#include <cstdint>

namespace Protocon {

struct RawRequest {
    uint16_t cmdId;
    uint64_t gatewayId;
    uint64_t clientId;
    uint16_t apiVersion;
    Request request;
};

struct RawResponse {
    uint16_t cmdId;
    Response response;
};

struct RawSignInRequest {
    uint16_t cmdId;
    uint64_t gatewayId;
    uint64_t clientId;
};

struct RawSignInResponse {
    uint16_t cmdId;
    uint8_t status;
};

struct RawSignUpRequest {
    uint16_t cmdId;
    uint64_t gatewayId;
};

struct RawSignUpResponse {
    uint16_t cmdId;
    uint64_t clientId;
    uint8_t status;
};

}  // namespace Protocon
