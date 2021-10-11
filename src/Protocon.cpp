#include <Protocon/Protocon.h>
#include <sockpp/tcp_connector.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <thread>

#include "Decoder.h"
#include "ThreadSafeQueue.h"
#include "ThreadSafeUnorderedMap.h"
#include "Util.h"

namespace Protocon {

Client::~Client() = default;

bool Client::isOpen() const {
    return mSocket->is_open();
}

bool Client::run(const char* host, uint16_t port) {
    mStopFlag = false;

    auto conn = std::make_unique<sockpp::tcp_connector>();
    if (!conn->connect(sockpp::inet_address(host, port))) {
        std::printf("Connect failed, details: %s\n", conn->last_error_str().c_str());
        return false;
    }
    mSocket = std::move(conn);

    mReceivedRequestQueue = std::make_unique<ThreadSafeQueue<ReceivedRequest>>();
    mReceivedResponseQueue = std::make_unique<ThreadSafeQueue<ReceivedResponse>>();

    mSentRequestQueue = std::make_unique<ThreadSafeQueue<std::pair<uint16_t, SentRequest>>>();
    mSentResponseQueue = std::make_unique<ThreadSafeQueue<std::pair<uint16_t, SentResponse>>>();

    mReaderHandle = std::thread([stopFlag = &mStopFlag,
                                 socket = mSocket->clone(),
                                 requestQueue = &mReceivedRequestQueue,
                                 responseQueue = &mReceivedResponseQueue]() mutable {
        while (!*stopFlag) {
            ReceivedRequest request;
            ReceivedResponse response;
            auto res = Decoder(socket).decode(request, response);
            if (res == Request)
                (*requestQueue)->emplace(std::move(request));
            else if (res == Response)
                (*responseQueue)->emplace(std::move(response));
            else
                break;
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
                                 gatewayId = mGatewayId,
                                 apiVersion = mApiVersion]() mutable {
        while (socket.is_open() && !*stopFlag) {
            if (!(*requestQueue)->empty()) {
                auto [cmdId, r] = (*requestQueue)->pop();

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
            std::printf("Writer closed, details: %s\n", socket.last_error_str().c_str());
    });

    return true;
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

        mSentResponseQueue->emplace(std::make_pair(r.commandId, mRequestHandlerMap.at(r.type)(r)));
    }

    while (!mReceivedResponseQueue->empty()) {
        ReceivedResponse r = mReceivedResponseQueue->pop();

        auto it = mSentRequestResponseHandlerMap.find(r.commandId);
        mSentRequestResponseHandlerMap.erase(it)->second(r);
    }
}

void Client::send(SentRequest&& r, ResponseHandler&& handler) {
    const uint16_t cmdId = mCmdIdCounter++;
    if (mCmdIdCounter > cMaxCmdId)
        mCmdIdCounter = 1;

    mSentRequestResponseHandlerMap.emplace(cmdId, std::move(handler));
    mSentRequestQueue->emplace(std::make_pair(cmdId, std::move(r)));
}

Client::Client(uint16_t apiVersion, uint64_t gatewayId,
               std::vector<std::pair<uint16_t, RequestHandler>>&& requestHandlers)
    : mApiVersion(apiVersion), mGatewayId(gatewayId) {
    for (auto&& h : requestHandlers)
        mRequestHandlerMap.emplace(h.first, std::move(h.second));
}

}  // namespace Protocon
