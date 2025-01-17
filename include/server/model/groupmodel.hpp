#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"


class GroupModel {
public:
    // 创建群组
    bool createGroup(Group& group);
    // 加入群组
    void addGroup(int userid, int groupid, string grouprole);
    // 查询用户所在群组信息
    vector<Group> queryGroups(int userid);
    // 根据指定的groupid 查询群组用户的id列表，除了userid自己，
    //  主要用于用户群聊业务给群组其他成员群发消息
    vector<int> queryGroupUsers(int userid, int groupid);
private:

};





#endif