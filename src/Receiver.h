#pragma once

#include <sockpp/stream_socket.h>

#include <atomic>

#include "ThreadSafeQueue.h"
#include "Util.h"

namespace Protocon {

class Receiver {
  public:
    Receiver(std::atomic_bool& stopFlag, sockpp::stream_socket&& socket,
             ThreadSafeQueue<ReceivedRequest>& requestTx,
             ThreadSafeQueue<ReceivedResponse>& responseTx)
        : mStopFlag(stopFlag), mSocket(std::move(socket)), mRequestTx(requestTx), mResponseTx(responseTx) {}

    void run() {
        mHandle = std::thread([this] {
            std::array<char8_t, 1024> buf;
            while (!mStopFlag) {
                uint16_t cmdId;
                if (!read(&cmdId, sizeof(cmdId))) break;
                cmdId = Util::BigEndian(cmdId);

                if ((cmdId & 0x8000) == 0 && cmdId != 0) {
                    // 请求。
                    ReceivedRequest request;

                    uint64_t gatewayId;
                    if (!read(&gatewayId, sizeof(gatewayId))) break;
                    gatewayId = Util::BigEndian(gatewayId);

                    uint64_t clientId;
                    if (!read(&clientId, sizeof(clientId))) break;
                    clientId = Util::BigEndian(clientId);

                    uint64_t time;
                    if (!read(&time, sizeof(time))) break;
                    time = Util::BigEndian(time);

                    uint16_t apiVersion;
                    if (!read(&apiVersion, sizeof(apiVersion))) break;
                    apiVersion = Util::BigEndian(apiVersion);

                    uint16_t type;
                    if (!read(&type, sizeof(type))) break;
                    type = Util::BigEndian(type);

                    uint32_t length;
                    if (!read(&length, sizeof(length))) break;
                    length = Util::BigEndian(length);

                    if (!read(&buf, length)) break;

                    request = ReceivedRequest{
                        .commandId = cmdId,
                        .gatewayId = gatewayId,
                        .clientId = clientId,
                        .time = time,
                        .apiVersion = apiVersion,
                        .type = type,
                        .data = std::u8string(buf.data(), length),
                    };
                    mRequestTx.emplace(std::move(request));

                } else if ((cmdId & 0x8000) == 0x8000 && cmdId != 0x8000) {
                    // 响应。
                    ReceivedResponse response;

                    cmdId ^= 0x8000;

                    uint64_t time;
                    if (!~mSocket.read_n(&time, sizeof(time))) break;
                    time = Util::BigEndian(time);

                    uint8_t status;
                    if (!~mSocket.read_n(&status, sizeof(status))) break;

                    uint32_t length;
                    if (!~mSocket.read_n(&length, sizeof(length))) break;
                    length = Util::BigEndian(length);

                    if (!~mSocket.read_n(&buf, length)) break;

                    response = ReceivedResponse{
                        .commandId = cmdId,
                        .time = time,
                        .status = status,
                        .data = std::u8string(buf.data(), length),
                    };
                    mResponseTx.emplace(std::move(response));

                } else {
                    std::printf("Fatal error, please contact to developers.\n");
                    break;
                }
            }

            if (mStopFlag)
                std::printf("Reader closed by shutdown\n");
            else
                std::printf("Reader closed, details: %s\n", mSocket.last_error_str().c_str());
        });
    }

    void stop() {
        mHandle.join();
    }

  private:
    std::atomic_bool& mStopFlag;
    sockpp::stream_socket mSocket;
    ThreadSafeQueue<ReceivedRequest>& mRequestTx;
    ThreadSafeQueue<ReceivedResponse>& mResponseTx;

    std::thread mHandle;

    inline bool read(void* buf, size_t n) {
        auto res = mSocket.read_n(buf, n);
        return res && ~res;
    }
};

}  // namespace Protocon
