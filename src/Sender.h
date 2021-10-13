#pragma once

#include <sockpp/tcp_connector.h>

#include <atomic>
#include <string>
#include <utility>

#include "ThreadSafeQueue.h"

namespace Protocon {

class Sender {
  public:
    Sender(uint16_t apiVersion, uint64_t gatewayId,
           sockpp::stream_socket&& socket,
           ThreadSafeQueue<std::pair<uint16_t, Request>>& requestRx,
           ThreadSafeQueue<std::pair<uint16_t, Response>>& responseRx)
        : mApiVersion(apiVersion),
          mGatewayId(gatewayId),
          mSocket(std::move(socket)),
          mRequestRx(requestRx),
          mResponseRx(responseRx) {}

    void run() {
        mStopFlag = false;

        mHandle = std::thread([this]() {
            while (mSocket.is_open() && !mStopFlag) {
                if (!mRequestRx.empty()) {
                    auto [cmdId, r] = mRequestRx.pop();

                    if (!send(cmdId, mGatewayId, mApiVersion, r)) break;
                }

                if (!mResponseRx.empty()) {
                    auto [cmdId, r] = mResponseRx.pop();

                    if (!send(cmdId, r)) break;
                }

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
    ThreadSafeQueue<std::pair<uint16_t, Request>>& mRequestRx;
    ThreadSafeQueue<std::pair<uint16_t, Response>>& mResponseRx;

    std::atomic_bool mStopFlag;

    std::thread mHandle;

    inline bool write(const void* buf, size_t n) {
        auto res = mSocket.write_n(buf, n);
        return res && ~res;
    }

    inline bool send(uint16_t cmdId, uint64_t gatewayId, uint16_t apiVersion, const Request& r) {
        cmdId = Util::BigEndian(cmdId);
        if (!write(&cmdId, sizeof(cmdId))) return false;

        gatewayId = Util::BigEndian(gatewayId);
        if (!write(&gatewayId, sizeof(gatewayId))) return false;

        uint64_t clientId = Util::BigEndian(r.clientId);
        if (!write(&clientId, sizeof(clientId))) return false;

        uint64_t time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        time = Util::BigEndian(time);
        if (!write(&time, sizeof(time))) return false;

        apiVersion = Util::BigEndian(apiVersion);
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

    inline bool send(uint16_t cmdId, const Response& r) {
        if (!~mSocket.write_n(&cmdId, sizeof(cmdId))) return false;

        uint64_t time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (!~mSocket.write_n(&time, sizeof(time))) return false;

        if (!~mSocket.write_n(&r.status, sizeof(r.status))) return false;

        uint32_t length = r.data.length();
        if (!~mSocket.write_n(&length, sizeof(length))) return false;

        if (!~mSocket.write_n(r.data.data(), length)) return false;

        return true;
    }
};

}  // namespace Protocon
