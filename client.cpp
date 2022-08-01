#include "client.h"

client::client(int port, string ip) :server_port(port),server_ip(ip){}

client::~client() {
    close(sock);
}
void client::run() {
    sock = socket(AF_INET,SOCK_STREAM,0);

    struct sockaddr_in servaddr;
    memset(&servaddr,0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(server_ip.c_str());
    servaddr.sin_port = htons(server_port);

    if (connect(sock,(struct sockaddr *)&servaddr, sizeof(servaddr))<0){
        perror("connect");
        exit(1);
    }
    cout<<"连接服务器成功\n";

    thread send_t(SendMsg,sock),recv_t(RecvMsg,sock);
    send_t.join();
    cout<<"发送线程已结束\n";

    recv_t.join();
    cout<<"接收线程已结束\n";
    return;
}
void client::SendMsg(int conn) {
    char sendbuf[100];
    while (1){
        memset(sendbuf,0, sizeof(sendbuf));
        cin>>sendbuf;
        int ret = send(conn, sendbuf, strlen(sendbuf),0);
        if (!strcmp(ret,"exit")||ret<=0){
            break;
        }
    }

}
void client::RecvMsg(int conn) {
    char buffer[1000];
    while (1){
        memset(buffer,0,sizeof(buffer));
        int len = recv(conn,buffer, sizeof(buffer),0);
        if (len<=0){
            break;
        }
        cout<<"收到服务器发来的信息："<<buffer<<endl;
    }
}

int main(){
    int sock_cli = socket(AF_INET,SOCK_STREAM,0);

    struct sockaddr_in servaddr;
    memset(&servaddr,0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(8023);

    if (connect(sock_cli,(struct sockaddr *)&servaddr, sizeof(servaddr))<0){
        perror("connect");
        exit(1);
    }
    cout<<"connect successfully"<<endl;

    char sendbuf[100];
    char recvbuf[100];
    while (1){
        memset(sendbuf,0, sizeof(sendbuf));
        cin>>sendbuf;
        send(sock_cli, sendbuf, strlen(sendbuf),0);
        if (strcmp(sendbuf,"exit")==0){
            break;
        }
    }
    close(sock_cli);
    return 0;

}
