#include <Protocon/Protocon.h>
#include <sockpp/tcp_connector.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <thread>

#include "Decoder.h"
#include "Encoder.h"
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

                if (!Encoder(socket).encode(cmdId, gatewayId, apiVersion, r)) break;
            }

            if (!(*responseQueue)->empty()) {
                auto [cmdId, r] = (*responseQueue)->pop();

                if (!Encoder(socket).encode(cmdId, r)) break;
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
