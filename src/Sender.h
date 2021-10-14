#pragma once

#include <sockpp/tcp_connector.h>

#include <atomic>
#include <string>
#include <utility>

#include "RawRequest.h"
#include "RawResponse.h"
#include "RawSignInRequest.h"
#include "RawSignUpRequest.h"
#include "ThreadSafeQueue.h"

namespace Protocon {

class Sender {
  public:
    Sender(sockpp::stream_socket&& socket,
           ThreadSafeQueue<RawRequest>& requestRx,
           ThreadSafeQueue<RawResponse>& responseRx,
           ThreadSafeQueue<RawSignUpRequest>& signUpRequestRx,
           ThreadSafeQueue<RawSignInRequest>& signInRequestRx)
        : mSocket(std::move(socket)),
          mRequestRx(requestRx),
          mResponseRx(responseRx),
          mSignUpRequestRx(signUpRequestRx),
          mSignInRequestRx(signInRequestRx) {}

    void run() {
        mStopFlag = false;

        mHandle = std::thread([this]() {
            while (mSocket.is_open() && !mStopFlag) {
                if (!mRequestRx.empty())
                    if (!send(mRequestRx.pop())) break;

                if (!mResponseRx.empty())
                    if (!send(mResponseRx.pop())) break;

                if (!mSignUpRequestRx.empty())
                    if (!send(mSignUpRequestRx.pop())) break;

                if (!mSignInRequestRx.empty())
                    if (!send(mSignInRequestRx.pop())) break;

                std::this_thread::sleep_for(std::chrono::milliseconds(400));
            }

            if (mStopFlag)
                std::printf("Writer closed by shutdown\n");
            else
                std::printf("Writer closed, details: %s\n", mSocket.last_error_str().c_str());
        });
    }

    void stop() {
        mStopFlag = true;
        mHandle.join();
    }

  private:
    inline bool write(const void* buf, size_t n) {
        auto res = mSocket.write_n(buf, n);
        return res && ~res;
    }

    inline bool send(const RawRequest& rawRequest) {
        const Request& r = rawRequest.request;

        uint8_t cmdFlag = 0x00;
        if (!write(&cmdFlag, sizeof(cmdFlag))) return false;

        uint16_t cmdId = Util::BigEndian(rawRequest.cmdId);
        if (!write(&cmdId, sizeof(cmdId))) return false;

        uint64_t gatewayId = Util::BigEndian(gatewayId);
        if (!write(&gatewayId, sizeof(gatewayId))) return false;

        uint64_t clientId = Util::BigEndian(rawRequest.clientId);
        if (!write(&clientId, sizeof(clientId))) return false;

        uint64_t time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        time = Util::BigEndian(time);
        if (!write(&time, sizeof(time))) return false;

        uint16_t apiVersion = Util::BigEndian(rawRequest.apiVersion);
        if (!write(&apiVersion, sizeof(apiVersion))) return false;

        uint16_t type = Util::BigEndian(r.type);
        if (!write(&type, sizeof(type))) return false;

        uint32_t length = r.data.length();
        length = Util::BigEndian(length);
        if (!write(&length, sizeof(length))) return false;
        length = Util::BigEndian(length);

        if (!write(r.data.data(), length)) return false;

        return true;
    }

    inline bool send(const RawResponse& rawResponse) {
        Response r = rawResponse.response;

        uint8_t cmdFlag = 0x80;
        if (!write(&cmdFlag, sizeof(cmdFlag))) return false;

        uint16_t cmdId = Util::BigEndian(rawResponse.cmdId);
        if (!~mSocket.write_n(&cmdId, sizeof(cmdId))) return false;

        uint64_t time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (!~mSocket.write_n(&time, sizeof(time))) return false;

        if (!~mSocket.write_n(&r.status, sizeof(r.status))) return false;

        uint32_t length = r.data.length();
        length = Util::BigEndian(length);
        if (!~mSocket.write_n(&length, sizeof(length))) return false;
        length = Util::BigEndian(length);

        if (!~mSocket.write_n(r.data.data(), length)) return false;

        return true;
    }

    inline bool send(const RawSignUpRequest& r) {
        uint8_t cmdFlag = 0x01;
        if (!write(&cmdFlag, sizeof(cmdFlag))) return false;

        uint16_t cmdId = Util::BigEndian(r.cmdId);
        if (!~mSocket.write_n(&cmdId, sizeof(cmdId))) return false;

        uint64_t gatewayId = Util::BigEndian(r.gatewayId);
        if (!write(&gatewayId, sizeof(gatewayId))) return false;

        return true;
    }

    inline bool send(const RawSignInRequest& r) {
        uint8_t cmdFlag = 0x02;
        if (!write(&cmdFlag, sizeof(cmdFlag))) return false;

        uint16_t cmdId = Util::BigEndian(r.cmdId);
        if (!~mSocket.write_n(&cmdId, sizeof(cmdId))) return false;

        uint64_t gatewayId = Util::BigEndian(r.gatewayId);
        if (!write(&gatewayId, sizeof(gatewayId))) return false;

        uint64_t clientId = Util::BigEndian(r.clientId);
        if (!write(&clientId, sizeof(clientId))) return false;

        return true;
    }

    sockpp::stream_socket mSocket;
    ThreadSafeQueue<RawRequest>& mRequestRx;
    ThreadSafeQueue<RawResponse>& mResponseRx;
    ThreadSafeQueue<RawSignUpRequest>& mSignUpRequestRx;
    ThreadSafeQueue<RawSignInRequest>& mSignInRequestRx;

    std::atomic_bool mStopFlag;

    std::thread mHandle;
};

}  // namespace Protocon
