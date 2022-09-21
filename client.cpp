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
    cout<<"连接服务器成功"<<endl;
    HandleClient(sock);

    return;
}
void client::SendMsg(int conn) {
    while (1){
        string str;
        cin>>str;

        if (conn>0){
            str="content:"+str;
        } else if (conn<0){
            str="gr_message:"+str;
        }
        int ret = send(abs(conn), str.c_str(), str.length(),0);
        if (str=="content:exit"||ret<=0){
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
        cout<<buffer<<endl;
    }
}
void client::HandleClient(int conn) {
    int choice;
    string login_name,name;
    bool if_login= false;

    cout<<" ------------------\n";
    cout<<"|                  |\n";
    cout<<"| 请输入你要的选项:|\n";
    cout<<"|    0:退出        |\n";
    cout<<"|    1:登录        |\n";
    cout<<"|                  |\n";
    cout<<" ------------------ \n\n";

    while (1){
        cin>>choice;
        if (choice==0){
            break;
        }else if (choice==1&&!if_login){
            cout<<"用户名：";
            cin>>name;
            string str="login"+name;
            send(sock,str.c_str(),str.length(),0);

            char buffer[1000];
            memset(buffer,0, sizeof(buffer));
            recv(sock,buffer, sizeof(buffer),0);

            string recv_str(buffer);
            if (recv_str.substr(0,3)=="wel"){
                if_login = true;
                login_name=name;
                cout<<"登录成功"<<endl;
                break;
            } else
                cout<<"登录失败"<<endl;

        }
    }

    while (if_login){
        if (if_login){
            cout<<"        欢迎回来,"<<login_name<<endl;
            cout<<" -------------------------------------------\n";
            cout<<"|                                           |\n";
            cout<<"|          请选择你要的选项：               |\n";
            cout<<"|              0:退出                       |\n";
            cout<<"|              1:发起单独聊天               |\n";
            cout<<"|              2:发起群聊                   |\n";
            cout<<"|                                           |\n";
            cout<<" ------------------------------------------- \n\n";
        }


        cin>>choice;
        if (choice==0){
            break;
        } else if (choice==1){
            cout<<"请输入对面用户名：";
            string target_name,content;
            cin>>target_name;
            string sendstr("target:"+target_name+"from:"+login_name);

            send(sock,sendstr.c_str(), sendstr.length(),0);
            cout<<"请输入您想说的话（exit退出）：\n";
            thread t1(client::SendMsg,conn);
            thread t2(client::RecvMsg,conn);
            t1.join();
            t2.join();
        } else if (choice==2){
            cout<<"请输入群号：";
            int num;
            cin>>num;
            string sendstr("group:"+ to_string(num));
            send(sock,sendstr.c_str(),sendstr.length(),0);
            cout<<"请输入你想说的话(输入exit退出)：\n";
            thread t1(client::SendMsg,-conn);
            thread t2(client::RecvMsg,conn);
            t1.join();
            t2.join();

        }
    }
    close(sock);
}

