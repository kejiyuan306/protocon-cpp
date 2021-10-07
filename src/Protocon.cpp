#include <Protocon/Protocon.h>
#include <sockpp/tcp_connector.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <thread>

namespace Protocon {

void Engine::run(const char* host, uint16_t port) {
    auto conn = std::make_unique<sockpp::tcp_connector>();
    if (!conn->connect(sockpp::inet_address(host, port)))
        printf("Connect failed, details: %s\n", conn->last_error_str().c_str());
    socket = std::move(conn);

    readerHandle = std::thread([socket = socket->clone(),
                                requestHandlerMap = &this->requestHandlerMap,
                                responseHandlerMap = &this->responseHandlerMap,
                                requestTypeMapMtx = &this->sentRequestTypeMapMtx,
                                requestTypeMap = &this->sentRequestTypeMap]() mutable {
        std::array<char, 1024> buf;

        while (true) {
            uint16_t cmdId;
            if (!~socket.read_n(&cmdId, sizeof(cmdId))) break;

            // TODO: 处理字节序问题。
            if (cmdId > 0) {
                // 请求。

                uint64_t gatewayId;
                if (!~socket.read_n(&gatewayId, sizeof(gatewayId))) break;

                uint64_t clientId;
                if (!~socket.read_n(&clientId, sizeof(clientId))) break;

                uint64_t time;
                if (!~socket.read_n(&time, sizeof(time))) break;

                uint16_t apiVersion;
                if (!~socket.read_n(&apiVersion, sizeof(apiVersion))) break;

                uint16_t type;
                if (!~socket.read_n(&type, sizeof(type))) break;

                uint32_t length;
                if (!~socket.read_n(&length, sizeof(length))) break;

                if (!~socket.read_n(&buf, length)) break;

                requestHandlerMap->at(type)->handle(ReceivedRequest{
                    .commandId = cmdId,
                    .gatewayId = gatewayId,
                    .clientId = clientId,
                    .time = time,
                    .apiVersion = apiVersion,
                    .type = type,
                    .data = std::string(buf.data(), length),
                });
            } else if (cmdId < 0) {
                // 响应。

                cmdId = -cmdId;

                uint64_t time;
                if (!~socket.read_n(&time, sizeof(time))) break;

                uint8_t status;
                if (!~socket.read_n(&status, sizeof(status))) break;

                uint32_t length;
                if (!~socket.read_n(&length, sizeof(length))) break;

                if (!~socket.read_n(&buf, length)) break;

                uint16_t type;
                {
                    std::lock_guard<std::mutex> lock(*requestTypeMapMtx);
                    type = requestTypeMap->at(cmdId);
                }

                responseHandlerMap->at(type)->handle(ReceivedResponse{
                    .commandId = cmdId,
                    .time = time,
                    .status = status,
                    .data = std::string(buf.data(), length),
                });
            } else {
                printf("Fatal error, please contact to developers.");
                return;
            }
        }
        printf("Disconnected, details: %s\n", socket.last_error_str().c_str());
    });
}

void Engine::stop() {
    socket->shutdown();
}

Engine::Engine(uint16_t apiVersion,
               std::vector<std::unique_ptr<RequestHandler>>&& requestHandlers,
               std::vector<std::unique_ptr<ResponseHandler>>&& responseHandlers)
    : apiVersion(apiVersion) {
    for (auto&& h : requestHandlers)
        requestHandlerMap.emplace(h->type(), std::move(h));
    for (auto&& h : responseHandlers)
        responseHandlerMap.emplace(h->type(), std::move(h));
}

Engine::~Engine() = default;

}  // namespace Protocon
