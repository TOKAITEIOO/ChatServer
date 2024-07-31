#ifndef GROUPUSER
#define GROUPUSER


#include"user.hpp"
#include<string>
using namespace std;


//群组用户，多了一个role 角色信息，从User 类直接继承，复用User的其他信息

class GroupUser: public User {
public:
    void setRole(string role) {this->role = role;}
    string getRole() {return this->role;}

private:
    string role;

};


#endif