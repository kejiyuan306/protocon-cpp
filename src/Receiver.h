#pragma once

#include <sockpp/stream_socket.h>

#include <atomic>

#include "ThreadSafeQueue.h"
#include "Util.h"

namespace Protocon {

class Receiver {
  public:
    Receiver(sockpp::stream_socket&& socket,
             ThreadSafeQueue<ReceivedRequest>& requestTx,
             ThreadSafeQueue<ReceivedResponse>& responseTx,
             ThreadSafeQueue<ReceivedSignUpResponse>& signUpResponseTx,
             ThreadSafeQueue<ReceivedSignInResponse>& signInResponseTx)
        : mSocket(std::move(socket)),
          mRequestTx(requestTx),
          mResponseTx(responseTx),
          mSignUpResponseTx(signUpResponseTx),
          mSignInResponseTx(signInResponseTx) {}

    void run() {
        mStopFlag = false;

        mHandle = std::thread([this] {
            while (!mStopFlag) {
                uint8_t cmdFlag;
                if (!read(&cmdFlag, sizeof(cmdFlag))) break;

                uint16_t cmdId;
                if (!read(&cmdId, sizeof(cmdId))) break;
                cmdId = Util::BigEndian(cmdId);

                if (cmdFlag == 0x00) {
                    if (!receiveRequest(cmdId)) break;
                } else if (cmdFlag == 0x80) {
                    if (!receiveResponse(cmdId)) break;
                } else if (cmdFlag == 0x81) {
                    if (!receiveSignUpResponse(cmdId)) break;
                } else if (cmdFlag == 0x82) {
                    if (!receiveSignInResponse(cmdId)) break;
                } else {
                    std::printf("致命错误，收到了不合法的命令标记，请联系开发者\n");
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
        mStopFlag = true;
        mHandle.join();
    }

  private:
    sockpp::stream_socket mSocket;
    ThreadSafeQueue<ReceivedRequest>& mRequestTx;
    ThreadSafeQueue<ReceivedResponse>& mResponseTx;
    ThreadSafeQueue<ReceivedSignUpResponse>& mSignUpResponseTx;
    ThreadSafeQueue<ReceivedSignInResponse>& mSignInResponseTx;

    std::array<char8_t, 1024> mBuf;

    std::atomic_bool mStopFlag;

    std::thread mHandle;

    inline bool read(void* buf, size_t n) {
        auto res = mSocket.read_n(buf, n);
        return res && ~res;
    }

    inline bool receiveRequest(uint16_t cmdId) {
        // 请求。
        uint64_t gatewayId;
        if (!read(&gatewayId, sizeof(gatewayId))) return false;
        gatewayId = Util::BigEndian(gatewayId);

        uint64_t clientId;
        if (!read(&clientId, sizeof(clientId))) return false;
        clientId = Util::BigEndian(clientId);

        uint64_t time;
        if (!read(&time, sizeof(time))) return false;
        time = Util::BigEndian(time);

        uint16_t apiVersion;
        if (!read(&apiVersion, sizeof(apiVersion))) return false;
        apiVersion = Util::BigEndian(apiVersion);

        uint16_t type;
        if (!read(&type, sizeof(type))) return false;
        type = Util::BigEndian(type);

        uint32_t length;
        if (!read(&length, sizeof(length))) return false;
        length = Util::BigEndian(length);

        if (!read(&mBuf, length)) return false;

        mRequestTx.emplace(ReceivedRequest{
            .commandId = cmdId,
            .gatewayId = gatewayId,
            .clientId = clientId,
            .time = time,
            .apiVersion = apiVersion,
            .type = type,
            .data = std::u8string(mBuf.data(), length),
        });

        return true;
    }

    inline bool receiveResponse(uint16_t cmdId) {
        // 响应。
        ReceivedResponse response;

        uint64_t time;
        if (!~mSocket.read_n(&time, sizeof(time))) return false;
        time = Util::BigEndian(time);

        uint8_t status;
        if (!~mSocket.read_n(&status, sizeof(status))) return false;

        uint32_t length;
        if (!~mSocket.read_n(&length, sizeof(length))) return false;
        length = Util::BigEndian(length);

        if (!~mSocket.read_n(&mBuf, length)) return false;

        response = ReceivedResponse{
            .commandId = cmdId,
            .time = time,
            .status = status,
            .data = std::u8string(mBuf.data(), length),
        };
        mResponseTx.emplace(std::move(response));

        return true;
    }

    inline bool receiveSignUpResponse(uint16_t cmdId) {
        // 注册响应

        uint64_t clientId;
        if (!read(&clientId, sizeof(clientId))) return false;
        clientId = Util::BigEndian(clientId);

        uint8_t status;
        if (!~mSocket.read_n(&status, sizeof(status))) return false;

        mSignUpResponseTx.emplace(ReceivedSignUpResponse{
            .commandId = cmdId,
            .clientId = clientId,
            .status = status});

        return true;
    }

    inline bool receiveSignInResponse(uint16_t cmdId) {
        // 登录响应

        uint8_t status;
        if (!~mSocket.read_n(&status, sizeof(status))) return false;

        mSignInResponseTx.emplace(ReceivedSignInResponse{
            .commandId = cmdId,
            .status = status});

        return true;
    }
};

}  // namespace Protocon
