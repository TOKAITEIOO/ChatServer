#include "json.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include <ctime>
using namespace std;
using json = nlohmann::json;


#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;

// 控制聊天页面的全局变量
bool isMainMenuRunning = false;

// 显示当前登录成功用户的基本信息
void showCurrentUserData();

// 接受线程
void readTaskHandler(int clientfd);

// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime();

// 主聊天页面程序，登录成功以后才能进入
void mainMenu(int clientfd);

// 聊天客户端程序实现，main线程用作发送线程，子线程用作接收线程
int main (int argc, char **argv) {
    if (argc < 3) {
        cerr << "command invalid example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1); 
    }
    // 解析通过命令行参数传递的ip 和 port
    char* ip = argv[1];
    uint16_t port = atoi(argv[2]);

    //创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1) {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    // 填写client需要连接的server 信息ip + port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client 和 server 进行连接
    if (-1 == connect(clientfd, (sockaddr*)&server, sizeof(sockaddr_in))) {
        cerr << "client server error" << endl;
        close(clientfd);
        exit(-1);
    }

    // main 线程用于接收用户输入，负责发送数据
    for (;;) {
        // 显示首页面菜单 登录、注册、退出
        cout << "=======================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "========================" << endl;
        cout << "choice: ";
        int choice = 0;
        cin >> choice; //从缓冲区读取一个整数需要清除残留的回车1,2,3
        cin.get(); // 读掉缓冲区残留的回车
        
        switch (choice) {
            case 1: {
                //login业务
                int id = 0; // 输入id
                char pwd[50] = {0}; // 输入密码
                cout << "userid: ";
                cin >> id;
                cin.get(); // 读掉缓冲区残留的回车
                cout << "userpassword: ";
                cin.getline(pwd, 50);

                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = pwd;
                string request = js.dump();
                
                //cout << request << endl; // 添加输出信息
                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if (len == -1) {
                    cerr << "send login msg error:" << request << endl;
                } else {
                    char buffer[1024] = {0};
                    len  = recv(clientfd, buffer, 1024, 0);
                    if (-1 == len) {
                        cerr << "recv login response error" << endl;
                    } else {
                        // cout << buffer << endl; // 添加输出信息
                        json responsejs = json::parse(buffer);
                        if (0 != responsejs["errno"].get<int>()) {
                            cerr << responsejs["errmsg"];
                        } else {
                            // 登录成功
                            // 记录当前用户的id 和 name 
                            g_currentUser.setId(responsejs["id"].get<int>());
                            g_currentUser.setName(responsejs["name"]);
                            
                            // 有些人有好友，有些人没有好友
                            // 记录当前用户的好友列表信息
                            if (responsejs.contains("friends")) {
                                //初始化
                                g_currentUserFriendList.clear();
                                vector<string> vec = responsejs["friends"];
                                for (string& str : vec) {
                                    json js = json::parse(str);
                                    User user;
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    g_currentUserFriendList.push_back(user); // 注销再登录可能又会放入
                                }
                            }

                            // 记录当前用户的群组列表信息
                            if (responsejs.contains("groups")) {
                                //初始化
                                g_currentUserGroupList.clear();
                                vector<string> vec1 = responsejs["groups"];
                                for (string& groupstr : vec1) {
                                    json grpjs = json::parse(groupstr);
                                    Group group;
                                    group.setId(grpjs["id"]);
                                    group.setname(grpjs["groupname"]);
                                    group.setdesc(grpjs["groupdesc"]);
                                    
                                    vector<string> vec2 = grpjs["users"];
                                    for (string& userstr : vec2) {
                                        GroupUser user;
                                        json js = json::parse(userstr);
                                        user.setId(js["id"].get<int>());
                                        user.setName(js["name"]);
                                        user.setRole(js["role"]);
                                        group.getUsers().push_back(user);
                                    }

                                    g_currentUserGroupList.push_back(group);
                                }
                            }
                            // 显示登录用户的基本信息
                            showCurrentUserData();
                            
                            // 显示当前用户的离线消息 个人聊天信息或者群组信息
                            if (responsejs.contains("offlinemsg")) {
                                vector<string> vec = responsejs["offlinemsg"];
                                for (string& str: vec) {
                                    json js = json::parse(str);
                                    // time + [id] + name + "said:" + xxx
                        
                                    // 如果接收到的消息类型是聊天消息，那么就需要将聊天消息进行返回
                                    if (ONE_CHAT_MSG == js["msgid"].get<int>()) {
                                        cout << js["time"] << "[" << js["id"] <<"]" 
                                        <<js["name"].get<string>() <<" said: " << js["msg"].get<string>() << endl;
                                    }
                                    else {
                                        cout << "群消息[" << js["groupid"] << "]: " << js["time"].get<string>() 
                                            << "[" << js["id"] << "]" 
                                            << js["name"].get<string>() <<
                                            " said: " <<js["msg"].get<string>() << endl;
                                        continue;
                                    }
                                    
                                    
                                }
                            }
                            // 登录成功，启动接收线程负责接收数据
                            // 可变参模版， 创建线程
                            // 该线程只启动一次
                            static int readThreadnumber = 0;
                            if (readThreadnumber == 0) {
                                std::thread readTask(readTaskHandler, clientfd); //pthread_create
                                // 设置为分离线程，线程运行完成后PCB资源自动由内核进行回收
                                readTask.detach(); //pthread_detach
                                readThreadnumber++;
                            }
                            
                            // 进入聊天主菜单页面
                            isMainMenuRunning = true;
                            mainMenu(clientfd);
                        }
                    }
                }
            }
            break;
            case 2: {
                // register 业务
                char name[50] = {0};
                char pwd[50] = {0};
                cout << "username: ";
                // 为什么不能用cin >>,因为cin >> 遇到空格后会导致结束，和scanf一样，应该改为cin.getline(),遇到回车结束
                cin.getline(name, 50);
                cout << "userpassword: ";
                cin.getline(pwd, 50);

                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = pwd;
                string request = js.dump(); // 序列化为字符串
                // 发送出去
                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1,0);
                if (len == -1) {
                    cerr << "send reg msg error:" << request << endl;
                } else {
                    char buffer[1024] = {0};
                    len = recv(clientfd, buffer, 1024, 0);
                    if (-1 == len) {
                        cerr << "recv reg response error" << endl;
                    } else {
                        json responsejs = json::parse(buffer);
                        if (0 != responsejs["errno"].get<int>()) {
                            // 注册失败
                            cerr << name << "is already exist, register error!" << endl;
                        } else {
                            // 注册成功
                            cout << name << " register success, userid is " 
                                << responsejs["id"] << ". do not forget it!" << endl;
                        }
                    }
                }
            }
            break;
            case 3: {
                // quit 业务
                close(clientfd);
                exit(0);
            }
            default : 
                cerr << "invalid input!" << endl;
                break;
        }
    }
    return 0;
}
// 接受线程
void readTaskHandler(int clientfd) {
    for (;;) {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0); //阻塞
        if (-1 == len || 0 == len) {
            close(clientfd);
            exit(-1);
        }
        // 接收ChatServer 转发的数据，反序列化成为json数据对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        // 如果接收到的消息类型是聊天消息，那么就需要将聊天消息进行返回
        if (ONE_CHAT_MSG == msgtype) {
            cout << js["time"].get<string>() << "[" << js["id"] << "]" 
                << js["name"].get<string>() <<
                 " said: " <<js["msg"].get<string>() << endl;
            continue;
        }
        if(GROUP_CHAT_MSG == msgtype) {
            cout << "群消息[" << js["groupid"] << "]: " << js["time"].get<string>() 
                << "[" << js["id"] << "]" 
                << js["name"].get<string>() <<
                 " said: " <<js["msg"].get<string>() << endl;
            continue;
        }
    }
}
// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime() {
    return "";
}



void showCurrentUserData() {
    cout << "======================login user=====================" << endl;
    cout << "current login user => id: " << g_currentUser.getId() 
            << " name: " << g_currentUser.getName() << endl;
    cout << "======================friend list=====================" << endl;   
    if (!g_currentUserFriendList.empty()) {
        for (User& user : g_currentUserFriendList) {
            // 打印好友的信息
            
            cout << user.getId() <<" " << user.getName() <<" "
                <<user.getState()<<endl;
        }
    }
    cout << "=======================group list=====================" << endl;
    if (!g_currentUserGroupList.empty()) {
        for(Group& group : g_currentUserGroupList) {
            // 打印群组的信息
            cout << group.getId() << " " << 
                group.getname() << " " << group.getdesc() << endl;

            // 打印群组成员的信息
            for (GroupUser & user : group.getUsers()) {
                cout << user.getId() << " " << user.getName() << " " 
                << user.getState() << " " << user.getRole() << endl;
            }
        }
    }
    cout << "========================================================" << endl;
}
// "help" command Handler
void help(int fd = 0, string = "");

// "chat" command Handler
void chat(int, string);

// "addfriend" command Handler
void addfriend(int, string);

// "creategroup" command Handler
void creategroup(int, string);

// "addgroup" command Handler
void addgroup(int, string);

// "groupchat" command Handler
void groupchat(int, string);

// "loginout" command Handler
void loginout(int, string);



unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式为chat:friendid:message"},
    {"addfriend", "添加好友，格式为addfriend:friendid"},
    {"creategroup", "创建群组，格式为creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式为addgroup:groupid"},
    {"groupchat","群聊，格式为groupchat:groupid:message"},
    {"loginout","注销，格式loginout"},
};



//注册系统所支持的客户端命令处理
// 方法为什么需要传递一个int 和 string 类型？ 因为
unordered_map<string, function<void(int,string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout},
};





// 主聊天页面程序
void mainMenu(int clientfd) {
    help(); //函数参数有默认值
    char buffer[1024] = {0};

    //不断地输出
    while (isMainMenuRunning) {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command;
        int idx = commandbuf.find(":"); //找到了就返回起始下标，找不到就返回-1
        if (-1 == idx) {
            command = commandbuf; //直接输出
        } else {
            command = commandbuf.substr(0, idx); // command 的名字 如chat这些
        }
        auto it = commandHandlerMap.find(command);
        // 说明没找到输入的命令，输出错误信息
        if (it == commandHandlerMap.end()) {
            cerr << "invalid input command" << endl;
            continue;
        }
        // 调用相应的命令的时间处理回调，mainMenu对修改封闭，添加新功能不需要修改该函数
        //cout << commandbuf.substr(idx + 1, commandbuf.size() - idx) << endl;
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));// 调用处理命令的方法

    }
}
// "help" command Handler
void help(int, string) {
    cout << " << show command list >>" << endl;
    for (auto &p : commandMap) {
        cout << p.first << " : " <<p.second << endl;
    }
    cout << endl;
}

// "chat" command Handler
void chat(int clientfd, string str) {
    int idx = str.find(":"); // friendid:message
    if (idx == -1) {
        cerr << "chat command invalid" << endl;
        return;
    }
    int friendid = atoi(str.substr(0, idx).c_str());
    string messages = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = messages;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1) {
        cerr << "send chat msg error ->" << buffer << endl;
    }    

}

// "addfriend" command Handler
void addfriend(int clientfd, string str) {
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);

    if (len == -1) {
        cerr << "send friend msg error ->" << buffer << endl;
    }
}


// "creategroup" command Handler
void creategroup(int clientfd, string str) {
    //groupname:groupdesc
    int idx = str.find(":");
    if (idx == -1) {
        cerr << "creategroup command invalid" << endl;
        return;
    }
    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;

    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1) {
        cerr << "send chat msg error ->" << buffer << endl;
    }    
}

// "addgroup" command Handler
void addgroup(int clientfd, string str) {
    //addgroup:groupid
    int groupid = atoi(str.c_str());

    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1) {
        cerr << "send addgroup msg error ->" << buffer << endl;
    }
}

// "groupchat" command Handler
void groupchat(int clientfd, string str) {
    //groupchat:groupid:message
    int idx = str.find(":");
    if (idx == -1) {
        cerr << "groupchat command invalid" << endl;
        return;
    }
    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);
    
    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();
    
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1) {
        cerr << "send groupchat msg error ->" << buffer << endl;
    }

}

// "loginout" command Handler
void loginout(int clientfd, string str) {
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);

    if (len == -1) {
        cerr << "send loginout msg error ->" << buffer << endl;
    } else {
        isMainMenuRunning = false;
    }

}

