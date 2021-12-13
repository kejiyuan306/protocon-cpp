#include <Protocon/Protocon.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstdio>
#include <ctime>
#include <thread>

bool stop_flag = false;

int main() {
    // 添加 0x0001 的服务端请求处理器
    auto gateway =
        Protocon::GatewayBuilder(2)
            .withRequestHandler(0x0001, [](Protocon::ClientToken tk, const Protocon::Request& r) {
                spdlog::info("Request receivd, data: %s", reinterpret_cast<const char*>(r.data.data()));

                return Protocon::Response{
                    static_cast<uint64_t>(time(nullptr)),
                    0x00,
                    "{}",
                };
            })
            .build();

    auto tk = gateway.createClientToken();

    // 尝试连接到服务端
    if (!gateway.run("127.0.0.1", 8082)) return 1;

    // 发送一个 0x0001 的客户端请求，并传入回调函数
    gateway.send(tk, Protocon::Request{
                         static_cast<uint64_t>(time(nullptr)),
                         0x0001,
                         "{\"msg\": \"Hello world!\"}",
                     },
                 [](const Protocon::Response& response) {
                     spdlog::info("Response received, data: %s", reinterpret_cast<const char*>(response.data.data()));
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
