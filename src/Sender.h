#pragma once

#include <sockpp/tcp_connector.h>

#include <atomic>
#include <string>
#include <utility>

#include "ThreadSafeQueue.h"

namespace Protocon {

template <typename T>
struct CommandWrapper {
    uint16_t cmdId;
    T value;
};

class Sender {
  public:
    Sender(uint16_t apiVersion, uint64_t gatewayId,
           sockpp::stream_socket&& socket,
           ThreadSafeQueue<CommandWrapper<Request>>& requestRx,
           ThreadSafeQueue<CommandWrapper<Response>>& responseRx,
           ThreadSafeQueue<RawSignUpRequest>& signUpRequestRx,
           ThreadSafeQueue<RawSignInRequest>& signInRequestRx)
        : mApiVersion(apiVersion),
          mGatewayId(gatewayId),
          mSocket(std::move(socket)),
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
    uint16_t mApiVersion;
    uint64_t mGatewayId;
    sockpp::stream_socket mSocket;
    ThreadSafeQueue<CommandWrapper<Request>>& mRequestRx;
    ThreadSafeQueue<CommandWrapper<Response>>& mResponseRx;
    ThreadSafeQueue<RawSignUpRequest>& mSignUpRequestRx;
    ThreadSafeQueue<RawSignInRequest>& mSignInRequestRx;

    std::atomic_bool mStopFlag;

    std::thread mHandle;

    inline bool write(const void* buf, size_t n) {
        auto res = mSocket.write_n(buf, n);
        return res && ~res;
    }

    inline bool send(const CommandWrapper<Request>& wrapper) {
        const Request& r = wrapper.value;

        uint8_t cmdFlag = 0x00;
        if (!write(&cmdFlag, sizeof(cmdFlag))) return false;

        uint16_t cmdId = Util::BigEndian(wrapper.cmdId);
        if (!write(&cmdId, sizeof(cmdId))) return false;

        uint64_t gatewayId = Util::BigEndian(gatewayId);
        if (!write(&gatewayId, sizeof(gatewayId))) return false;

        uint64_t clientId = Util::BigEndian(r.clientId);
        if (!write(&clientId, sizeof(clientId))) return false;

        uint64_t time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        time = Util::BigEndian(time);
        if (!write(&time, sizeof(time))) return false;

        uint16_t apiVersion = Util::BigEndian(mApiVersion);
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

    inline bool send(const CommandWrapper<Response>& wrapper) {
        Response r = wrapper.value;

        uint8_t cmdFlag = 0x80;
        if (!write(&cmdFlag, sizeof(cmdFlag))) return false;

        uint16_t cmdId = Util::BigEndian(wrapper.cmdId);
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

        uint64_t gatewayId = Util::BigEndian(mGatewayId);
        if (!write(&gatewayId, sizeof(gatewayId))) return false;

        return true;
    }

    inline bool send(const RawSignInRequest& r) {
        uint8_t cmdFlag = 0x02;
        if (!write(&cmdFlag, sizeof(cmdFlag))) return false;

        uint16_t cmdId = Util::BigEndian(r.cmdId);
        if (!~mSocket.write_n(&cmdId, sizeof(cmdId))) return false;

        uint64_t gatewayId = Util::BigEndian(mGatewayId);
        if (!write(&gatewayId, sizeof(gatewayId))) return false;

        uint64_t clientId = Util::BigEndian(r.clientId);
        if (!write(&clientId, sizeof(clientId))) return false;

        return true;
    }
};

}  // namespace Protocon
