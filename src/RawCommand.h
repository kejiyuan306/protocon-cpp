#pragma once

#include <Protocon/Request.h>
#include <Protocon/Response.h>
#include <Protocon/SignInResponse.h>
#include <Protocon/SignUpResponse.h>

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
    SignInResponse response;
};

struct RawSignUpRequest {
    uint16_t cmdId;
    uint64_t gatewayId;
};

struct RawSignUpResponse {
    uint16_t cmdId;
    SignUpResponse response;
};

}  // namespace Protocon
