This project deals with making a linux server, client application that works on the linux CLI.
The server listens for incomming connection in a seperate thread to the main thread for connections, when a client connected, a seperate thread is created to handle it's connection and data processing.

Each client can communicate with each other individually or in groups. All messages are relayed through the server which is responsble for unique naming of each client, and passing of each message to the correct client.
A provision has been made to have an admin that can access chat logs showcasing the database capabilities.
