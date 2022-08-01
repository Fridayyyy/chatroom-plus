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

using namespace std;

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
