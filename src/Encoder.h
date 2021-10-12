#pragma once

#include <Protocon/Protocon.h>
#include <sockpp/stream_socket.h>

namespace Protocon {

class Encoder {
  public:
    inline Encoder(sockpp::stream_socket& mSocket) : mSocket(mSocket) {}

    inline bool encode(uint16_t cmdId, uint64_t gatewayId, uint16_t apiVersion, const SentRequest& r) {
        cmdId = Util::BigEndian(cmdId);
        if (!write(&cmdId, sizeof(cmdId))) return false;

        gatewayId = Util::BigEndian(gatewayId);
        if (!write(&gatewayId, sizeof(gatewayId))) return false;

        uint64_t clientId = Util::BigEndian(r.clientId);
        if (!write(&clientId, sizeof(clientId))) return false;

        uint64_t time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        time = Util::BigEndian(time);
        if (!write(&time, sizeof(time))) return false;

        apiVersion = Util::BigEndian(apiVersion);
        if (!write(&apiVersion, sizeof(apiVersion))) return false;

        uint16_t type = Util::BigEndian(r.type);
        if (!write(&type, sizeof(type))) return false;

        uint32_t length = r.data.length();
        length = Util::BigEndian(length);
        if (!write(&length, sizeof(length))) return false;
        length = Util::BigEndian(length);

        if (!write(r.data.data(), length)) return false;

        return true;
    }

    inline bool encode(uint16_t cmdId, const SentResponse& r) {
        if (!~mSocket.write_n(&cmdId, sizeof(cmdId))) return false;

        uint64_t time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (!~mSocket.write_n(&time, sizeof(time))) return false;

        if (!~mSocket.write_n(&r.status, sizeof(r.status))) return false;

        uint32_t length = r.data.length();
        if (!~mSocket.write_n(&length, sizeof(length))) return false;

        if (!~mSocket.write_n(r.data.data(), length)) return false;

        return true;
    }

  private:
    sockpp::stream_socket& mSocket;

    inline bool write(const void* buf, size_t n) {
        auto res = mSocket.write_n(buf, n);
        return res && ~res;
    }
};

}  // namespace Protocon
