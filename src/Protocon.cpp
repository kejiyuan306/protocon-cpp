#include <Protocon/Protocon.h>
#include <spdlog/spdlog.h>

#include <array>
#include <chrono>
#include <cinttypes>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <thread>

#include "Receiver.h"
#include "Sender.h"
#include "Socket.h"
#include "ThreadSafeQueue.h"
#include "ThreadSafeUnorderedMap.h"
#include "Util.h"

namespace Protocon {

Gateway::Gateway(Gateway&& gateway) = default;

Gateway::~Gateway() {}

bool Gateway::isOpen() const {
    return mSocket->is_open();
}

bool Gateway::run(const char* host, uint16_t port) {
    mSocket = std::make_unique<Socket>();
    if (!mSocket->connect(host, port))
        return false;

    mRequestRx = std::make_unique<ThreadSafeQueue<RawRequest>>();
    mResponseRx = std::make_unique<ThreadSafeQueue<RawResponse>>();

    mRequestTx = std::make_unique<ThreadSafeQueue<RawRequest>>();
    mResponseTx = std::make_unique<ThreadSafeQueue<RawResponse>>();

    mReceiver = std::make_unique<Receiver>(
        mSocket->socket(), *mRequestRx, *mResponseRx,
        *mSignUpResponseRx, *mSignInResponseRx);
    mReceiver->run();

    mSender = std::make_unique<Sender>(
        mSocket->socket(),
        *mRequestTx, *mResponseTx,
        *mSignUpRequestTx, *mSignInRequestTx);
    mSender->run();

    // 发送登录请求
    for (const auto& it : mClientIdTokenMap)
        sendSignInRequest(it.first);

    // 发送注册请求
    for (decltype(mAnonymousTokens)::size_type i = 0; i < mAnonymousTokens.size(); i++)
        sendSignUpRequest();

    return true;
}

void Gateway::stop() {
    mSocket.reset();

    mReceiver->stop();
    mSender->stop();
}

void Gateway::poll() {
    while (!mRequestRx->empty()) {
        RawRequest r = mRequestRx->pop();

        auto clientIdIt = mClientIdTokenMap.find(r.clientId);
        auto handlerIt = mRequestHandlerMap.find(r.request.type);
        if (clientIdIt != mClientIdTokenMap.end() && handlerIt != mRequestHandlerMap.end())
            mResponseTx->emplace(
                RawResponse{
                    r.cmdId,
                    (handlerIt->second)(ClientToken(clientIdIt->second), r.request)});
    }

    while (!mResponseRx->empty()) {
        RawResponse r = mResponseRx->pop();

        auto it = mRequestResponseHandlerMap.find(r.cmdId);
        mRequestResponseHandlerMap.erase(it)->second(r.response);
    }

    while (!mSignUpResponseRx->empty()) {
        RawSignUpResponse r = mSignUpResponseRx->pop();

        if (!r.status) {
            // 注册成功
            auto tk = mAnonymousTokens.back();
            mAnonymousTokens.pop_back();
            mTokenClientIdMap[tk] = r.clientId;
            mClientIdTokenMap.emplace(r.clientId, tk);
            sendSignInRequest(r.clientId);

            spdlog::info("注册成功，client Id：%" PRIu64 "", r.clientId);
        } else {
            spdlog::warn("注册失败，状态码：%02x", r.status);
        }
    }

    while (!mSignInResponseRx->empty()) {
        RawSignInResponse r = mSignInResponseRx->pop();

        if (!r.status)
            spdlog::info("登录成功");
        else
            spdlog::warn("登录失败，状态码：%02x", r.status);
    }
}

void Gateway::send(ClientToken tk, Request&& r, ResponseHandler&& handler) {
    const uint16_t cmdId = nextCmdId();

    uint64_t clientId = mTokenClientIdMap.at(tk);

    mRequestResponseHandlerMap.emplace(cmdId, std::move(handler));
    mRequestTx->emplace(RawRequest{cmdId, mGatewayId, clientId, mApiVersion, std::move(r)});
}

Gateway::Gateway(uint16_t apiVersion, uint64_t gatewayId,
                 std::vector<std::pair<uint16_t, RequestHandler>> requestHandlers)
    : mApiVersion(apiVersion), mGatewayId(gatewayId) {
    for (auto&& h : requestHandlers)
        mRequestHandlerMap.emplace(h.first, std::move(h.second));
}

void Gateway::sendSignUpRequest() {
    uint16_t cmdId = nextCmdId();
    mSignUpRequestTx->emplace(RawSignUpRequest{cmdId, mGatewayId});
}

void Gateway::sendSignInRequest(uint64_t clientId) {
    uint16_t cmdId = nextCmdId();
    mSignInRequestTx->emplace(RawSignInRequest{cmdId, mGatewayId, clientId});
}

}  // namespace Protocon
