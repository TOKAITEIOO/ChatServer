#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <string>
#include <vector>
#include <map>
#include <iostream>
// 获取单例对象的接口函数
using namespace std;
using namespace std::placeholders;
ChatService* ChatService::instance() {
    static ChatService service;
    return &service;
}

// 注册消息以及Handler对应的回调操作
ChatService::ChatService () {
    // 用户基本业务管理相关事件处理回调注册
    _msgHandlerMap.insert({LOGIN_MSG, bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, bind(&ChatService::createGroup, this, _1, _2, _3)});
    
    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({ADD_GROUP_MSG, bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, bind(&ChatService::groupChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, bind(&ChatService::loginOut, this, _1, _2, _3)});
    
    // 连接redis服务器
    if(_redis.connect()) {
        //设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage,this, _1, _2));
    }
}

Msghandler ChatService::getHandler(int msgid) {
    //记录错误日志，msgid没有对应事件的处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end()) {
        // 找不到相应的处理器
        //LOG_ERROR << "message id " << msgid <<"cannot find handler!";
        return [=](const TcpConnectionPtr& conn, json& js, Timestamp time){
            LOG_ERROR << "message id " << msgid <<" cannot find handler!";
        };
    } else {
        return _msgHandlerMap[msgid];
    }
    
}

// 处理登录业务 ORM框架（Object Relation map） 在业务层操作的都是对象，数据层
void ChatService::login(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    LOG_INFO << "do login service!!";


    int id = js["id"].get<int>();
    string pwd = js["password"];
    User user = _userModel.query(id);
    // 表示存在该用户（user.getId() != -1）
    if (user.getId() == id && user.getPassword() == pwd) {
        if (user.getState() == "online") {
            // 表示该用户已经登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account has been login, use another";
            conn->send(response.dump());

        } else {
            // 登录成功，记录用户连接信息
            // 需要添加线程互斥操作
            {
                lock_guard<mutex> lock(_connMtx);
                _userConnMap.insert({id, conn});
            }
            //id 用户登录成功以后，向redis订阅channel（id）;
            _redis.subscribe(id);


            //临界区代码段
            //登录成功,更新用户状态信息 state offline => online
            user.setState("online");
            _userModel.updateState(user); //将用户的信息刷新一下进行返回

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();


            //查询用户是否有离线消息，如果有的话，就把消息打包进json一起传回去
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty()) {
                response["offlinemsg"] = vec;
                //读取该用户的离线消息后，应该删除该用户的离线消息
                _offlineMsgModel.remove(id);
            }
            // 查询用户的好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty()) {
                vector<string> vec2;
                for (User& user : userVec) {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }
            // 查询用户的群组消息
            vector <Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty()) {
                // group:[{groupid : [xxx,xxx,xxx,xxx]}]
                vector<string> groupV;
                for (Group & group : groupuserVec) {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getname();
                    grpjson["groupdesc"] = group.getdesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers()) {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    } 
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                response["groups"] = groupV;
            }

            conn->send(response.dump());

        }
    
    } else {
        //(1)该用户不存在登录失败 (2) 该用户存在但是密码错误
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "name or password error";
        conn->send(response.dump());
    }

}



// 处理注册业务 name password
void ChatService::reg(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    // LOG_INFO << "do reg service!!";
    string name = js["name"];
    string pwd = js["password"];
    User user;
    user.setName(name);
    user.setPassword(pwd);
    bool state = _userModel.insert(user);
    if (state) {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    } else {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}



// 服务器异常，业务重置方法
void ChatService::reset() {
    //把online状态的用户设置为offline
    _userModel.resetState();

}



// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr& conn) {
    User user; //更新用户的信息需要用到
    //做两件事情（1）从存储在线用户连接信息的Map表中删去该conn连接信息，并获得删去连接信息的id
    //（2）根据id将对用的State设置为offline;
    {
        lock_guard<mutex> lock(_connMtx);
        for (auto it = _userConnMap.begin(); it !=_userConnMap.end(); ++it) {
            if (it->second == conn) {
                // 从Map表中删除用户的连接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    // 用户注销，相当于是下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getId());

    //更新用户的状态信息
    if (user.getId() != -1) {
        user.setState("offline");
        _userModel.updateState(user);
    }

}
// 处理注销业务

void ChatService::loginOut(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMtx);
        auto it = _userConnMap.find(userid);
        if(it != _userConnMap.end()) {
            _userConnMap.erase(it);
        }
    }
    // 用户注销，相当于是下线，在redis中取消订阅通道
    _redis.unsubscribe(userid);


    //更新用户的状态信息
    User user(userid,"","","offline"); //更新用户的信息需要用到
    _userModel.updateState(user);


}




// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    int toId = js["toid"].get<int>();
    //情况1 收发用户在同一服务器上，且双方在线
    {
        lock_guard<mutex> lock(_connMtx);
        auto it = _userConnMap.find(toId);
        if (it != _userConnMap.end()) {
            //toid 在线，转发消息
            //服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        } 
    }
    //情况2 收发用户在不同一服务器上，且双方在线
    User user = _userModel.query(toId);
    if (user.getState() == "online") {
        _redis.publish(toId, js.dump());
        return;
    }

    // toid 不在线，在数据库中存储离线消息
    _offlineMsgModel.insert(toId, js.dump());

}


void ChatService::addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _friendModel.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    int userid = js["id"];
    string name = js["groupname"];
    string desc = js["groupdesc"];
 
    Group group(-1, name, desc);
    // 存储新创建的群组信息
    if (_groupModel.createGroup(group)) {
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组业务
void  ChatService::addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}


// 群组聊天业务
void  ChatService::groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);
    
    //保证map表操作时的线程安全问题
    lock_guard<mutex> lock(_connMtx);
    for (int id : useridVec) {
        
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end()) {
            
            // 转发群消息
            it->second->send(js.dump());

        } else {
            // 查询toid是否在线
            User user = _userModel.query(id);
            if (user.getState() == "online") {
                _redis.publish(id, js.dump());
            } else {
                // 存储离线消息
                _offlineMsgModel.insert(id, js.dump()); 
            }
        }
    }
}

// 从redis 消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string message) {
    lock_guard<mutex> lock(_connMtx);

    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end()) {
        it->second->send(message);
    }
    // 存储该用户的离线消息
    _offlineMsgModel.insert(userid, message);
}