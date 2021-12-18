#pragma once

#include <spdlog/spdlog.h>

#include <asio/connect.hpp>
#include <asio/ip/address.hpp>
#include <asio/ip/address_v4.hpp>
#include <asio/ip/tcp.hpp>
#include <cstdio>
#include <exception>

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
            spdlog::warn("Failed to connect to server, details: {}", e.what());
            return false;
        }

        return true;
    }

    bool shutdown() {
        try {
            mSocket.shutdown(asio::socket_base::shutdown_both);
        } catch (std::exception& e) {
            spdlog::warn("Failed to shutdown the socket, details: {}", e.what());
        }

        return true;
    }

  private:
    asio::io_context mContext;
    asio::ip::tcp::socket mSocket;
};

}  // namespace Protocon
