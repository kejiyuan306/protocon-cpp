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

struct ReceivedRequest {
    uint16_t commandId;
    uint64_t gatewayId;
    uint64_t clientId;
    uint64_t time;
    uint16_t apiVersion;
    uint16_t type;
    std::string data;
};

struct ReceivedResponse {
    uint16_t commandId;
    uint64_t time;
    uint8_t status;
    std::string data;
};

struct Request {
    uint16_t type;
    std::string data;
};

struct Response {
    uint8_t status;
    std::string data;
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

class Engine {
  public:
    void run(const char* host, uint16_t port);
    void stop();

  private:
    Engine(uint16_t apiVersion,
           std::vector<std::unique_ptr<RequestHandler>>&& requestHandlers,
           std::vector<std::unique_ptr<ResponseHandler>>&& responseHandlers);
    ~Engine();

    uint16_t apiVersion;
    std::unordered_map<uint16_t, std::unique_ptr<RequestHandler>> requestHandlerMap;
    std::unordered_map<uint16_t, std::unique_ptr<ResponseHandler>> responseHandlerMap;

    std::unique_ptr<sockpp::stream_socket> socket;

    std::thread readerHandle;

    std::mutex sentRequestTypeMapMtx;
    std::unordered_map<uint16_t, uint16_t> sentRequestTypeMap;

    friend class EngineBuilder;
};

class EngineBuilder {
  public:
    EngineBuilder(uint16_t apiVersion) : apiVersion(apiVersion) {}
    EngineBuilder& with(std::unique_ptr<RequestHandler>&& handler) {
        requestHandlers.emplace_back(std::move(handler));
        return *this;
    }
    EngineBuilder& with(std::unique_ptr<ResponseHandler>&& handler) {
        responseHandlers.emplace_back(std::move(handler));
        return *this;
    }
    Engine build() { return Engine(apiVersion, std::move(requestHandlers), std::move(responseHandlers)); }

  private:
    uint16_t apiVersion;
    std::vector<std::unique_ptr<RequestHandler>> requestHandlers;
    std::vector<std::unique_ptr<ResponseHandler>> responseHandlers;
};

}  // namespace Protocon
