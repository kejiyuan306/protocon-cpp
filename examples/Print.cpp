#include <Protocon/Protocon.h>

#include <chrono>
#include <cstdio>
#include <thread>

bool stop_flag = false;

int main() {
    // 添加 0x0001 的服务端请求处理器
    auto gateway =
        Protocon::GatewayBuilder(2)
            .withRequestHandler(0x0001, [](Protocon::ClientToken tk, const Protocon::Request& r) {
                std::printf("Request receivd, data: %s\n", reinterpret_cast<const char*>(r.data.data()));

                return Protocon::Response{
                    .status = 0x00,
                    .data = "{}",
                };
            })
            .build();

    auto tk = gateway.createClientToken(1);

    // 尝试连接到服务端
    if (!gateway.run("127.0.0.1", 8081)) return 1;

    // 发送一个 0x0001 的客户端请求，并传入回调函数
    gateway.send(tk, Protocon::Request{
                         .type = 0x0001,
                         .data = "{\"msg\": \"Hello world!\"}",
                     },
                 [](const Protocon::Response& response) {
                     std::printf("Response received, data: %s\n", reinterpret_cast<const char*>(response.data.data()));
                     stop_flag = true;
                 });

    while (gateway.isOpen() && !stop_flag) {
        // 从服务端收到的报文会放在队列中
        // 只有在 poll 的时候才会调用各个回调处理相应的报文
        gateway.poll();

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    gateway.stop();

    return 0;
}
