#include <Protocon/Protocon.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstdio>
#include <ctime>
#include <thread>

int main() {
    bool stopFlag = false;
    bool sendTrigger = false;
    // Add request handler for command type 0x0001
    auto gateway =
        Protocon::GatewayBuilder(2)
            .withRequestHandler(0x0001, [](Protocon::ClientToken tk, const Protocon::Request& r) {
                spdlog::info("Request receivd, data: {}", reinterpret_cast<const char*>(r.data.data()));

                return Protocon::Response{
                    static_cast<uint64_t>(time(nullptr)),
                    0x00,
                    "{}",
                };
            })
            .withSignInResponseHandler([&sendTrigger](const Protocon::SignInResponse& r) {
                if (!r.status) {
                    sendTrigger = true;
                }
            })
            .build();

    auto tk = gateway.createClientToken();

    // Try to connect to server
    if (!gateway.run("127.0.0.1", 8082)) return 1;

    spdlog::info("Successfully connect to server");

    while (gateway.isOpen() && !stopFlag) {
        // Messages received from server will be in a queue
        // We need a poll() each frame to handle these messages
        gateway.poll();

        if (sendTrigger) {
            // Send a 0x0001 client request with callback
            gateway.send(tk, Protocon::Request{
                                 static_cast<uint64_t>(time(nullptr)),
                                 0x0004,
                                 "{\"msg\": \"Hello world!\"}",
                             },
                         [&stopFlag](const Protocon::Response& response) {
                             spdlog::info("Response received, data: {}", reinterpret_cast<const char*>(response.data.data()));
                             stopFlag = true;
                         });
            sendTrigger = false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    gateway.stop();

    return 0;
}
