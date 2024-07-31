#ifndef GROUP_H
#define GROUP_H

#include "groupuser.hpp"
#include <vector>
#include <string>
using namespace std;
class Group {
public:
    Group(int id = -1, string name ="", string desc="") {
        this->id = id;
        this->name = name;
        this->desc = desc;
    }
    void setId(int id) {this->id = id;}
    void setname(string name) {this->name = name;}
    void setdesc(string desc) {this->desc = desc;}

    int getId() {return this->id;}
    string getname() {return this->name;}
    string getdesc() {return this->desc;}

    vector<GroupUser>& getUsers() {return this->users;}

private:
    int id;
    string name;
    string desc;
    vector<GroupUser> users;
};


#endif