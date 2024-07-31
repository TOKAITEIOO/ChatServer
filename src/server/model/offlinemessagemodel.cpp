#include "offlinemessagemodel.hpp"
#include "db.h"


void OfflineMsgModel::insert(int userid, string msg) {
    //1. 组装SQL 语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlineMessage(userid,message) values(%d,'%s')", userid, msg.c_str());
    MySQL mysql;
    if(mysql.connect()) {
        mysql.update(sql);
    }
    return;
}


void OfflineMsgModel::remove(int userid) {
    //1. 组装SQL 语句
    char sql[1024] = {0};
    sprintf(sql,"delete from offlineMessage where userid = %d", userid);
    MySQL mysql;

    if (mysql.connect()) {
        mysql.update(sql);
    }
    return;
}


vector<string> OfflineMsgModel::query(int userid) {
    //1. 组装SQL 语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlineMessage where userid = %d", userid);
    MySQL mysql;
    vector<string> vec;
    if(mysql.connect()) {
        MYSQL_RES *res =  mysql.query(sql); // 这里的资源是mysql开辟出来的

        if (res != nullptr) {
            MYSQL_ROW row;
            // 把userid 用户所有的离线消息放入vec中返回
            while ((row = mysql_fetch_row(res)) != nullptr) {
                vec.push_back(row[0]);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}
