#include <Protocon/Protocon.h>
#include <sockpp/tcp_connector.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <thread>

#include "Receiver.h"
#include "Sender.h"
#include "ThreadSafeQueue.h"
#include "ThreadSafeUnorderedMap.h"
#include "Util.h"

namespace Protocon {

Gateway::~Gateway() = default;

bool Gateway::isOpen() const {
    return mSocket->is_open();
}

bool Gateway::run(const char* host, uint16_t port) {
    auto conn = std::make_unique<sockpp::tcp_connector>();
    if (!conn->connect(sockpp::inet_address(host, port))) {
        std::printf("Connect failed, details: %s\n", conn->last_error_str().c_str());
        return false;
    }
    mSocket = std::move(conn);

    mReceivedRequestQueue = std::make_unique<ThreadSafeQueue<RawRequest>>();
    mReceivedResponseQueue = std::make_unique<ThreadSafeQueue<RawResponse>>();

    mSentRequestQueue = std::make_unique<ThreadSafeQueue<std::pair<uint16_t, Request>>>();
    mSentResponseQueue = std::make_unique<ThreadSafeQueue<std::pair<uint16_t, Response>>>();

    mReceiver = std::make_unique<Receiver>(
        mSocket->clone(), *mReceivedRequestQueue, *mReceivedResponseQueue,
        *mReceivedSignUpResponseQueue, *mReceivedSignInResponseQueue);
    mReceiver->run();

    mSender = std::make_unique<Sender>(
        mApiVersion, mGatewayId,
        mSocket->clone(), *mSentRequestQueue, *mSentResponseQueue);
    mSender->run();

    return true;
}

void Gateway::stop() {
    mSocket->shutdown();

    mReceiver->stop();
    mSender->stop();
}

void Gateway::poll() {
    while (!mReceivedRequestQueue->empty()) {
        RawRequest r = mReceivedRequestQueue->pop();

        mSentResponseQueue->emplace(std::make_pair(r.cmdId, mRequestHandlerMap.at(r.request.type)(r.request)));
    }

    while (!mReceivedResponseQueue->empty()) {
        RawResponse r = mReceivedResponseQueue->pop();

        auto it = mSentRequestResponseHandlerMap.find(r.cmdId);
        mSentRequestResponseHandlerMap.erase(it)->second(r.response);
    }
}

void Gateway::send(Request&& r, ResponseHandler&& handler) {
    const uint16_t cmdId = mCmdIdCounter++;
    if (mCmdIdCounter > cMaxCmdId)
        mCmdIdCounter = 1;

    mSentRequestResponseHandlerMap.emplace(cmdId, std::move(handler));
    mSentRequestQueue->emplace(std::make_pair(cmdId, std::move(r)));
}

Gateway::Gateway(uint16_t apiVersion, uint64_t gatewayId,
                 std::vector<std::pair<uint16_t, RequestHandler>>&& requestHandlers)
    : mApiVersion(apiVersion), mGatewayId(gatewayId) {
    // TODO: 处理注册登录
    for (auto&& h : requestHandlers)
        mRequestHandlerMap.emplace(h.first, std::move(h.second));
}

}  // namespace Protocon
