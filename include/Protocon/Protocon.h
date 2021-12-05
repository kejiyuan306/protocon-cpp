#pragma once

#include <Protocon/ClientToken.h>
#include <Protocon/Request.h>
#include <Protocon/Response.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Protocon {

class Socket;

template <typename T>
class ThreadSafeQueue;

template <typename K, typename T>
class ThreadSafeUnorderedMap;

using RequestHandler = std::function<Response(ClientToken, const Request&)>;

using ResponseHandler = std::function<void(const Response&)>;

class Gateway {
  public:
    Gateway(Gateway&& gateway);
    ~Gateway();

    bool isOpen() const;

    uint64_t clientId(ClientToken tk) const { return mTokenClientIdMap.at(tk); }

    inline ClientToken createClientToken(uint64_t clientId = 0) {
        auto tk = ClientToken{mTokenCounter++};

        mTokenClientIdMap.emplace(tk, clientId);

        if (clientId)
            mClientIdTokenMap.emplace(clientId, tk);
        else
            mAnonymousTokens.emplace_back(tk);

        return tk;
    }

    bool run(const char* host, uint16_t port);
    void stop();

    void poll();
    void send(ClientToken tk, Request&& r, ResponseHandler&& handler);

  private:
    Gateway(uint16_t apiVersion, uint64_t gatewayId,
            std::vector<std::pair<uint16_t, RequestHandler>>&& requestHandlers);

    uint16_t nextCmdId() { return mCmdIdCounter++; }

    void sendSignUpRequest();
    void sendSignInRequest(uint64_t clientId);

    uint16_t mApiVersion;
    uint64_t mGatewayId;
    std::unordered_map<uint16_t, RequestHandler> mRequestHandlerMap;

    uint64_t mTokenCounter = 0;
    std::vector<ClientToken> mAnonymousTokens;
    std::unordered_map<ClientToken, uint64_t> mTokenClientIdMap;
    std::unordered_map<uint64_t, ClientToken> mClientIdTokenMap;

    std::unique_ptr<Socket> mSocket;

    std::unique_ptr<class Receiver> mReceiver;
    std::unique_ptr<class Sender> mSender;

    uint16_t mCmdIdCounter = 0;

    std::unordered_map<uint64_t, ResponseHandler> mRequestResponseHandlerMap;

    // Maintained by Reader
    std::unique_ptr<ThreadSafeQueue<struct RawRequest>> mRequestRx;
    std::unique_ptr<ThreadSafeQueue<struct RawResponse>> mResponseRx;
    std::unique_ptr<ThreadSafeQueue<struct RawSignUpResponse>> mSignUpResponseRx;
    std::unique_ptr<ThreadSafeQueue<struct RawSignInResponse>> mSignInResponseRx;

    // Maintained by Writer
    std::unique_ptr<ThreadSafeQueue<struct RawRequest>> mRequestTx;
    std::unique_ptr<ThreadSafeQueue<struct RawResponse>> mResponseTx;
    std::unique_ptr<ThreadSafeQueue<struct RawSignUpRequest>> mSignUpRequestTx;
    std::unique_ptr<ThreadSafeQueue<struct RawSignInRequest>> mSignInRequestTx;

    friend class GatewayBuilder;
};

class GatewayBuilder {
  public:
    GatewayBuilder(uint16_t apiVersion) : mApiVersion(apiVersion) {}
    GatewayBuilder& withGatewayId(uint64_t gatewayId) {
        this->mGatewayId = gatewayId;
        return *this;
    }
    GatewayBuilder& withRequestHandler(uint16_t type, RequestHandler&& handler) {
        mRequestHandlers.emplace_back(std::make_pair(type, std::move(handler)));
        return *this;
    }
    Gateway build() {
        return Gateway(
            mApiVersion,
            mGatewayId,
            std::move(mRequestHandlers));
    }

  private:
    uint16_t mApiVersion;

    uint64_t mGatewayId = 0;

    std::vector<std::pair<uint16_t, RequestHandler>> mRequestHandlers;
};

}  // namespace Protocon
