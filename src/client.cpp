//
// Created by the-quartz on 09/11/2021.
//
#include <sys/socket.h>
#include <iostream>
#include <netdb.h>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define MAXDATASIZE 2000

using namespace std;

int main(int argc, char *argv[]) {
    int sockFd; // socket file descriptor
    size_t bytesSent, bytesRcv; // number of bytes sent and received respectively
    char s[INET_ADDRSTRLEN],  // to store the server address
    buffer[MAXDATASIZE]; // buffer to receive data from server
    struct addrinfo hints{}, *servInfo, *info;
    char *serverPort;
    char defaultPort[] = "80"; // default port to use if port isn't specified
    fd_set readFd; // set of sockets ready to read from
    struct timeval tv{}; // timeout for select

    // define client attributes
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // get server info
    switch (argc) {
        case 3:
            serverPort = argv[2];
            break;
        case 2:
            serverPort = defaultPort;
            break;
        default:
            cerr << "Invalid number of arguments!";
            exit(-1);
    }
    if (getaddrinfo(argv[1], serverPort, &hints, &servInfo) == -1) {
        cerr << "Couldn't get server address info!";
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
            cerr << "Couldn't connect to socket!";
            exit(-3);
        }
        break;
    }
    // if none of the result were valid
    if (!info) {
        cerr << "Failed to connect!";
        exit(-4);
    }

    // convert server address from network to presentation and display it
    inet_ntop(info->ai_family, &((struct sockaddr_in *) info->ai_addr)->sin_addr, s, sizeof s);
    cout << "Client connected to " << s << endl;

    // free the space allocated to the server info
    freeaddrinfo(servInfo);

    // send request to the server
    char msg[] = "GET / HTTP/1.1\r\n\r\n";
    if ((bytesSent = send(sockFd, &msg, strlen(msg), 0)) == -1) {
        cerr << "Failed to send request!";
        exit(-5);
    }

    // add the current socket to the read set
    FD_SET(sockFd, &readFd);
    // timeout after 2 seconds if the server isn't sending more data
    tv.tv_sec = 2;
    // receive response from the server and display it
    while (true) {
        if (select(sockFd + 1, &readFd, nullptr, nullptr, &tv) == -1) {
            cerr << "Select error!";
            exit(-6);
        }
        if (!FD_ISSET(sockFd, &readFd))
            break;
        if ((bytesRcv = recv(sockFd, &buffer, MAXDATASIZE - 1, 0)) == -1) {
            cerr << "Failed to receive response!";
            exit(-7);
        }
        buffer[bytesRcv] = '\0'; // null terminate the string
        cout << buffer;
    }

    // close the socket
    close(sockFd);
    return 0;
}
