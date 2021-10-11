#include <Protocon/Protocon.h>

#include <chrono>
#include <cstdio>
#include <thread>

bool stop_flag = false;

int main() {
    // 添加 0x0001 的服务端请求处理器
    auto client =
        Protocon::ClientBuilder(2)
            .withRequestHandler(0x0001, [](const Protocon::ReceivedRequest& r) {
                std::printf("Request receivd, data: %s\n", reinterpret_cast<const char*>(r.data.data()));

                return Protocon::SentResponse{
                    .clientId = 0x0001,
                    .status = 0x00,
                    .data = u8"{}",
                };
            })
            .build();

    // 尝试连接到服务端
    if (!client.run("127.0.0.1", 8081)) return 1;

    // 发送一个 0x0001 的客户端请求，并传入回调函数
    client.send(Protocon::SentRequest{
                    .clientId = 0x0001,
                    .type = 0x0001,
                    .data = u8"{\"msg\": \"Hello world!\"}",
                },
                [](const Protocon::ReceivedResponse& response) {
                    std::printf("Response received, data: %s\n", reinterpret_cast<const char*>(response.data.data()));
                    stop_flag = true;
                });

    while (client.isOpen() && !stop_flag) {
        // 从服务端收到的报文会放在队列中
        // 只有在 poll 的时候才会调用各个回调处理相应的报文
        client.poll();

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    client.stop();

    return 0;
}
