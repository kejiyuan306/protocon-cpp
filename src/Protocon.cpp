#include <Protocon/Protocon.h>
#include <sockpp/tcp_connector.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <thread>

#include "ThreadSafeQueue.h"
#include "ThreadSafeUnorderedMap.h"
#include "Util.h"

namespace Protocon {

Client::~Client() = default;

void Client::run(const char* host, uint16_t port) {
    mStopFlag = false;

    auto conn = std::make_unique<sockpp::tcp_connector>();
    if (!conn->connect(sockpp::inet_address(host, port)))
        std::printf("Connect failed, details: %s\n", conn->last_error_str().c_str());
    mSocket = std::move(conn);

    mReceivedRequestQueue = std::make_unique<ThreadSafeQueue<ReceivedRequest>>();
    mReceivedResponseQueue = std::make_unique<ThreadSafeQueue<ReceivedResponse>>();

    mSentRequestQueue = std::make_unique<ThreadSafeQueue<std::pair<uint16_t, SentRequest>>>();
    mSentResponseQueue = std::make_unique<ThreadSafeQueue<std::pair<uint16_t, SentResponse>>>();
    mSentRequestTypeMap = std::make_unique<ThreadSafeUnorderedMap<uint16_t, uint16_t>>();

    mReaderHandle = std::thread([stopFlag = &mStopFlag,
                                 socket = mSocket->clone(),
                                 requestQueue = &mReceivedRequestQueue,
                                 responseQueue = &mReceivedResponseQueue]() mutable {
        std::array<char8_t, 1024> buf;

        while (!*stopFlag) {
            uint16_t cmdId;
            if (!~socket.read_n(&cmdId, sizeof(cmdId))) break;
            cmdId = Util::BigEndian(cmdId);

            if ((cmdId & 0x8000) == 0 && cmdId != 0) {
                // 请求。

                uint64_t gatewayId;
                if (!~socket.read_n(&gatewayId, sizeof(gatewayId))) break;
                gatewayId = Util::BigEndian(gatewayId);

                uint64_t clientId;
                if (!~socket.read_n(&clientId, sizeof(clientId))) break;
                clientId = Util::BigEndian(clientId);

                uint64_t time;
                if (!~socket.read_n(&time, sizeof(time))) break;
                time = Util::BigEndian(time);

                uint16_t apiVersion;
                if (!~socket.read_n(&apiVersion, sizeof(apiVersion))) break;
                apiVersion = Util::BigEndian(apiVersion);

                uint16_t type;
                if (!~socket.read_n(&type, sizeof(type))) break;
                type = Util::BigEndian(type);

                uint32_t length;
                if (!~socket.read_n(&length, sizeof(length))) break;
                length = Util::BigEndian(length);

                if (!~socket.read_n(&buf, length)) break;

                (*requestQueue)->emplace(ReceivedRequest{
                    .commandId = cmdId,
                    .gatewayId = gatewayId,
                    .clientId = clientId,
                    .time = time,
                    .apiVersion = apiVersion,
                    .type = type,
                    .data = std::u8string(buf.data(), length),
                });
            } else if ((cmdId & 0x8000) == 0x8000 && cmdId != 0x8000) {
                // 响应。

                cmdId ^= 0x8000;

                uint64_t time;
                if (!~socket.read_n(&time, sizeof(time))) break;
                time = Util::BigEndian(time);

                uint8_t status;
                if (!~socket.read_n(&status, sizeof(status))) break;

                uint32_t length;
                if (!~socket.read_n(&length, sizeof(length))) break;
                length = Util::BigEndian(length);

                if (!~socket.read_n(&buf, length)) break;

                (*responseQueue)->emplace(ReceivedResponse{
                    .commandId = cmdId,
                    .time = time,
                    .status = status,
                    .data = std::u8string(buf.data(), length),
                });
            } else {
                std::printf("Fatal error, please contact to developers.\n");
                return;
            }
        }

        if (*stopFlag)
            std::printf("Reader closed by shutdown\n");
        else
            std::printf("Reader closed, details: %s\n", socket.last_error_str().c_str());
    });

    mWriterHandle = std::thread([stopFlag = &mStopFlag,
                                 socket = mSocket->clone(),
                                 requestQueue = &mSentRequestQueue,
                                 responseQueue = &mSentResponseQueue,
                                 requestTypeMap = &mSentRequestTypeMap,
                                 gatewayId = mGatewayId,
                                 apiVersion = mApiVersion]() mutable {
        while (!*stopFlag) {
            if (!(*requestQueue)->empty()) {
                auto [cmdId, r] = (*requestQueue)->pop();

                (*requestTypeMap)->emplace(cmdId, r.type);

                cmdId = Util::BigEndian(cmdId);
                if (!~socket.write_n(&cmdId, sizeof(cmdId))) break;

                gatewayId = Util::BigEndian(gatewayId);
                if (!~socket.write_n(&gatewayId, sizeof(gatewayId))) break;

                r.clientId = Util::BigEndian(r.clientId);
                if (!~socket.write_n(&r.clientId, sizeof(r.clientId))) break;

                uint64_t time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                time = Util::BigEndian(time);
                if (!~socket.write_n(&time, sizeof(time))) break;

                apiVersion = Util::BigEndian(apiVersion);
                if (!~socket.write_n(&apiVersion, sizeof(apiVersion))) break;

                r.type = Util::BigEndian(r.type);
                if (!~socket.write_n(&r.type, sizeof(r.type))) break;

                uint32_t length = r.data.length();
                length = Util::BigEndian(length);
                if (!~socket.write_n(&length, sizeof(length))) break;
                length = Util::BigEndian(length);

                if (!~socket.write_n(r.data.data(), length)) break;
            }

            if (!(*responseQueue)->empty()) {
                auto [cmdId, r] = (*responseQueue)->pop();

                cmdId &= 0x8000;
                if (!~socket.write_n(&cmdId, sizeof(cmdId))) break;

                uint64_t time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                if (!~socket.write_n(&time, sizeof(time))) break;

                if (!~socket.write_n(&r.status, sizeof(r.status))) break;

                uint32_t length = r.data.length();
                if (!~socket.write_n(&length, sizeof(length))) break;

                if (!~socket.write_n(r.data.data(), length)) break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(400));
        }

        if (*stopFlag)
            std::printf("Writer closed by shutdown\n");
        else
            std::printf("Write closed, details: %s\n", socket.last_error_str().c_str());
    });
}

void Client::stop() {
    mStopFlag = true;

    mSocket->shutdown();

    mReaderHandle.join();
    mWriterHandle.join();
}

void Client::poll() {
    while (!mReceivedRequestQueue->empty()) {
        ReceivedRequest r = mReceivedRequestQueue->pop();

        mSentResponseQueue->emplace(std::make_pair(r.commandId, mRequestHandlerMap.at(r.type)->handle(r)));
    }

    while (!mReceivedResponseQueue->empty()) {
        ReceivedResponse r = mReceivedResponseQueue->pop();

        uint16_t type = mSentRequestTypeMap->at(r.commandId);
        mResponseHandlerMap.at(type)->handle(r);
    }
}

void Client::send(SentRequest&& r) {
    const uint16_t cmdId = mCmdIdCounter++;
    if (mCmdIdCounter > cMaxCmdId)
        mCmdIdCounter = 1;

    mSentRequestQueue->emplace(std::make_pair(cmdId, std::move(r)));
}

Client::Client(uint16_t apiVersion, uint64_t gatewayId,
               std::vector<std::unique_ptr<RequestHandler>>&& requestHandlers,
               std::vector<std::unique_ptr<ResponseHandler>>&& responseHandlers)
    : mApiVersion(apiVersion), mGatewayId(gatewayId) {
    for (auto&& h : requestHandlers)
        mRequestHandlerMap.emplace(h->type(), std::move(h));
    for (auto&& h : responseHandlers)
        mResponseHandlerMap.emplace(h->type(), std::move(h));
}

}  // namespace Protocon
