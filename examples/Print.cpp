#include <Protocon/Protocon.h>

#include <chrono>
#include <cstdio>
#include <thread>

bool stop_flag = false;

int main() {
    auto client =
        Protocon::ClientBuilder(2)
            .withRequestHandler(std::make_pair(0x0001, [](const Protocon::ReceivedRequest& r) {
                std::printf("Request receivd, data: %s\n", reinterpret_cast<const char*>(r.data.data()));

                return Protocon::SentResponse{
                    .clientId = 0x0001,
                    .status = 0x00,
                    .data = u8"{}",
                };
            }))
            .build();

    if (!client.run("127.0.0.1", 8081)) return 1;

    client.send(Protocon::SentRequest{
                    .clientId = 0x0001,
                    .type = 0x0001,
                    .data = u8"{\"msg\": \"Hello world!\"}",
                },
                [](const Protocon::ReceivedResponse& response) {
                    std::printf("Response received, data: %s\n", reinterpret_cast<const char*>(response.data.data()));
                    stop_flag = true;
                });

    while (!stop_flag) {
        client.poll();

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    client.stop();

    return 0;
}
