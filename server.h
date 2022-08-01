//
// Created by 常贵杰 on 2022/8/1.
//

#ifndef SERVER_H
#define SERVER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <iostream>
#include <thread>
#include <vector>

using namespace std;
class server{
private:
    int server_port;
    int server_sockfd;//设为listen状态的套接字描述符
    string server_ip;
    vector<int> sock_arr;//保存所有套接字描述符

public:
    server(int port,string ip);
    ~server();
    void run();
    static void RecvMsg(int conn);//子线程工作的静态函数
};

#endif //CHATROOM_PLUS_SERVER_H
