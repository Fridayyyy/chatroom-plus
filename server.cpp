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
pthread_mutex_t server::group_mutx;
unordered_map<int,set<int>> server::group_map;

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
    int LISTENQ=200;
    int i,maxi,listenfd,connfd,sockfd,epfd,nfds;
    ssize_t n;
    socklen_t clilen;
    struct epoll_event ev,events[10000];
    epfd=epoll_create(10000);
    struct sockaddr_in clientaddr;
    struct sockaddr_in serveraddr;
    listenfd = socket(PF_INET,SOCK_STREAM,0);
    setnonblocking(listenfd);
    ev.data.fd=listenfd;
    ev.events=EPOLLIN|EPOLLET;
    epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");//此处设为服务器的ip
    serveraddr.sin_port=htons(8023);
    bind(listenfd,(struct sockaddr *)&serveraddr, sizeof(serveraddr));
    listen(listenfd,LISTENQ);
    clilen=sizeof(clientaddr);
    maxi=0;

    while (1){
        cout<<"--------------------------"<<endl;
        cout<<"epoll_wait阻塞中"<<endl;
        //等待epoll事件的发生
        nfds=epoll_wait(epfd,events,10000,-1);
        cout<<"epoll_wait返回，有事件发生"<<endl;

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd==listenfd){
                connfd = accept(listenfd,(sockaddr *)&clientaddr,clilen);
                if(connfd<0){
                    perror("connfd<0");
                    exit(1);
                }
                else{
                    cout<<"用户"<<inet_ntoa(clientaddr.sin_addr)<<"正在连接\n";
                }
                ev.data.fd=connfd;
                ev.events=EPOLLIN|EPOLLET|EPOLLONESHOT;
                setnonblocking(connfd);
                epoll_ctl(epfd,EPOLL_CTL_ADD,connfd,&ev);

            }else if(events[i].events&EPOLLIN){
                sockfd=events[i].data.fd;
                events[i].data.fd=-1;

                cout<<"接收到读事件"<<endl;

                string recv_str;
                boost::asio::post(boost::bind(RecvMsg,epfd,sockfd));
            }
        }
    }
    close(listenfd);

}
void server::RecvMsg(int epollfd,int conn) {
    //if_login,login_name,target_name,target_conn
    tuple<bool,string,string,int,int> info;

    get<0>(info)=false;
    get<3>(info)=-1;

    string recv_str;

    while (1){
        char buf[10];
        memset(buf,0, sizeof(buf));
        int ret = recv(conn,buf, sizeof(buf),0);

        if (ret<0){
            cout<<"recv返回值小于0"<<endl;
            //对于非阻塞IO，下面的事件成立标识数据已经全部读取完毕
            if((errno == EAGAIN) || (errno == EWOULDBLOCK)){
                printf("数据读取完毕\n");
                cout<<"接收到的完整内容为："<<recv_str<<endl;
                cout<<"开始处理事件"<<endl;
                break;
            }
            cout<<"errno:"<<errno<<endl;
            return;
        } else if (ret==0){
            cout<<"recv返回值为0"<<endl;
            return;
        } else{
            printf("接收到内容如下: %s\n",buf);
            string tmp(buf);
            recv_str+=tmp;
        }
    }
    string str=recv_str;
    HandleRequest(epollfd,conn,str,info);
}
void server::HandleRequest(int epollfd,int conn, string str,
                           tuple<bool,string,string,int,int> &info) {
    char buffer[1000];
    string name,pass;

    bool if_login=get<0>(info);
    string login_name=get<1>(info);
    string target_name=get<2>(info);
    int target_conn=get<3>(info);
    int group_num=get<4>(info);

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
        string target=str.substr(7,pos1-7),from=str.substr(pos1+4);
        target_name=target;
        if (name_sock_map.find(target)==name_sock_map.end())
            cout<<"源用户为:"<<login_name<<",目标用户："<<target_name<<" 仍未登录";
        else{
            cout<<"源用户"<<login_name<<"向目标用户"<<target_name<<"发起的私聊即将建立";
            cout<<",目标用户的套接字描述符为"<<name_sock_map[target]<<endl;
            target_conn=name_sock_map[target];
        }

    } else if (str.find("content:")!=str.npos){
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

    } else if (str.find("group:")!=str.npos){
        string recv_str(str);
        string num_str= recv_str.substr(6);
        group_num= stoi(num_str);
        cout<<"用户"<<login_name<<"绑定群聊号为："<<num_str<<endl;

        pthread_mutex_lock(&group_mutx);
        group_map[group_num].insert(conn);
        pthread_mutex_unlock(&group_mutx);
    } else if (str.find("gr_message:")!=str.npos){
        string send_str(str);
        send_str=send_str.substr(11);
        send_str="["+login_name+"]:"+send_str;
        cout<<"群聊信息："<<send_str<<endl;

        for (auto i:group_map[group_num]) {
            if (i!=conn)
                send(i,send_str.c_str(),send_str.length(),0);
        }
    }

    epoll_event event;
    event.data.fd=conn;
    event.events=EPOLLIN|EPOLLET|EPOLLONESHOT;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,conn,&event);


    get<0>(info)=if_login;//记录当前服务对象是否成功登录
    get<1>(info)=login_name;//记录当前服务对象的名字
    get<2>(info)=target_name;//记录目标对象的名字
    get<3>(info)=target_conn;//目标对象的套接字描述符
    get<4>(info)=group_num;

}
void server::setnonblocking(int sock){
    int opts;
    opts= fcntl(sock,F_GETFL);
    if(opts<0)
    {
        perror("fcntl(sock,GETFL)");
        exit(1);
    }
    opts=opts|O_NONBLOCK;
    if (fcntl(sock,F_SETFL,opts)<0){
        perror("fcntl(sock,SETFL,opts)");
        exit(1);
    }
}

