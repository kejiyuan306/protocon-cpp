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

    mSentRequestQueue = std::make_unique<ThreadSafeQueue<RawRequest>>();
    mSentResponseQueue = std::make_unique<ThreadSafeQueue<RawResponse>>();

    mReceiver = std::make_unique<Receiver>(
        mSocket->clone(), *mReceivedRequestQueue, *mReceivedResponseQueue,
        *mReceivedSignUpResponseQueue, *mReceivedSignInResponseQueue);
    mReceiver->run();

    mSender = std::make_unique<Sender>(
        mSocket->clone(),
        *mSentRequestQueue, *mSentResponseQueue,
        *mSentSignUpRequestQueue, *mSentSignInRequestQueue);
    mSender->run();

    // 发送登录请求
    for (const auto& it : mClientIdTokenMap)
        sendSignInRequest(it.first);

    // 发送注册请求
    for (const auto& tk : mAnonymousTokens)
        sendSignUpRequest();
    mAnonymousTokens.clear();

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

        auto clientIdIt = mClientIdTokenMap.find(r.clientId);
        auto handlerIt = mRequestHandlerMap.find(r.request.type);
        if (clientIdIt != mClientIdTokenMap.end() && handlerIt != mRequestHandlerMap.end())
            mSentResponseQueue->emplace(
                RawResponse{
                    r.cmdId,
                    (handlerIt->second)(ClientToken{clientIdIt->second}, r.request)});
    }

    while (!mReceivedResponseQueue->empty()) {
        RawResponse r = mReceivedResponseQueue->pop();

        auto it = mSentRequestResponseHandlerMap.find(r.cmdId);
        mSentRequestResponseHandlerMap.erase(it)->second(r.response);
    }

    while (!mReceivedSignUpResponseQueue->empty()) {
        RawSignUpResponse r = mReceivedSignUpResponseQueue->pop();
    }
}

void Gateway::send(ClientToken tk, Request&& r, ResponseHandler&& handler) {
    const uint16_t cmdId = nextCmdId();

    uint64_t clientId = mTokenClientIdMap.at(tk);

    mSentRequestResponseHandlerMap.emplace(cmdId, std::move(handler));
    mSentRequestQueue->emplace(RawRequest{cmdId, mGatewayId, clientId, mApiVersion, std::move(r)});
}

Gateway::Gateway(uint16_t apiVersion, uint64_t gatewayId,
                 std::vector<std::pair<uint16_t, RequestHandler>>&& requestHandlers)
    : mApiVersion(apiVersion), mGatewayId(gatewayId) {
    for (auto&& h : requestHandlers)
        mRequestHandlerMap.emplace(h.first, std::move(h.second));
}

void Gateway::sendSignUpRequest() {
    uint16_t cmdId = nextCmdId();
    mSentSignUpRequestQueue->emplace(RawSignUpRequest{cmdId, mGatewayId});
}

void Gateway::sendSignInRequest(uint64_t clientId) {
    uint16_t cmdId = nextCmdId();
    mSentSignInRequestQueue->emplace(RawSignInRequest{cmdId, mGatewayId, clientId});
}

}  // namespace Protocon
