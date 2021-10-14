#pragma once

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

namespace sockpp {

class stream_socket;

}

namespace Protocon {

template <typename T>
class ThreadSafeQueue;

template <typename K, typename T>
class ThreadSafeUnorderedMap;

template <typename T>
struct CommandWrapper;

class Sender;
class Receiver;

struct Client {
    uint64_t token;
};

struct Request {
    uint64_t clientId;
    uint64_t time;
    uint16_t type;
    std::u8string data;
};

struct RawRequest {
    uint16_t cmdId;
    uint64_t gatewayId;
    uint16_t apiVersion;
    Request request;
};

struct Response {
    uint64_t time;
    uint8_t status;
    std::u8string data;
};

struct RawResponse {
    uint16_t cmdId;
    Response response;
};

struct RawSignUpRequest {
    uint16_t cmdId;
};

struct RawSignUpResponse {
    uint16_t cmdId;
    uint64_t clientId;
    uint8_t status;
};

struct RawSignInRequest {
    uint16_t cmdId;
    uint64_t clientId;
};

struct RawSignInResponse {
    uint16_t cmdId;
    uint8_t status;
};

using RequestHandler = std::function<Response(const Request&)>;

using ResponseHandler = std::function<void(const Response&)>;

class Gateway {
  public:
    ~Gateway();

    bool isOpen() const;

    uint64_t clientId(const Client& client) const { return mTokenClientIdMap.at(client.token); }

    inline Client createClient(uint64_t clientId = 0) {
        mTokenClientIdMap.emplace(mTokenCounter, clientId);
        if (!clientId)
            mAnonymousTokens.emplace_back(mTokenCounter);
        return Client{.token = mTokenCounter++};
    }

    bool run(const char* host, uint16_t port);
    void stop();

    void poll();
    void send(Request&& r, ResponseHandler&& handler);

  private:
    Gateway(uint16_t apiVersion, uint64_t gatewayId,
            std::vector<std::pair<uint16_t, RequestHandler>>&& requestHandlers);

    uint64_t mGatewayId;
    uint16_t mApiVersion;
    std::unordered_map<uint16_t, RequestHandler> mRequestHandlerMap;

    uint64_t mTokenCounter = 0;
    std::vector<uint64_t> mAnonymousTokens;
    std::unordered_map<uint64_t, uint64_t> mTokenClientIdMap;

    std::unique_ptr<sockpp::stream_socket> mSocket;

    std::unique_ptr<Receiver> mReceiver;
    std::unique_ptr<Sender> mSender;

    uint16_t mCmdIdCounter = 1;
    const uint16_t cMaxCmdId = 0x7fff;

    std::unordered_map<uint64_t, ResponseHandler> mSentRequestResponseHandlerMap;

    // Maintained by Reader
    std::unique_ptr<ThreadSafeQueue<RawRequest>> mReceivedRequestQueue;
    std::unique_ptr<ThreadSafeQueue<RawResponse>> mReceivedResponseQueue;
    std::unique_ptr<ThreadSafeQueue<RawSignUpResponse>> mReceivedSignUpResponseQueue;
    std::unique_ptr<ThreadSafeQueue<RawSignInResponse>> mReceivedSignInResponseQueue;

    // Maintained by Writer
    std::unique_ptr<ThreadSafeQueue<CommandWrapper<Request>>> mSentRequestQueue;
    std::unique_ptr<ThreadSafeQueue<CommandWrapper<Response>>> mSentResponseQueue;
    std::unique_ptr<ThreadSafeQueue<RawSignUpRequest>> mSentSignUpRequestQueue;
    std::unique_ptr<ThreadSafeQueue<RawSignInRequest>> mSentSignInRequestQueue;

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
