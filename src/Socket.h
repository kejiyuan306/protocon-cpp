#pragma once

#include <spdlog/spdlog.h>

#include <asio/connect.hpp>
#include <asio/ip/address.hpp>
#include <asio/ip/address_v4.hpp>
#include <asio/ip/tcp.hpp>
#include <cstdio>

namespace Protocon {

class Socket {
  public:
    Socket() : mSocket(mContext) {}

    // TODO: We shouldn't expose it directly
    asio::ip::tcp::socket& socket() { return mSocket; };

    bool is_open() const { return mSocket.is_open(); }

    bool connect(const char* host, uint16_t port) {
        try {
            mSocket.connect(asio::ip::tcp::endpoint(asio::ip::make_address(host), port));
        } catch (std::exception& e) {
            spdlog::warn("Failed to connect to server, details: %s", e.what());
        }

        return true;
    }

  private:
    asio::io_context mContext;
    asio::ip::tcp::socket mSocket;
};

}  // namespace Protocon
