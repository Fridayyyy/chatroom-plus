//
// Created by 常贵杰 on 2022/8/1.
//

#ifndef SERVER_H
#define SERVER_H

#include "global.h"

class server{
private:
    int server_port;
    int server_sockfd;//设为listen状态的套接字描述符
    string server_ip;
    static vector<bool> sock_arr;//保存所有套接字描述符
    static unordered_map<string,int> name_sock_map;
    static pthread_mutex_t name_sock_mutex;

    static unordered_map<int,set<int>> group_map;
    static pthread_mutex_t group_mutx;

public:
    server(int port,string ip);
    ~server();
    void run();
    static void RecvMsg(int epollfd,int conn);//子线程工作的静态函数
    static void HandleRequest(int epollfd,int conn,string str,
                              tuple<bool,string,string,int,int>&info);
    static void setnonblocking(int conn);
};

#endif //CHATROOM_PLUS_SERVER_H
