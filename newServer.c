#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <sys/types.h>
#include <pthread.h>

int createServer(char *ip, int port)
{
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(port);

    printf("1. Creation\t");
    int fileDescripter = socket(AF_INET, SOCK_STREAM, 0);
    if (fileDescripter == -1)
    {
        printf("Failed\n");
        return (-1);
    }
    printf("Successful\n");

    printf("2. Binding\t");
    int binding = bind(fileDescripter, (struct sockaddr *)&address, sizeof(address));
    if (binding == -1)
    {
        printf("Failed\n");
        return (-1);
    }
    printf("Successful\n");

    printf("3. Listening\t");
    int listening = listen(fileDescripter, 10);
    if (listening == -1)
    {
        printf("Failed\n");
        return (-1);
    }
    printf("Successful\n");

    return (fileDescripter);
}

struct message
{
    char sender[100];
    char metadata[100];
    char text[1024];
};
const int size_of_message = sizeof(struct message);

struct client_property
{
    char *name;
    int index;

    int socketFD;
    struct sockaddr address;
    socklen_t length;

    pthread_t iniThread;
    pthread_t recvThread;
    pthread_t analThread;
    pthread_t sendThread;
} clients[5];
int clientCount = 0;
int lastClient = 0;

void analyzer(struct message *recvBuffer)
{
    if(strchr(recvBuffer->text, '@') == NULL)
    {
        for(int i = 0; i<lastClient; i++)
        {
            if(strstr(recvBuffer->sender, clients[i].name) != NULL)
            continue;
            send(clients[i].socketFD, recvBuffer, size_of_message, 0);
        }
    }
    else
    {
        for(int i = 0; i<lastClient; i++)
        {
            if(strstr(recvBuffer->text, clients[i].name) != NULL)
            {
                send(clients[i].socketFD, recvBuffer, size_of_message, 0);
            }
        }
    }
    free(recvBuffer);
}

void receiver(int *clientIndex)
{
    int index = *clientIndex;
    while (1)
    {
        struct message *recvBuffer = malloc(size_of_message);
        recv(clients[index].socketFD, recvBuffer, size_of_message, 0);
        pthread_create(&clients[index].analThread, NULL, (void *)analyzer, recvBuffer);
    }
}

int checkNameAvailibity(char *name)
{
    printf("in name check\n");
    int i;
    for (i = 0; i < lastClient; i++)
    {
        printf("inside for?\n");
        if (strcmp(clients[i].name, name) == 0)
        {
            printf("%s and %s duplicate name\n", clients[i].name, name);
            return (1);
        }
        if (strchr(clients[i].name, '@') != NULL || strchr(clients[i].name, '#') != NULL)
        {
            printf("%s and %s contains special char\n", clients[i].name, name);
            return (1);
        }
    }
    printf("returnning\n");
    return (0);
}

void initializer(int *clientIndex)
{
    int index = *clientIndex;
    struct message *sendBuffer = calloc(1, size_of_message);
    struct message *recvBuffer = calloc(1, size_of_message);

    strcpy(sendBuffer->sender, "server\0");
    strcpy(sendBuffer->metadata, "question\0");
    strcpy(sendBuffer->text, "Enter your name.. \0");

    int status = 0;
    do
    {
        if (status == 1)
            strcpy(sendBuffer->text, "Name unavailable, already taken or contains special characters @/#.\nEnter new name.. \0");
        send(clients[index].socketFD, sendBuffer, size_of_message, 0);
        recv(clients[index].socketFD, recvBuffer, size_of_message, 0);
        status = checkNameAvailibity(recvBuffer->text);
    } while (status == 1);
    printf("out of while\n");

    strcpy(sendBuffer->sender, "server\0");
    strcpy(sendBuffer->metadata, "name\0");
    strcpy(sendBuffer->text, recvBuffer->text);
    send(clients[index].socketFD, sendBuffer, size_of_message, 0);

    clients[index].name = malloc(sizeof(char) * 100);
    strcpy(clients[index].name, sendBuffer->text);
    lastClient++;

    free(sendBuffer);
    free(recvBuffer);

    pthread_create(&clients[index].recvThread, NULL, (void *)receiver, &clients[index].index);
}

void accepter(int *serverSFD)
{
    printf("Successful\n\n");
    while (1)
    {
        int index = clientCount;
        clientCount++;

        printf("Server waiting for connection...\n");
        int clientSFD = accept(*serverSFD, &(clients[index].address), &(clients[index].length));

        printf("\n%dth client connected successfully\n", index + 1);
        clients[index].socketFD = clientSFD;
        clients[index].index = index;
        pthread_create(&clients[index].iniThread, NULL, (void *)initializer, &clients[index].index);
    }
}

void main()
{
    int counter = 0;
    int serverSFD;
    printf("\nCreating server socket:");
    do
    {
        printf("\n");
        serverSFD = createServer("127.0.0.1", 8001);
        counter++;
    } while (serverSFD == -1 && counter < 5);
    printf("4. Receiving\t");
    if (serverSFD == -1)
    {
        printf("Failed\n");
        exit(0);
    }
    printf("Successful\n");

    // int status = close(serverSFD);
    // printf("Socket status %d\n", status);

    printf("\nThreading accepter()\t");
    pthread_t accepterThread;
    pthread_create(&accepterThread, NULL, (void *)accepter, (void *)&serverSFD);

    while (1)
    {
    }
}