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

#include "server.h"

using namespace std;
vector<bool> server::sock_arr(1000,false);
unordered_map<string,int> server::name_sock_map;
pthread_mutex_t server::name_sock_mutex;

server::server(int port, string ip):server_port(port),server_ip(ip){
    pthread_mutex_init(&name_sock_mutex,NULL);
}

server::~server() {
    for (int i = 0; i < sock_arr.size(); ++i) {
        if (sock_arr[i])
            close(i);
    }
    close(server_sockfd);
}

void server::run(){
    server_sockfd = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_addr.s_addr = inet_addr(server_ip.c_str());
    server_sockaddr.sin_port = htons(server_port);

    if (bind(server_sockfd,(struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr))==-1){
        perror("bind");
        exit(1);
    }
    if (listen(server_sockfd,20)==-1){
        perror("listen");
        exit(1);
    }
    struct sockaddr_in client_addr;
    socklen_t length = sizeof(client_addr);

    while (1){
        int conn = accept(server_sockfd,(struct sockaddr*)&client_addr,&length);
        if (conn<0){
            perror("connect");
            exit(1);
        }
        cout<<"文件描述符为 "<<conn<<" 的客户端成功连接"<<endl;
        sock_arr.push_back(conn);
        thread t(server::RecvMsg,conn);
        t.detach();//设置分离状态，不能用join，join会导致阻塞
    }
}
void server::RecvMsg(int conn) {
    //if_login,login_name,target_name,target_conn
    tuple<bool,string,string,int> info;

    get<0>(info)=false;
    get<3>(info)=-1;

    char buffer[1000];

    while (1){
        memset(buffer,0, sizeof(buffer));
        int len = recv(conn,buffer, sizeof(buffer),0);
        if (strcmp(buffer,"content:exit")==0||len<=0){
            close(conn);
            sock_arr[conn]=false;
            break;
        }
        cout<<"收到套接字描述符为"<<conn<<"发来的信息："<<buffer<<endl;

        string str(buffer);
        HandleRequest(conn,str,info);
    }
}
void server::HandleRequest(int conn, string str,
                           tuple<bool,string,string,int> &info) {
    char buffer[1000];
    string name,pass;

    bool if_login=get<0>(info);
    string login_name=get<1>(info);
    string target_name=get<2>(info);
    int target_conn=get<3>(info);

    if (str.find("login")!=str.npos){
        int p1=str.find("login"),p2=str.find("end");
        name = str.substr(p1+5,p2-3);
        login_name=name;
        if_login= true;
        pthread_mutex_lock(&name_sock_mutex);
        name_sock_map[login_name]=conn;
        pthread_mutex_unlock(&name_sock_mutex);
        string str1("welcome,"+login_name);
        send(conn,str1.c_str(),str1.length(),0);

    }else if (str.find("target:")!=str.npos){
        int pos1=str.find("from");
        string target=str.substr(7,pos1-1),from=str.substr(pos1+4);
        target_name=target;
        if (name_sock_map.find(target)==name_sock_map.end())
            cout<<"源用户为:"<<login_name<<",目标用户："<<target_name<<" 仍未登录";
        else{
            cout<<"源用户"<<login_name<<"向目标用户"<<target_name<<"发起的私聊即将建立";
            cout<<",目标用户的套接字描述符为"<<name_sock_map[target]<<endl;
            target_conn=name_sock_map[target];
        }

    } else if (str.find("content:")!=str.nops){
        if (target_conn==-1){
            cout<<"找不到目标用户"<<target_name<<"的套接字，将尝试重新寻找目标用户的套接字\n";
            if (name_sock_map.find(target_name)!=name_sock_map.end()){
                target_conn=name_sock_map[target_name];
                cout<<"重新查找目标用户套接字成功\n";
            }
            else{
                cout<<"查找仍然失败，转发失败！\n";
            }

        }
        string recv_str(str);
        string send_str=recv_str.substr(8);
        cout<<"用户"<<login_name<<"向"<<target_name<<"发送:"<<send_str<<endl;
        send_str="["+login_name+"]:"+send_str;

        send(target_conn,send_str.c_str(),send_str.length(),0);

    }
    get<0>(info)=if_login;//记录当前服务对象是否成功登录
    get<1>(info)=login_name;//记录当前服务对象的名字
    get<2>(info)=target_name;//记录目标对象的名字
    get<3>(info)=target_conn;//目标对象的套接字描述符
}

