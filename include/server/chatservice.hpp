#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include "json.hpp"
#include <map>
#include <functional>
#include <mutex>
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"
using namespace std;
using namespace std::placeholders;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

// 表示处理消息事件的回调方法类型
using Msghandler = std::function<void(const TcpConnectionPtr& conn, json& js, Timestamp time)>;

// 聊天服务器业务类
class ChatService {
public:
    //获取单例对象的接口函数
    static ChatService* instance();
    // 处理登录业务
    void login(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 处理注册业务
    void reg(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 一对一聊天业务(网络层派发的处理器回调)
    void oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 添加好友业务
    void addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 创建群组业务
    void createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 处理注销业务
    void loginOut(const TcpConnectionPtr& conn, json& js, Timestamp time);

    // 获取消息对应的处理器
    Msghandler getHandler(int msgid);
    // 服务器异常，业务重置方法
    void reset();
    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr& conn);
    // 从redis 消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int userid, string message);

private:
    ChatService();
    // 存储消息id 和其对应业务的处理方法
    unordered_map<int, Msghandler> _msgHandlerMap;
    // 存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap;
    //定义互斥锁，保证userConnection的线程安全
    mutex _connMtx;
    // 数据操作类对象
    UserModel _userModel;
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;
    //redis 操作对象
    Redis _redis;
};

#endif