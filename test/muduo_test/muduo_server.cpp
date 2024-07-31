/*
muduo 网络库给用户提供了两个主要的类

TcpServer：用于编写服务端程序
TcpClient: 用于编写客户端程序

epoll + 线程池
好处：能够把网络I/O的代码和业务代码区分开来
        （1）用户的链接和断开 （2）用户的可读写时间

*/

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <functional>
#include <iostream>
#include <string>

using namespace std;
using namespace std::placeholders;
// 基于muduo网络库开发服务器程序
// 1. 组合TcpServer 对象
// 2. 创建Eventloop事件循环对象的指针
// 3. 明确TcpServer 构造函数需要什么什么样的参数，输出ChatServer的构造函数
// 4. 在当前服务器类的构造函数当中，注册处理连接的回调函数和处理读写事件的回调函数
// 5. 设置合适的服务端线程数量，muduo库会自己划分IO线程和worker线程
class ChatServer {
public:
    ChatServer(muduo::net::EventLoop* loop, //事件循环
               const muduo::net::InetAddress& listenAddr, // IP + port
               const string& nameArg) // 服务器的名字
        :_loop(loop) 
        ,_server(loop, listenAddr, nameArg) 
    {
        // 给服务器注册用户连接的创建和断开回调
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
        // 给服务器注册用户读写时间的回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
        // 设置服务器端的线程数量 , 一个IO线程，三个worker 线程
        _server.setThreadNum(4);

    }
    // 开启事件的循环
    void start() {
        _server.start();
    }
private:
    // typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
    // 专门处理用户的链接创建和断开 epoll listenfd accept
    void onConnection(const muduo::net::TcpConnectionPtr& conn) 
    {

        if (conn->connected()) {
            cout << conn->peerAddress().toIpPort() << "->"
                << conn->localAddress().toIpPort() << "state:online" << endl;
        } else {
            cout << conn->peerAddress().toIpPort() << "->"
                << conn->localAddress().toIpPort() << "state:offline" << endl;
                conn->shutdown(); //close(fd); 释放socket资源
                //_loop->quit();
        }

    }
    // 专门处理用户的读写事件的
    void onMessage(const muduo::net::TcpConnectionPtr& conn, // 连接
                    muduo::net::Buffer* buffer, //缓冲区
                    muduo::Timestamp time // 接收到数据的时间信息
                    ) 
    { 
        string buf = buffer->retrieveAllAsString();
        cout << "receive data:" << buf <<"time:" << time.toString() << endl;
        conn->send(buf);
    }

    muduo::net::TcpServer _server; //#1 TcpServer对象定义
    muduo::net::EventLoop *_loop; //#2 epoll // 时间循环的指针


};

int main() {
    muduo::net::EventLoop loop;
    muduo::net::InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");
    server.start();
    loop.loop(); // 调用了大家的epoll_wait(), 以阻塞的方式等待新用户的链接，已连接用户的读写事件等
    return 0;
}