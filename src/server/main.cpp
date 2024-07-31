#include "chatserver.hpp"
#include "chatservice.hpp"
#include "signal.h"
#include <iostream>
using namespace std;



// 处理服务器ctrl + c结束以后，重置user 的状态信息
void resetHandler(int) {
    ChatService::instance()->reset();
    exit(0);
}

int main(int agrc, char** argv) {
    if (agrc < 3) {
        cerr << "command invalid example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }
    char* ip = argv[1];
    uint16_t port = atoi(argv[2]);
    
    signal(SIGINT, resetHandler);


    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "");

    server.start();
    loop.loop();
    return 0;
}