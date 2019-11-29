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
#define SIZE_MSG 100

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct tInfo {
    pthread_t threadId;
    pthread_t timerThd;
    int socket;
    char* lic;
    int number;
    int time;
    int cond; // cond == 1 -> is parked; 0 -> new client, 2 -> released client
} *clients;

struct payLog { // TODO log
    struct tInfo client;
    int payment;
    int change;
} *pLog;

int summary;
int clientQuantity = 0;

void* clientHandler(void* args);
void* clientTimer(void* args);
void* connectionListener(void* args);
void* cashier(int payNum);
void* kickClient(int kickNum);
int readN(int socket, char* buf);


int main( int argc, char** argv) {

//Серверный сокет
    struct sockaddr_in listenerInfo;
    listenerInfo.sin_family = AF_INET;
    listenerInfo.sin_port = htons(PORT);
    listenerInfo.sin_addr.s_addr = htonl(INADDR_ANY); //INADDR_ANY INADDR_LOOPBACK

    int listener = socket(AF_INET, SOCK_STREAM, 0 ); //Прослушивающий сокет
    if ( listener < 0 ) {
        perror( "Can't create socket to listen: ");
        exit(1);
    }
    printf("Прослушивающий сокет создан.\n");
    fflush(stdout);

    int resBind = bind(listener, (struct sockaddr *)&listenerInfo, sizeof(listenerInfo));
    if (resBind < 0 ) {
        perror( "Can't bind socket" );
        exit(1);
    }
    printf("Прослушивающий сокет забинден.\n");
    fflush(stdout);

    if ( listen(listener, 2) ) { //Слушаем входящие соединения
        perror("Error while listening: ");
        exit(1);
    }
    printf("Начинаем слушать входящие соединения...\n");
    fflush(stdout);
//------------------------------------------------------

//Создание потока, который будет принимать входящие запросы на соединение
    pthread_t listenerThread;
    if (pthread_create(&listenerThread, NULL, connectionListener, (void *) &listener)){
        printf("ERROR: Can't create listener thread!"); fflush(stdout);
        exit(1);
    }

//Цикл чтения ввода с клавиатуры
    printf("Input (/help to help): \n"); fflush(stdout);
    char buf[100];
    for(;;) {
        bzero(buf, 100);
        fgets(buf, 100, stdin);
        buf[strlen(buf) - 1] = '\0';

        if(!strcmp("/help", buf)){
            printf("HELP:\n");
            printf("\'/lc\' to list clients\n");
            printf("\'/kick [number client]\' to kick client from server;\n");
            printf("\'/quit or /q\' to shutdown;\n");
            fflush(stdout);
        } else if(!strcmp("/lc", buf)){
            printf("Clients on-line:\n");
            printf("N TIME\n");

            pthread_mutex_lock(&mutex);
            for(int i = 0; i < clientQuantity; i++){
                if(clients[i].socket != -1)
                    printf("%d %d\n", clients[i].number, clients[i].time);
            }
            pthread_mutex_unlock(&mutex);

            fflush(stdout);
        } else if(!strcmp("/quit", buf) || !strcmp("/q", buf)) {
            shutdown(listener, 2);
            close(listener);
            pthread_join(listenerThread, NULL);
            break;
        } else {
            char *sep = " ";
            char *str = strtok(buf, sep);
            if(str == NULL) {
                printf("Illegal format!\n"); fflush(stdout);
                continue;
            }
            if(!strcmp("/kick", str)){
                str = strtok(NULL, sep);
                if (str != NULL) {
                    int kickNum = atoi(str);

                    if (str[0] != '0' && kickNum == 0) {
                        printf("Illegal format! Use /kick NUMBER.\n");
                        fflush(stdout);
                        continue;
                    }
                    kickClient(kickNum);
                }
            }
        }

    }

    printf("ENDED SERVER!\n"); fflush(stdout);

    return 0;
}

//Обработка одного клиента
void* clientHandler(void* args){ //TODO quit handler, parking condition 1, LIC

    pthread_mutex_lock(&mutex);
    int index = *((int*)args);
    int sock = clients[index].socket;
    pthread_mutex_unlock(&mutex);

    char msg[SIZE_MSG] = {0};
    for(;;) {
        if (readN(sock, msg) <= 0) {
            printf("Client №%d disconnect\n", index);
            fflush(stdout);
            shutdown(sock, 2);
            close(sock);
            pthread_mutex_lock(&mutex);
            clients[index].socket = -1;
            pthread_mutex_unlock(&mutex);
            break;

        } else if(!strcmp("/release" ,msg))   {
            printf("Client №%d sent leave request.\n", index);
            fflush(stdout);

            snprintf(msg, SIZE_MSG, "%d", clients[index].time);
            send(sock, msg, sizeof(msg), 0);
            clients[index].cond = 2;
          //  kickClient(index);
        } else {
            char *sep = " ";
            char *str = strtok(msg, sep);
            if(str == NULL) {
                printf("Undefined message from client %d.\n", index);
                fflush(stdout);
                send(sock, "Wrong command.\n", strlen("Wrong command.\n"), 0);
                continue;
            }
            if(!strcmp("/park", str)){
                if (clients[index].cond != 1){
                    str = strtok(NULL, sep);
                    if (str != NULL) {
                        clients[index].lic = str;
                        clients[index].cond = 1;
                        printf("LIC %s added\n", str);
                    }
                } else{
                    printf("New parking without release from Client %d\n", index);
                    fflush(stdout);
                    send(sock, "New parking without release\n", strlen("New parking without release\n"), 0);
                }

            } else if(!strcmp("/pay", str)){
                str = strtok(NULL, sep);
                if (str != NULL) {
                    int pay = atoi(str);

                    if (str[0] != '0' && pay == 0) {
                        strcpy(msg, "Dude, you have to /pay NUMBER.\n");
                        send(sock, msg, sizeof(msg), 0);
                        continue;
                    }
                    printf("Received payment %d$ from client %d.\n", pay, index);
                    fflush(stdout);
                }

            }

        }
        memset(msg, 0, sizeof(msg));
    }

    printf("ENDED CLIENT №%d!\n", index); fflush(stdout);
}

void* clientTimer(void* args) {
    pthread_mutex_lock(&mutex);
    int index = *((int*)args);
    pthread_mutex_unlock(&mutex);
   // if (clients[index].cond == 1) {
        for (;;) {
            if (clients[index].cond == 2){
                break;
            }
            clients[index].time++;
            sleep(1);
        }
 //   }

}

void* connectionListener(void* args){
    int listener = *((int*) args);

    int s;
    int indexClient;
    struct sockaddr_in a;
    int aLen = sizeof(a);
    for(;;){

        s = accept(listener, (struct sockaddr *) &a, &aLen);
        if (s <= 0){
            printf("STOPPING SERVER...\n"); fflush(stdout);

            pthread_mutex_lock(&mutex);
            for(int i = 0; i < clientQuantity; i++){
                shutdown(clients[i].socket, 2);
                close(clients[i].socket);
                clients[i].socket = -1;
            }
            for (int i = 0; i < clientQuantity; i++){
                pthread_join(clients[i].threadId, NULL);
                pthread_join(clients[i].timerThd, NULL);
            }
            pthread_mutex_unlock(&mutex);

            break;
        }

        pthread_mutex_lock(&mutex);
        clients = (struct tInfo*) realloc(clients, sizeof(struct tInfo) * (clientQuantity + 1));
        clients[clientQuantity].socket = s;
//        clients[clientQuantity].address = inet_ntoa(a.sin_addr);
//        clients[clientQuantity].port = a.sin_port;
        clients[clientQuantity].number = clientQuantity;
        clients[clientQuantity].cond = 0;
        clients[clientQuantity].time = 0;
        indexClient = clientQuantity;
        if(pthread_create(&(clients[clientQuantity].threadId), NULL, clientHandler, (void*) &indexClient)) {
            printf("ERROR: Can't create thread for client!\n"); fflush(stdout);
            continue;
        }
        if(pthread_create(&(clients[clientQuantity].timerThd), NULL, clientTimer, (void*) &indexClient)) {
            printf("ERROR: Can't create timer thread for client!\n"); fflush(stdout);
            continue;
        }

        pthread_mutex_unlock(&mutex);

        clientQuantity++;
    }

    printf("ENDED LISTENER CONNECTIONS!\n"); fflush(stdout);
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

void* cashier(int payNum) {
    //TODO cashier processing
}

void* kickClient(int kickNum){
    pthread_mutex_lock(&mutex);
    int check = 0;
    char msg[15] = "slish plati";
    for(int i = 0; i < clientQuantity; i++){
        if(clients[i].number == kickNum) {
            shutdown(clients[i].socket, 2);
            close(clients[i].socket);
            clients[i].socket = -1;
            check = 1;
            break;
        }
    }
    if (check == 0){
        printf("Client №%d not found, please try again.\n", kickNum);
    }
    pthread_mutex_unlock(&mutex);
}