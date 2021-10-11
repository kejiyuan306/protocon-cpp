#pragma once

#include <Protocon/Protocon.h>
#include <sockpp/stream_socket.h>

#include "Util.h"

namespace Protocon {

enum DecodeResult {
    Request,
    Response,
    None,
};

class Decoder {
  public:
    inline Decoder(sockpp::stream_socket& mSocket) : mSocket(mSocket) {}

    inline DecodeResult decode(ReceivedRequest& request, ReceivedResponse& response) {
        std::array<char8_t, 1024> buf;

        uint16_t cmdId;
        if (!read(&cmdId, sizeof(cmdId))) return None;
        cmdId = Util::BigEndian(cmdId);

        if ((cmdId & 0x8000) == 0 && cmdId != 0) {
            // 请求。

            uint64_t gatewayId;
            if (!read(&gatewayId, sizeof(gatewayId))) return None;
            gatewayId = Util::BigEndian(gatewayId);

            uint64_t clientId;
            if (!read(&clientId, sizeof(clientId))) return None;
            clientId = Util::BigEndian(clientId);

            uint64_t time;
            if (!read(&time, sizeof(time))) return None;
            time = Util::BigEndian(time);

            uint16_t apiVersion;
            if (!read(&apiVersion, sizeof(apiVersion))) return None;
            apiVersion = Util::BigEndian(apiVersion);

            uint16_t type;
            if (!read(&type, sizeof(type))) return None;
            type = Util::BigEndian(type);

            uint32_t length;
            if (!read(&length, sizeof(length))) return None;
            length = Util::BigEndian(length);

            if (!read(&buf, length)) return None;

            request = ReceivedRequest{
                .commandId = cmdId,
                .gatewayId = gatewayId,
                .clientId = clientId,
                .time = time,
                .apiVersion = apiVersion,
                .type = type,
                .data = std::u8string(buf.data(), length),
            };
            return Request;
        } else if ((cmdId & 0x8000) == 0x8000 && cmdId != 0x8000) {
            // 响应。

            cmdId ^= 0x8000;

            uint64_t time;
            if (!~mSocket.read_n(&time, sizeof(time))) return None;
            time = Util::BigEndian(time);

            uint8_t status;
            if (!~mSocket.read_n(&status, sizeof(status))) return None;

            uint32_t length;
            if (!~mSocket.read_n(&length, sizeof(length))) return None;
            length = Util::BigEndian(length);

            if (!~mSocket.read_n(&buf, length)) return None;

            response = ReceivedResponse{
                .commandId = cmdId,
                .time = time,
                .status = status,
                .data = std::u8string(buf.data(), length),
            };
            return Response;
        } else {
            std::printf("Fatal error, please contact to developers.\n");
            return None;
        }
    }

  private:
    sockpp::stream_socket& mSocket;

    inline bool read(void* buf, size_t n) {
        auto res = mSocket.read_n(buf, n);
        return res && ~res;
    }
};

}  // namespace Protocon
