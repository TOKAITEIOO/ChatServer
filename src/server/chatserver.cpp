#include "chatservice.hpp"
#include "chatserver.hpp"
#include "json.hpp"

#include <functional>
#include <string>
#include <iostream>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;
//
ChatServer::ChatServer(EventLoop* loop, const InetAddress& listenAddr, const string& nameArg)
    :_server(loop ,listenAddr, nameArg)
    ,_loop(loop)
{
    //注册连接的回调
    _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));
    //注册消息的回调
    _server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));
    //设置线程的数量
    _server.setThreadNum(4);
}

void ChatServer::start() {
    _server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr& conn) {
   
    if (!conn->connected()) {
        ChatService::instance()->clientCloseException(conn);

        conn->shutdown();
    }
}


void ChatServer::onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp time) {
    string buf = buffer->retrieveAllAsString();
    // std::cout << buf <<std::endl;
    json js = json::parse(buf);
    // 达到的目的，完全解耦网络模块的代码或者业务模块的代码
    // 通过js["msgid"]获取->业务处理器handler->conn 解析出来的json对象 time
    // 这里解耦出来的仍然是一个json对象，需要将其转换为整型类型
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // 回调消息绑定好的事件处理器，来执行相应的业务处理
    msgHandler(conn, js, time);
    
}