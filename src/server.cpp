//
// Created by the-quartz on 09/11/2021.
//
#include <sys/socket.h>
#include <iostream>
#include <netdb.h>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

// define port to bind server to
#define PORT "12345"
// define the number of connections the server could handle at a given time
#define BACKLOG 50

using namespace std;

void *handleConnection(void *ptr) {
    int clientFd = *(int *) ptr;
    char msg[] = "Hello Client!\n";
    if (send(clientFd, &msg, strlen(msg), 0) == -1) {
        cerr << "Failed to send to client!\n";
        exit(-6);
    }
    return nullptr;
}

int main() {
    // define variables to be used later
    int sockFd, clientFd; // socket file descriptors for listener socket and worker socket
    int yes = 1;
    addrinfo hints{}, *servInfo, *info;
    struct sockaddr_storage clientAddress{};
    socklen_t sinSize;
    char clAddress[INET_ADDRSTRLEN]; // store the client address in presentation form

    // set server info
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // use IPv4
    hints.ai_socktype = SOCK_STREAM; // use TCP
    hints.ai_flags = AI_PASSIVE; // use my IP address

    // get server info
    if (getaddrinfo(nullptr, PORT, &hints, &servInfo) == -1) {
        cerr << "Failed to get server address info!\n";
        exit(-1);
    }

    // loop over the results of getaddrinfo(), create socket and bind to it
    for (info = servInfo; info != nullptr; info = info->ai_next) {
        // create socket if possible
        if ((sockFd = socket(info->ai_family, info->ai_socktype, info->ai_protocol)) == -1) {
            continue;
        }
        // make the socket reusable
        if (setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            cerr << "Failed to make the socket reusable!\n";
            exit(-2);
        }
        // bind to the socket
        if (bind(sockFd, (sockaddr *) info->ai_addr, info->ai_addrlen) == -1) {
            close(sockFd);
            continue;
        }
        break;
    }
    // if failed to bind to any of the results
    if (!info) {
        cerr << "Failed to bind socket to specified port!\n";
        exit(-3);
    }

    // free the allocated space to the server info
    freeaddrinfo(servInfo);

    // listen on the specified port for incoming connections
    if (listen(sockFd, BACKLOG) == -1) {
        cerr << "Failed to listen on specified port!\n";
        exit(-4);
    }

    cout << "Server waiting on connections...\n";

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (true) {
        sinSize = sizeof clientAddress;
        // accept incoming connections
        if ((clientFd = accept(sockFd, (sockaddr *) &clientAddress, &sinSize)) == -1) {
            cerr << "Failed to accept connection!\n";
            continue;
        }

        // convert server address from network to presentation and display it
        inet_ntop(clientAddress.ss_family, &((struct sockaddr_in *) &clientAddress)->sin_addr, clAddress,
                  sizeof clAddress);
        cout << "Accepted connection from " << clAddress << endl;

        pthread_t worker;
        int *arg = static_cast<int *>(malloc(sizeof(*arg)));
        if (arg == nullptr) {
            cerr << "Couldn't allocate memory for thread arg!" << endl;
            exit(-5);
        }
        *arg = clientFd;
        pthread_create(&worker, nullptr, handleConnection, arg);
    }
#pragma clang diagnostic pop
}


