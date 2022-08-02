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
    HandleClient(sock);

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
        if (strcmp(sendbuf,"exit")||ret<=0){
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
void client::HandleClient(int conn) {
    int choice;
    string name,pass,pass1

    cout<<" ------------------\n";
    cout<<"|                  |\n";
    cout<<"| 请输入你要的选项:|\n";
    cout<<"|    0:退出        |\n";
    cout<<"|    1:登录        |\n";
    cout<<"|    2:注册        |\n";
    cout<<"|                  |\n";
    cout<<" ------------------ \n\n";

    while (1){
        cin>>choice;
        if (choice==0){
            break;
        } else if (choice==2){
            cout<<"name: ";
            cin>>name;
            while (1){
                cout<<"password: ";
                cin>>pass;
                cout<<"confirm password: ";
                cin>>pass1;
                if(pass==pass1)
                    break;
                else
                    cout<<"两次密码不一致!\n\n";
            }
            name="name:"+name;
            pass="pass:"+pass;
            string str=name+pass;
            send(conn,str.c_str(),str.length(),0);
            cout<<"register successfully"<<endl;
            cout<<"move on"<<endl;
        }
    }
}

