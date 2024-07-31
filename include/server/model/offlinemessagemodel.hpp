#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H



#include <string>
#include <list>
#include <vector>
using namespace std;

class OfflineMsgModel {
public:
    //存储用户离线消息
    void insert(int userid, string msg);
    
    // 删除用户的离线消息，返回后删除防止上线一直发送
    void remove(int userid);

    // 查询用户的离线消息
    vector<string> query(int userid);


};









#endif