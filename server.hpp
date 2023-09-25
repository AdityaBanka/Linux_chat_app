class server
{
    public:
    string IPAddress;
    int portNumber;
    sqlite3 *DataBase;

    int socketFD;
    struct sockaddr_in address;
    socklen_t length;
}