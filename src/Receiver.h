#pragma once

#include <sockpp/stream_socket.h>

#include <atomic>

#include "RawRequest.h"
#include "RawResponse.h"
#include "RawSignInResponse.h"
#include "RawSignUpResponse.h"
#include "ThreadSafeQueue.h"
#include "Util.h"

namespace Protocon {

class Receiver {
  public:
    Receiver(sockpp::stream_socket&& socket,
             ThreadSafeQueue<RawRequest>& requestTx,
             ThreadSafeQueue<RawResponse>& responseTx,
             ThreadSafeQueue<RawSignUpResponse>& signUpResponseTx,
             ThreadSafeQueue<RawSignInResponse>& signInResponseTx)
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

        mRequestTx.emplace(RawRequest{
            .cmdId = cmdId,
            .gatewayId = gatewayId,
            .clientId = clientId,
            .apiVersion = apiVersion,
            .request = Request{
                .time = time,
                .type = type,
                .data = std::string(mBuf.data(), length),
            }});

        return true;
    }

    inline bool receiveResponse(uint16_t cmdId) {
        // 响应。
        uint64_t time;
        if (!~mSocket.read_n(&time, sizeof(time))) return false;
        time = Util::BigEndian(time);

        uint8_t status;
        if (!~mSocket.read_n(&status, sizeof(status))) return false;

        uint32_t length;
        if (!~mSocket.read_n(&length, sizeof(length))) return false;
        length = Util::BigEndian(length);

        if (!~mSocket.read_n(&mBuf, length)) return false;

        mResponseTx.emplace(RawResponse{
            .cmdId = cmdId,
            .response = Response{
                .time = time,
                .status = status,
                .data = std::string(mBuf.data(), length),
            }});

        return true;
    }

    inline bool receiveSignUpResponse(uint16_t cmdId) {
        // 注册响应

        uint64_t clientId;
        if (!read(&clientId, sizeof(clientId))) return false;
        clientId = Util::BigEndian(clientId);

        uint8_t status;
        if (!~mSocket.read_n(&status, sizeof(status))) return false;

        mSignUpResponseTx.emplace(RawSignUpResponse{
            .cmdId = cmdId,
            .clientId = clientId,
            .status = status});

        return true;
    }

    inline bool receiveSignInResponse(uint16_t cmdId) {
        // 登录响应

        uint8_t status;
        if (!~mSocket.read_n(&status, sizeof(status))) return false;

        mSignInResponseTx.emplace(RawSignInResponse{
            .cmdId = cmdId,
            .status = status});

        return true;
    }

    sockpp::stream_socket mSocket;
    ThreadSafeQueue<RawRequest>& mRequestTx;
    ThreadSafeQueue<RawResponse>& mResponseTx;
    ThreadSafeQueue<RawSignUpResponse>& mSignUpResponseTx;
    ThreadSafeQueue<RawSignInResponse>& mSignInResponseTx;

    std::array<char, 1024> mBuf;

    std::atomic_bool mStopFlag;

    std::thread mHandle;
};

}  // namespace Protocon
