#ifndef CHATSERVER_H_
#define CHATSERVER_H_

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

using namespace muduo;
using namespace muduo::net;

//聊天服务器的种类
class ChatServer {
public:
    //初始化聊天服务器对象
    ChatServer(EventLoop* loop, const InetAddress& listenAddr, const string& nameArg);

    void start();
private:
    // 上报链接相关信息的回调函数
    void onConnection(const TcpConnectionPtr& conn);
    // 上报读写事件相关信息的回调函数
    void onMessage(const TcpConnectionPtr& conn,
                            Buffer* buffer,
                            Timestamp time);
    TcpServer _server; //组合muduo库，实现服务器功能的类对象
    EventLoop* _loop; //指向事件循环的指针
};



#endif