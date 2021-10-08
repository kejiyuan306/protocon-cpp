#include <Protocon/Protocon.h>
#include <sockpp/tcp_connector.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <thread>

#include "ThreadSafeQueue.h"
#include "ThreadSafeUnorderedMap.h"

namespace Protocon {

void Client::run(const char* host, uint16_t port) {
    auto conn = std::make_unique<sockpp::tcp_connector>();
    if (!conn->connect(sockpp::inet_address(host, port)))
        printf("Connect failed, details: %s\n", conn->last_error_str().c_str());
    mSocket = std::move(conn);

    mSentRequestQueue = std::make_unique<ThreadSafeQueue<SentRequest>>();
    mSentRequestTypeMap = std::make_unique<ThreadSafeUnorderedMap<uint16_t, uint16_t>>();

    mReaderHandle = std::thread([socket = mSocket->clone(),
                                 requestHandlerMap = &mRequestHandlerMap,
                                 responseHandlerMap = &mResponseHandlerMap,
                                 requestTypeMap = &mSentRequestTypeMap]() mutable {
        std::array<char8_t, 1024> buf;

        while (true) {
            uint16_t cmdId;
            if (!~socket.read_n(&cmdId, sizeof(cmdId))) break;

            // TODO: 处理字节序问题。
            if (cmdId > 0) {
                // 请求。

                uint64_t gatewayId;
                if (!~socket.read_n(&gatewayId, sizeof(gatewayId))) break;

                uint64_t clientId;
                if (!~socket.read_n(&clientId, sizeof(clientId))) break;

                uint64_t time;
                if (!~socket.read_n(&time, sizeof(time))) break;

                uint16_t apiVersion;
                if (!~socket.read_n(&apiVersion, sizeof(apiVersion))) break;

                uint16_t type;
                if (!~socket.read_n(&type, sizeof(type))) break;

                uint32_t length;
                if (!~socket.read_n(&length, sizeof(length))) break;

                if (!~socket.read_n(&buf, length)) break;

                requestHandlerMap->at(type)->handle(ReceivedRequest{
                    .commandId = cmdId,
                    .gatewayId = gatewayId,
                    .clientId = clientId,
                    .time = time,
                    .apiVersion = apiVersion,
                    .type = type,
                    .data = std::u8string(buf.data(), length),
                });
            } else if (cmdId < 0) {
                // 响应。

                cmdId = -cmdId;

                uint64_t time;
                if (!~socket.read_n(&time, sizeof(time))) break;

                uint8_t status;
                if (!~socket.read_n(&status, sizeof(status))) break;

                uint32_t length;
                if (!~socket.read_n(&length, sizeof(length))) break;

                if (!~socket.read_n(&buf, length)) break;

                uint16_t type = (*requestTypeMap)->at(cmdId);

                responseHandlerMap->at(type)->handle(ReceivedResponse{
                    .commandId = cmdId,
                    .time = time,
                    .status = status,
                    .data = std::u8string(buf.data(), length),
                });
            } else {
                printf("Fatal error, please contact to developers.");
                return;
            }
        }
        printf("Disconnected, details: %s\n", socket.last_error_str().c_str());
    });

    mWriterHandle = std::thread([socket = mSocket->clone(),
                                 requestQueue = &mSentRequestQueue,
                                 requestTypeMap = &mSentRequestTypeMap,
                                 gatewayId = mGatewayId,
                                 apiVersion = mApiVersion]() mutable {
        uint16_t cmdIdCounter = 0;
        const uint16_t maxCmdId = 0x7fff;

        // TODO: 处理字节序问题。
        while (true) {
            if (!(*requestQueue)->empty()) {
                SentRequest r = (*requestQueue)->pop();

                if (cmdIdCounter == maxCmdId)
                    cmdIdCounter = 0;
                cmdIdCounter++;
                const uint16_t cmdId = cmdIdCounter;

                (*requestTypeMap)->emplace(cmdId, r.type);

                if (!~socket.write_n(&cmdId, sizeof(cmdId))) break;

                if (!~socket.write_n(&gatewayId, sizeof(gatewayId))) break;

                if (!~socket.write_n(&r.clientId, sizeof(r.clientId))) break;

                uint64_t time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                if (!~socket.write_n(&time, sizeof(time))) break;

                if (!~socket.write_n(&apiVersion, sizeof(apiVersion))) break;

                if (!~socket.write_n(&r.type, sizeof(r.type))) break;

                uint32_t length = r.data.length();
                if (!~socket.write_n(&length, sizeof(length))) break;

                if (!~socket.write_n(r.data.data(), length)) break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(400));
        }
        printf("Disconnected, details: %s\n", socket.last_error_str().c_str());
    });
}

void Client::stop() {
    mSocket->shutdown();

    mReaderHandle.join();
    mWriterHandle.join();
}

void Client::send(SentRequest&& r) {
    mSentRequestQueue->emplace(std::move(r));
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

Client::~Client() = default;

}  // namespace Protocon
