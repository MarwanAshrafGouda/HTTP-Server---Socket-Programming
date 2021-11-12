//
// Created by the-quartz on 09/11/2021.
//

#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include "client.h"

int main(int argc, char *argv[]) {
    // define all variables to be used later
    int sockFd; // socket file descriptor
    char serverAddress[INET_ADDRSTRLEN];  // to store the server address in presentation form
    struct addrinfo hints{}, *servInfo, *info;
    char *serverPort;
    char defaultPort[] = DEFAULT_PORT;
    ifstream input("input.txt"); // text file containing client commands
    string line; // a single client command
    regex commandPattern(R"(client_(get|post)\s(.+))");
    smatch matchCommand;

    // set client info
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // use IPv4
    hints.ai_socktype = SOCK_STREAM; // use TCP

    // get server info
    switch (argc) {
        case 3:
            serverPort = argv[2];
            break;
        case 2:
            serverPort = defaultPort;
            break;
        default:
            cerr << "Invalid number of arguments!\n";
            exit(-1);
    }
    if (getaddrinfo(argv[1], serverPort, &hints, &servInfo) == -1) {
        cerr << "Failed to get server address info!\n";
        exit(-2);
    }

    // loop through getaddrinfo() results, create socket and connect to it
    for (info = servInfo; info != nullptr; info = info->ai_next) {
        // create socket if possible
        if ((sockFd = socket(info->ai_family, info->ai_socktype, info->ai_protocol)) == -1) {
            continue;
        }
        // connect to socket
        if (connect(sockFd, (sockaddr *) info->ai_addr, info->ai_addrlen)) {
            cerr << "Couldn't connect to socket!\n";
            exit(-3);
        }
        break;
    }
    // if none of the results were valid
    if (!info) {
        cerr << "Failed to connect!\n";
        exit(-4);
    }

    // convert server address from network to presentation and display it
    inet_ntop(info->ai_family, &((struct sockaddr_in *) info->ai_addr)->sin_addr, serverAddress, sizeof serverAddress);
    cout << "Client connected to " << serverAddress << endl;

    // free the space allocated to the server info
    freeaddrinfo(servInfo);

    // read from input file and execute commands
    while (getline(input, line)) {
        if (regex_search(line, matchCommand, commandPattern)) {
            // returns get or post
            string command = matchCommand.str(1);
            // returns the file-name argument
            string filename = matchCommand.str(2);
            // call the corresponding client function
            if (command == "get") {
                clientGet(sockFd, filename);
            } else if (command == "post") {
                clientPost(sockFd, filename);
            }
        }
    }
    input.close();

    // close the socket when done with the commands
    close(sockFd);
    return 0;
}
