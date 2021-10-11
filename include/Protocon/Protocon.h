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

struct ReceivedRequest {
    uint16_t commandId;
    uint64_t gatewayId;
    uint64_t clientId;
    uint64_t time;
    uint16_t apiVersion;
    uint16_t type;
    std::u8string data;
};

struct ReceivedResponse {
    uint16_t commandId;
    uint64_t time;
    uint8_t status;
    std::u8string data;
};

struct SentRequest {
    uint64_t clientId;
    uint16_t type;
    std::u8string data;
};

struct SentResponse {
    uint64_t clientId;
    uint8_t status;
    std::u8string data;
};

using RequestHandler = std::function<SentResponse(const ReceivedRequest&)>;

using ResponseHandler = std::function<void(const ReceivedResponse&)>;

class Client {
  public:
    ~Client();

    bool stopFlag() const { return mStopFlag; }

    bool isOpen() const;

    bool run(const char* host, uint16_t port);
    void stop();

    void poll();
    void send(SentRequest&& r, ResponseHandler&& handler);

  private:
    Client(uint16_t apiVersion, uint64_t gatewayId,
           std::vector<std::pair<uint16_t, RequestHandler>>&& requestHandlers);

    uint64_t mGatewayId;
    uint16_t mApiVersion;
    std::unordered_map<uint16_t, RequestHandler> mRequestHandlerMap;

    std::atomic_bool mStopFlag;

    std::unique_ptr<sockpp::stream_socket> mSocket;

    std::thread mReaderHandle;
    std::thread mWriterHandle;

    uint16_t mCmdIdCounter = 1;
    const uint16_t cMaxCmdId = 0x7fff;

    std::unordered_map<uint64_t, ResponseHandler> mSentRequestResponseHandlerMap;

    // Maintained by Reader
    std::unique_ptr<ThreadSafeQueue<ReceivedRequest>> mReceivedRequestQueue;
    std::unique_ptr<ThreadSafeQueue<ReceivedResponse>> mReceivedResponseQueue;

    // Maintained by Writer
    std::unique_ptr<ThreadSafeQueue<std::pair<uint16_t, SentRequest>>> mSentRequestQueue;
    std::unique_ptr<ThreadSafeQueue<std::pair<uint16_t, SentResponse>>> mSentResponseQueue;

    friend class ClientBuilder;
};

class ClientBuilder {
  public:
    ClientBuilder(uint16_t apiVersion) : mApiVersion(apiVersion) {}
    ClientBuilder& withGatewayId(uint64_t gatewayId) {
        this->mGatewayId = gatewayId;
        return *this;
    }
    ClientBuilder& withRequestHandler(uint16_t type, RequestHandler&& handler) {
        mRequestHandlers.emplace_back(std::make_pair(type, std::move(handler)));
        return *this;
    }
    Client build() { return Client(mApiVersion, mGatewayId, std::move(mRequestHandlers)); }

  private:
    uint16_t mApiVersion;

    uint64_t mGatewayId = 0;

    std::vector<std::pair<uint16_t, RequestHandler>> mRequestHandlers;
};

}  // namespace Protocon
