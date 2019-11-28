#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define PORT 62320 //Порт сервера
#define IP_SERVER "192.168.139.3" //Адрес сервера
#define SIZE_MSG 100

int readN(int socket, char* buf);


int main(int argc, char** argv) {

    printf("Welcome, traveller!\n"); fflush(stdout);

    struct sockaddr_in peer;
    peer.sin_family = AF_INET;

    char inputBuf[SIZE_MSG];
    int sock = -1;
    int rc = -1;

    for(;;){
        printf("Please, enter IP address:\n"); fflush(stdout);
        memset(inputBuf, 0, sizeof(inputBuf));
        fgets(inputBuf, sizeof(inputBuf), stdin);
        inputBuf[strlen(inputBuf) - 1] = '\0';
        peer.sin_addr.s_addr = inet_addr(inputBuf);
        printf("Please, enter port (Try port = 62320):\n"); fflush(stdout);
        memset(inputBuf, 0, sizeof(inputBuf));
        fgets(inputBuf, sizeof(inputBuf), stdin);
        inputBuf[strlen(inputBuf) - 1] = '\0';
        peer.sin_port = htons(atoi(inputBuf));

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            printf("ERROR: Can't create socket! Try again.\n"); fflush(stdout);
            continue;
        }

        int rc = connect(sock, (struct sockaddr*) &peer, sizeof(peer));
        if(rc == -1){
            perror("ERROR: Can't connect to server! Try again.\n"); fflush(stdout);
            continue;
        } else {
            printf("Connected.\n"); fflush(stdout);
            break;
        }
    }

    char msg[SIZE_MSG] = {0};
    printf("Input (\'/help\' to help): \n"); fflush(stdout);
    for(;;){
        fgets(inputBuf, sizeof(inputBuf), stdin);
        inputBuf[strlen(inputBuf) - 1] = '\0';

        if(!strcmp("/help", inputBuf)){
            printf("HELP:\n");
            printf("\'/quit or /q\' to shutdown;\n");
            fflush(stdout);
        } else if((!strcmp("/quit", inputBuf)) || (!strcmp("/q", inputBuf))) {
           // send(sock, inputBuf, sizeof(inputBuf), 0);

            shutdown(sock, 2);
            break;
        } else {
            send(sock, inputBuf, sizeof(inputBuf), 0);

            if (readN(sock, msg) <= 0) {
                printf("Out of service...\n"); fflush(stdout);
                close(sock);
                break;
            } else {
                printf("Message from SERVER: %s\n", msg); fflush(stdout);
            }

            memset(inputBuf, 0, 100);
        }
    }

    printf("ENDED CLIENT!\n"); fflush(stdout);

    return 0;
}

int readN(int socket, char* buf){
    int result = 0;
    int readBytes = 0;
    int sizeMsg = SIZE_MSG;
    readBytes = recv(socket, buf, sizeMsg, 0);
    result += readBytes;
    sizeMsg -= readBytes;
    while(sizeMsg > 0){
        readBytes = recv(socket, buf + result, sizeMsg, 0);
        if (readBytes <= 0){
            return -1;
        }
        result += readBytes;
        sizeMsg -= readBytes;
    }
    return result;
}