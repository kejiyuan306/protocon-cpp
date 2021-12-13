#pragma once

#include <spdlog/spdlog.h>

#include <asio/buffer.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/write.hpp>
#include <atomic>
#include <cstddef>
#include <exception>
#include <string>
#include <thread>
#include <utility>

#include "RawCommand.h"
#include "ThreadSafeQueue.h"
#include "Util.h"

namespace Protocon {

class Sender {
  public:
    Sender(asio::ip::tcp::socket& socket,
           ThreadSafeQueue<RawRequest>& requestRx,
           ThreadSafeQueue<RawResponse>& responseRx,
           ThreadSafeQueue<RawSignUpRequest>& signUpRequestRx,
           ThreadSafeQueue<RawSignInRequest>& signInRequestRx)
        : mSocket(socket),
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
                spdlog::info("Writer closed by shutdown");
            else
                spdlog::warn("Writer closed by error");
        });
    }

    void stop() {
        mStopFlag = true;
        mHandle.join();
    }

  private:
    inline bool write(const void* buf, size_t n) {
        std::size_t len;
        try {
            len = asio::write(mSocket, asio::buffer(buf, n));
        } catch (std::exception& e) {
            spdlog::warn("Writer error occurs, details: %s", e.what());
            return false;
        }
        return len;
    }

    inline bool send(const RawRequest& rawRequest) {
        const Request& r = rawRequest.request;

        uint8_t cmdFlag = 0x00;
        if (!write(&cmdFlag, sizeof(cmdFlag))) return false;

        uint16_t cmdId = Util::BigEndian(rawRequest.cmdId);
        if (!write(&cmdId, sizeof(cmdId))) return false;

        uint64_t gatewayId = Util::BigEndian(rawRequest.gatewayId);
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

        uint32_t length = static_cast<uint32_t>(r.data.length());
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
        if (!write(&cmdId, sizeof(cmdId))) return false;

        uint64_t time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (!write(&time, sizeof(time))) return false;

        if (!write(&r.status, sizeof(r.status))) return false;

        uint32_t length = static_cast<uint32_t>(r.data.length());
        length = Util::BigEndian(length);
        if (!write(&length, sizeof(length))) return false;
        length = Util::BigEndian(length);

        if (!write(r.data.data(), length)) return false;

        return true;
    }

    inline bool send(const RawSignUpRequest& r) {
        uint8_t cmdFlag = 0x01;
        if (!write(&cmdFlag, sizeof(cmdFlag))) return false;

        uint16_t cmdId = Util::BigEndian(r.cmdId);
        if (!write(&cmdId, sizeof(cmdId))) return false;

        uint64_t gatewayId = Util::BigEndian(r.gatewayId);
        if (!write(&gatewayId, sizeof(gatewayId))) return false;

        return true;
    }

    inline bool send(const RawSignInRequest& r) {
        uint8_t cmdFlag = 0x02;
        if (!write(&cmdFlag, sizeof(cmdFlag))) return false;

        uint16_t cmdId = Util::BigEndian(r.cmdId);
        if (!write(&cmdId, sizeof(cmdId))) return false;

        uint64_t gatewayId = Util::BigEndian(r.gatewayId);
        if (!write(&gatewayId, sizeof(gatewayId))) return false;

        uint64_t clientId = Util::BigEndian(r.clientId);
        if (!write(&clientId, sizeof(clientId))) return false;

        return true;
    }

    asio::ip::tcp::socket& mSocket;
    ThreadSafeQueue<RawRequest>& mRequestRx;
    ThreadSafeQueue<RawResponse>& mResponseRx;
    ThreadSafeQueue<RawSignUpRequest>& mSignUpRequestRx;
    ThreadSafeQueue<RawSignInRequest>& mSignInRequestRx;

    std::atomic_bool mStopFlag;

    std::thread mHandle;
};

}  // namespace Protocon
