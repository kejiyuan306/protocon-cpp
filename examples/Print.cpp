#include <Protocon/Protocon.h>

#include <chrono>
#include <cstdio>
#include <thread>

bool stop_flag = false;

int main() {
    auto client = Protocon::ClientBuilder(2).build();

    client.run("127.0.0.1", 8081);

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
}
