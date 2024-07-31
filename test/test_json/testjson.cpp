#include "json.hpp"
using json = nlohmann::json;
#include <iostream>
#include <vector>
#include <map>
#include <string>
using namespace std;


string func1() {
    // Json 实例化示例1
    // 无序的映射
    json js;
    js["id"] = {1,2,3,4,5};
    js["msg_type"] = 2;
    js["from"] = "Zhang San";
    js["to"] = "Li SI";
    js["msg"] = "hello,what are you doing now";
    string sentBuf = js.dump();
    cout << sentBuf.c_str() << endl;
    return sentBuf;
}
// 序列化示例2
string func2() {
    // Json 实例化示例2
    json js;
    // 添加数组
    js["id"] = {1,2,3,4,5};
    // 添加key-value
    js["name"] = "zhang san";
    // 添加对象
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["liu shuo"] = "hello china";
    // 上面等同于下面这句一次性添加数组对象
    js["msg"] = {{"zhang san", "hello world"}, {"liu shuo", "hello china"}};
    cout << js << endl;
    string sentBuf = js.dump();
    return sentBuf;
}

// json 的序列化示例代码3
string func3() {
    json js;

    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    js["list"] = vec;
    map<int, string> m;
    m.insert({1,"黄山"});
    m.insert({2,"华山"});
    m.insert({3,"泰山"});

    js["path"] = m;
    cout << js << endl;
    string sentBuf = js.dump();
    return sentBuf;
}


int main() {
    string receiveBuf = func1();
    //json 字符串反序列化成 json 字符串--> 反序列化 ---> 数据对象
    json buf = json::parse(receiveBuf);

    cout << buf["msg_type"] << endl;
    cout << buf["from"] << endl;
    cout << buf["to"] << endl;
    cout << buf["msg"] << endl;

    return 0;
}