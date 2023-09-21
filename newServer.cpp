#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <thread>
#include <pthread.h>
#include <sqlite3.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#pragma comment(lib, "sqlite3")

using namespace std;

class Server
{
public:
    string IPAddress;
    int portNumber;
    sqlite3 *DataBase;

    int socketFD;
    struct sockaddr_in address;
    socklen_t length = sizeof(address);

    void takeInput()
    {
        cout << "Enter IP address (0 for localhost:8000).. ";
        getline(cin, IPAddress);
        if (IPAddress.compare("0") == 0)
        {
            IPAddress = "127.0.0.1";
            portNumber = 8000;
        }
        else
        {
            cout << "Enter port number.. ";
            cin >> portNumber;
        }
    }
    int createSocket()
    {
        address.sin_family = AF_INET;
        address.sin_port = htons(portNumber);
        address.sin_addr.s_addr = inet_addr(IPAddress.c_str());

        cout << "1. Creation\t";
        socketFD = socket(AF_INET, SOCK_STREAM, 0);
        if (socketFD == -1)
        {
            cout << "Failed\n";
            return (-1);
        }
        cout << "Successful\n";

        cout << "2. Binding\t";
        int binding = bind(socketFD, (struct sockaddr *)&address, length);
        if (binding == -1)
        {
            cout << "Failed\n";
            return (-1);
        }
        cout << "Successful\n";

        cout << "3. Listening\t";
        int listening = listen(socketFD, 10);
        if (listening == -1)
        {
            cout << "Failed\n";
            return (-1);
        }
        cout << "Successful\n";

        cout << "4. Completion\tSuccessful\n";
        return (1);
    }
    int createDataBase()
    {
        cout << "1. Creating :\t";
        int status = sqlite3_open(":memory:", &DataBase);
        if (status != SQLITE_OK)
        {
            cout << "Failed\n";
            return (-1);
        }
        return (1);
    }
    int createServer()
    {
        // create server socket
        int counter = 0, status;
        cout << "Initialising Server Socket creation..\n";
        do
        {
            if (counter != 0)
                cout << "Re-Enter server details, failed to make for that input\n\n";
            counter++;
            takeInput();
            status = createServer();
        } while (status != 1 && counter < 5);

        counter = 0;
        do
        {
            counter++;
            status = createDataBase();
        } while (status != 1 && counter < 5);
        // create server database
    }

} server;

class Client
{
public:
    string name;
    int index;

    int is_admin;
    int is_bot;
 
    int socketFD;
    struct sockaddr address;
    socklen_t length;

    pthread_t initializerThread;
};
vector<Client> clients;
vector<Client> processingClients;

struct message
{
    string sender;
    string receiver;

    string message;
    string metadata;
};

void accepter();
void initializer();
int serchClient(string name);

int main()
{
    server.createServer();

    cout << "\n5. Threading\n   accepter()\t";
}

void accepter()
{
    cout << "Successful\n\n";
    cout << "Server is accepting connections..\n\n";
    while (1)
    {
        Client *client = (Client *)malloc(sizeof(Client));
        client->socketFD = accept(server.socketFD, &(client->address), &(client->length));
        cout << "New client connected : sent for processing\n";
        processingClients.push_back(*client);

        // thread thread1(threadFunction);
        // thread1.join();
    }
}

void initializer()
{
    Client client = processingClients.back();
    processingClients.pop_back();
}