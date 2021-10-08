#pragma once

#include <cstdint>
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

struct Request {
    uint64_t clientId;
    uint16_t type;
    std::u8string data;
};

struct Response {
    uint64_t clientId;
    uint8_t status;
    std::u8string data;
};

class RequestHandler {
  public:
    virtual ~RequestHandler(){};

    virtual uint16_t type() = 0;
    virtual Response handle(const ReceivedRequest& request) = 0;
};

class ResponseHandler {
  public:
    virtual ~ResponseHandler(){};

    virtual uint16_t type() = 0;
    virtual void handle(const ReceivedResponse& response) = 0;
};

class Client {
  public:
    void run(const char* host, uint16_t port);
    void stop();

    void send(Request&& r);

  private:
    Client(uint16_t apiVersion, uint64_t gatewayId,
           std::vector<std::unique_ptr<RequestHandler>>&& requestHandlers,
           std::vector<std::unique_ptr<ResponseHandler>>&& responseHandlers);
    ~Client();

    uint64_t mGatewayId;
    uint16_t mApiVersion;
    std::unordered_map<uint16_t, std::unique_ptr<RequestHandler>> mRequestHandlerMap;
    std::unordered_map<uint16_t, std::unique_ptr<ResponseHandler>> mResponseHandlerMap;

    std::unique_ptr<sockpp::stream_socket> mSocket;

    std::thread mReaderHandle;
    std::thread mWriterHandle;

    std::unique_ptr<ThreadSafeQueue<Request>> mRequestQueue;
    std::unique_ptr<ThreadSafeUnorderedMap<uint16_t, uint16_t>> mSentRequestTypeMap;

    friend class ClientBuilder;
};

class ClientBuilder {
  public:
    ClientBuilder(uint16_t apiVersion) : mApiVersion(apiVersion) {}
    ClientBuilder& withGatewayId(uint64_t gatewayId) {
        this->mGatewayId = gatewayId;
        return *this;
    }
    ClientBuilder& with(std::unique_ptr<RequestHandler>&& handler) {
        mRequestHandlers.emplace_back(std::move(handler));
        return *this;
    }
    ClientBuilder& with(std::unique_ptr<ResponseHandler>&& handler) {
        mResponseHandlers.emplace_back(std::move(handler));
        return *this;
    }
    Client build() { return Client(mApiVersion, mGatewayId, std::move(mRequestHandlers), std::move(mResponseHandlers)); }

  private:
    uint16_t mApiVersion;

    uint64_t mGatewayId = 0;

    std::vector<std::unique_ptr<RequestHandler>> mRequestHandlers;
    std::vector<std::unique_ptr<ResponseHandler>> mResponseHandlers;
};

}  // namespace Protocon
