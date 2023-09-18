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

FILE *serverLog;

struct message
{
    char sender[100];
    char metadata[100];
    char text[1024];
};
const int size_of_message = sizeof(struct message);

const int MAX_CLIENTS = 5;
struct client_property
{
    char *name;
    int index;
    int status;
    int tempAlloc;
    int permAlloc;

    int socketFD;
    struct sockaddr address;
    socklen_t length;

    pthread_t iniThread;
    pthread_t recvThread;
    pthread_t analThread;
    pthread_t sendThread;
} clients[5];

void analyzer(struct message *recvBuffer)
{
    // to send to all
    if (strchr(recvBuffer->text, '@') == NULL || strstr(recvBuffer->text, "@all") != NULL)
    {
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].permAlloc == 0)
                continue;
            if (strstr(clients[i].name, "admin\0") != NULL)
                continue;

            if (strstr(recvBuffer->sender, clients[i].name) != NULL)
                continue;
            send(clients[i].socketFD, recvBuffer, size_of_message, 0);
        }
    }
    // to send to specific person
    else
    {
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].permAlloc == 0 || strstr(clients[i].name, "admin\0") != NULL)
                continue;
            if (strstr(recvBuffer->sender, clients[i].name) != NULL)
                continue;

            char *name = malloc(100 * sizeof(char));
            strcpy(name, "@\0");
            strcat(name, clients[i].name);
            if (strstr(recvBuffer->text, name) != NULL)
            {
                send(clients[i].socketFD, recvBuffer, size_of_message, 0);
            }
            free(name);
        }
    }
    free(recvBuffer);
}

void informOthers(int index, char *text)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (index == i)
            continue;
        if (clients[i].permAlloc == 1)
        {
            struct message *sendBuffer = malloc(size_of_message);
            strcpy(sendBuffer->metadata, "text\0");
            strcpy(sendBuffer->sender, "server\0");
            strcpy(sendBuffer->text, clients[index].name);
            strcat(sendBuffer->text, text);
            send(clients[i].socketFD, sendBuffer, size_of_message, 0);
            free(sendBuffer);
        }
    }
}

void administrator(struct message *recvBuffer)
{
}

void receiver(int *clientIndex)
{
    int index = *clientIndex;
    while (1)
    {
        struct message *recvBuffer = malloc(size_of_message);
        clients[index].status = recv(clients[index].socketFD, recvBuffer, size_of_message, 0);
        if (clients[index].status == size_of_message)
        {
            if (strstr(recvBuffer->sender, "admin\0") == NULL)
            {
                // char *line;
                // strcpy(line, "[");
                // strcpy(line, recvBuffer->sender);
                // strcpy(line, "] ");
                // strcpy(line, recvBuffer->text);
                // fprintf(serverLog, line);
                // free(line);
                pthread_create(&clients[index].analThread, NULL, (void *)analyzer, recvBuffer);
            }
            else
                pthread_create(&clients[index].analThread, NULL, (void *)administrator, recvBuffer);
        }
        else
        {
            clients[index].permAlloc = 0;
            clients[index].tempAlloc = 0;
            printf("[%s] disconnected\n", clients[index].name);
            if (strstr(clients[index].name, "admin\0") == NULL)
                informOthers(index, " disconnected.\0");
            pthread_cancel(clients[index].recvThread);
            break;
        }
    }
}

int onlyClient()
{
    int count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].permAlloc == 1)
            count++;
    }
    if (count == 1)
        return (1);
    return (0);
}

void sendClientList(int index)
{
    if (onlyClient())
    {
        struct message *sendBuffer = malloc(size_of_message);
        strcpy(sendBuffer->metadata, "text\0");
        strcpy(sendBuffer->sender, "server\0");
        strcpy(sendBuffer->text, "You are the first one here.\0");
        send(clients[index].socketFD, sendBuffer, size_of_message, 0);
    }
    else
    {
        struct message *sendBuffer = malloc(size_of_message);
        strcpy(sendBuffer->metadata, "text\0");
        strcpy(sendBuffer->sender, "server\0");
        strcpy(sendBuffer->text, "Here's a list of connected clients..\0");
        send(clients[index].socketFD, sendBuffer, size_of_message, 0);
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if(index == i)
            continue;
            if (clients[i].permAlloc == 1)
            {
                strcpy(sendBuffer->text, clients[i].name);
                send(clients[index].socketFD, sendBuffer, size_of_message, 0);
            }
        }
        free(sendBuffer);
    }
}

int checkNameAvailibity(char *name)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].permAlloc == 0)
            continue;

        if (strcmp(clients[i].name, name) == 0)
        {
            // printf("%s and %s duplicate name\n", clients[i].name, name);
            return (1);
        }
        if (strchr(clients[i].name, '@') != NULL || strchr(clients[i].name, '#') != NULL)
        {
            // printf("%s and %s contains special char\n", clients[i].name, name);
            return (1);
        }
    }
    // printf("returnning\n");
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
        int statusRecv = recv(clients[index].socketFD, recvBuffer, size_of_message, 0);
        if (statusRecv == 0)
        {
            clients[index].tempAlloc = 0;
            clients[index].permAlloc = 0;
            printf("[no name] disconnected\n");
            pthread_cancel(clients[index].iniThread);
        }
        status = checkNameAvailibity(recvBuffer->text);
    } while (status == 1);

    clients[index].permAlloc = 1;

    sendClientList(index);

    strcpy(sendBuffer->metadata, "name\0");
    strcpy(sendBuffer->text, recvBuffer->text);
    send(clients[index].socketFD, sendBuffer, size_of_message, 0);

    strcpy(clients[index].name, sendBuffer->text);

    if (strstr(clients[index].name, "admin") == NULL)
        informOthers(index, " just connected say Hi!\0");

        free(sendBuffer);
    free(recvBuffer);

    pthread_create(&clients[index].recvThread, NULL, (void *)receiver, &clients[index].index);
}

int clientAllocation()
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].tempAlloc == 0 && clients[i].permAlloc == 0)
        {
            clients[i].tempAlloc = 1;
            return (i);
        }
    }
    return (-1);
}

void accepter(int *serverSFD)
{
    printf("Successful\n\n");
    printf("Server accepting connections..\n\n");
    while (1)
    {
        struct sockaddr address;
        socklen_t length;
        int clientSFD = accept(*serverSFD, &address, &length);

        int index = clientAllocation();

        if (index != -1)
        {
            printf("%dth client connected successfully\n", (index + 1));

            clients[index].address = address;
            clients[index].length = length;
            clients[index].socketFD = clientSFD;
            clients[index].index = index;

            pthread_create(&clients[index].iniThread, NULL, (void *)initializer, &clients[index].index);
        }
        else
        {
            struct message *sendBuffer = malloc(size_of_message);
            strcpy(sendBuffer->metadata, "wait\0");
            strcpy(sendBuffer->sender, "server\0");
            strcpy(sendBuffer->text, "Sorry the server is full, disconnecting, retry later\0");
            send(clientSFD, sendBuffer, size_of_message, 0);
        }
    }
}

void main()
{
    char *fileName = "serverLog.txt";
    remove(fileName);
    serverLog = fopen(fileName, "w");

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

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].name = malloc(sizeof(char) * 100);
        clients[i].tempAlloc = 0;
        clients[i].permAlloc = 0;
    }

    printf("\n5. Threading\n   accepter()\t");
    pthread_t accepterThread;
    pthread_create(&accepterThread, NULL, (void *)accepter, (void *)&serverSFD);

    while (1)
    {
        // char *command = malloc(sizeof(char) * 100);
        // fgets(command, 1024, stdin);
        // // char *pos;
        // // if ((pos = strchr(command, '\n')) != NULL)
        // //     *pos = '\0';
        // if(strchr(command, '#') != NULL)
        // {

        // }
    }
}