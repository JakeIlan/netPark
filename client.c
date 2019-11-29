#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define PORT 1234 //Порт сервера
#define IP_SERVER "127.0.0.1" //Адрес сервера
#define SIZE_MSG 100

int readN(int socket, char *buf);

int commandHandler(int socket, char *buf, int type);

int main(int argc, char **argv) {

    printf("Welcome, traveller!\n");
    fflush(stdout);

    struct sockaddr_in peer;
    peer.sin_family = AF_INET;

    char inputBuf[SIZE_MSG];
    int sock = -1;
    int rc = -1;

    for (;;) {
//        printf("Please, enter IP address:\n");
//        fflush(stdout);
//
//        memset(inputBuf, 0, sizeof(inputBuf));
//
//        fgets(inputBuf, sizeof(inputBuf), stdin);
//        inputBuf[strlen(inputBuf) - 1] = '\0';
//        peer.sin_addr.s_addr = inet_addr(inputBuf);
        peer.sin_addr.s_addr = inet_addr(IP_SERVER);

//        printf("Please, enter port (Try port = %d):\n", PORT);
//        fflush(stdout);
//
//        memset(inputBuf, 0, sizeof(inputBuf));
//
//        fgets(inputBuf, sizeof(inputBuf), stdin);
//        inputBuf[strlen(inputBuf) - 1] = '\0';
//        peer.sin_port = htons(atoi(inputBuf));
        peer.sin_port = htons(PORT);

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            printf("ERROR: Can't create socket! Try again.\n");
            fflush(stdout);
            break;
        }

        int rc = connect(sock, (struct sockaddr *) &peer, sizeof(peer));
        if (rc == -1) {
            perror("ERROR: Can't connect to server! Try again.\n");
            fflush(stdout);
            break;
        } else {
            printf("Connected.\n");
            fflush(stdout);
            break;
        }
    }

    char msg[SIZE_MSG] = {0};
    char msg_buf[SIZE_MSG] = {0};
    printf("Input (\'/help\' to help): \n");
    fflush(stdout);
    for (;;) {
        int type = 0;

        fgets(inputBuf, sizeof(inputBuf), stdin);
        inputBuf[strlen(inputBuf) - 1] = '\0';

        if (!strcmp("/help", inputBuf)) {
            type = 0;
            printf("%d\n", type);
            printf("HELP:\n");
            printf("\'/park LIC\' to park a car with LIC license plate\n");
            printf("\'/release\' to request receipt\n");
            printf("\'/pay NUM\' to pay NUM\n");
            printf("\'/quit or /q\' to leave after paying receipt\n");
            fflush(stdout);
            if (commandHandler(sock, msg, type) < 0) {
                printf("Out of service...\n");
                fflush(stdout);
                close(sock);
                break;
            }
        } else if ((!strcmp("/quit", inputBuf)) || (!strcmp("/q", inputBuf))) {
            type = 1;
            printf("%d\n", type);
            fflush(stdout);
            shutdown(sock, 2);
            break;
        } else if (!strcmp("/release", inputBuf)) {
            type = 2;
            printf("%d\n", type);
            fflush(stdout);

            send(sock, inputBuf, sizeof(inputBuf), 0);
            if (commandHandler(sock, msg, type) < 0) {
                printf("Out of service...\n");
                fflush(stdout);
                close(sock);
                break;
            }

        } else {

            strcpy(msg_buf, inputBuf);
            char *sep = " ";
            char *str = strtok(msg_buf, sep);
            if (str == NULL) {
                printf("Wrong syntax! Try /help to see command menu.\n");
                fflush(stdout);
                continue;
            }
            if (!strcmp("/park", str)) {
                type = 3;
                printf("%d\n", type);
                fflush(stdout);
                str = strtok(NULL, sep);
                if (str != NULL) {
                    send(sock, inputBuf, sizeof(inputBuf), 0);
                    if (commandHandler(sock, msg, type) < 0) {
                        printf("Out of service...\n");
                        fflush(stdout);
                        close(sock);
                        break;
                    }
                }
            } else if (!strcmp("/pay", str)) {
                type = 4;
                printf("%d\n", type);
                fflush(stdout);
                str = strtok(NULL, sep);
                if (str != NULL) {
                    int pay = atoi(str);

                    if (str[0] != '0' && pay == 0) {
                        printf("Illegal format! Use /pay NUMBER.\n");
                        fflush(stdout);
                        continue;
                    }
                    send(sock, inputBuf, sizeof(inputBuf), 0);
                    if (commandHandler(sock, msg, type) < 0) {
                        printf("Out of service...\n");
                        fflush(stdout);
                        close(sock);
                        break;
                    }
                }
            }

        }

        memset(inputBuf, 0, 100);
    }

    printf("ENDED CLIENT!\n");
    fflush(stdout);

    return 0;
}

int commandHandler(int sock, char *msg, int type) {
    printf("Entering switch\n");
    fflush(stdout);
    if (readN(sock, msg) <= 0) {
        return readN(sock, msg);
    } else {
        printf("Entering listener switch...\n");
        fflush(stdout);
        switch (type) {
            case 0: {
                printf("%s\n", msg);
                fflush(stdout);
                return type;
            }//Undefined command or /help

            case 1: {
                printf("%s\n", msg);
                fflush(stdout);
                return type;
            }// /quit

            case 2: {
                printf("Your time is: %s\n", msg);
                printf("You have to /pay at least %s$ to leave\n", msg);
                fflush(stdout);
                return type;
            }// /release

            case 3: {
                printf("%s\n", msg);
                fflush(stdout);
                return type;
            }// /park


            case 4: {
                printf("%s\n", msg);
                fflush(stdout);
                return type;
            }// /pay


            default:
                printf("Ti kak eto sdelal\n");
        }
    }
}


int readN(int socket, char *buf) {
    int result = 0;
    int readBytes = 0;
    int sizeMsg = SIZE_MSG;
    readBytes = recv(socket, buf, sizeMsg, 0);
    result += readBytes;
    sizeMsg -= readBytes;
    while (sizeMsg > 0) {
        readBytes = recv(socket, buf + result, sizeMsg, 0);
        if (readBytes <= 0) {
            return -1;
        }
        result += readBytes;
        sizeMsg -= readBytes;
    }
    return result;
}