#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

struct message
{
    char sender[100];
    char metadata[100];
    char text[1024];
};
int size_of_message = sizeof(struct message);

struct details
{
    char name[100];
    int clientSFD;

    struct sockaddr_in address;

    pthread_t recvThread;
} me;

int createClient(char *ip, int port)
{
    me.address.sin_family = AF_INET;
    me.address.sin_addr.s_addr = inet_addr(ip);
    me.address.sin_port = htons(port);

    printf("1. Creation\t");
    int fileDescripter = socket(AF_INET, SOCK_STREAM, 0);
    if (fileDescripter == -1)
    {
        printf("Failed\n");
        return (-1);
    }
    printf("Successful\n");

    printf("2. Connecting\t");
    int connecting = connect(fileDescripter, (struct sockaddr *)&me.address, sizeof(me.address));
    if (connecting == -1)
    {
        printf("Failed\n");
        return (-1);
    }
    printf("Successful\n");

    return (fileDescripter);
}

void receiverFunction()
{
    int clientSFD = me.clientSFD;
    struct message *recvBuffer = malloc(size_of_message);
    while (1)
    {
        recv(clientSFD, recvBuffer, size_of_message, 0);
        printf("[%s] %s\n", recvBuffer->sender, recvBuffer->text);
    }
}
void initialize()
{
    struct message *iniRecv = malloc(sizeof(struct message));
    struct message *iniSend = malloc(sizeof(struct message));
    recv(me.clientSFD, iniRecv, size_of_message, 0);
    printf("[%s] %s\n", iniRecv->sender, iniRecv->text);
    do
    {
        fgets(iniSend->text, 1024, stdin);
        char *pos;
        if ((pos = strchr(iniSend->text, '\n')) != NULL)
            *pos = '\0';
        send(me.clientSFD, iniSend, size_of_message, 0);
        recv(me.clientSFD, iniRecv, size_of_message, 0);
        printf("[%s] %s\n", iniRecv->sender, iniRecv->text);
    } while (strcmp(iniRecv->metadata, "name\0") != 0);

    strcpy(me.name, iniRecv->text);

    free(iniRecv);
    free(iniSend);
}
void main()
{
    int counter = 0;
    printf("\nCreating client socket:");
    do
    {
        printf("\n");
        me.clientSFD = createClient("127.0.01", 8001);
        counter++;
    } while (me.clientSFD == -1 && counter < 5);
    printf("3. Receiving\t");
    if (me.clientSFD == -1)
    {
        printf("Failed\n");
        exit(0);
    }
    printf("Successful\n");
    printf("Starting initialization..\n");
    initialize();

    pthread_create(&me.recvThread, NULL, (void *)receiverFunction, NULL);

    struct message *sendBuffer = malloc(sizeof(char) * 1024);
    strcpy(sendBuffer->sender, me.name);
    strcpy(sendBuffer->metadata, "text\0");
    while (1)
    {
        fgets(sendBuffer->text, 1024, stdin);
        char *pos;
        if ((pos = strchr(sendBuffer->text, '\n')) != NULL)
            *pos = '\0';
        send(me.clientSFD, sendBuffer, size_of_message, 0);
    }
}