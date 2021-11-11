//
// Created by the-quartz on 09/11/2021.
//
#include <sys/socket.h>
#include <iostream>
#include <netdb.h>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <regex>

// reading buffer size
#define MAX_DATA_SIZE 2000
// default port to use if port isn't specified in argv[]
#define DEFAULT_PORT "80"

using namespace std;

// handle client_get command
void clientGet(int sockFd, string filename) {
    size_t bytesRcv; // number of bytes received from the server
    fd_set readFd; // set of sockets ready to read from
    struct timeval tv{}; // timeout for select
    char buffer[MAX_DATA_SIZE]; // buffer to receive data from server
    bool skipResponse = false, // flag raised when the char ptr passes the response line
    skipHeaders = false; // flag raised when the char ptr passes the headers
    regex filepathPattern("(\\/.+)*\\/(.+)"), responsePattern(R"(.+ 404 .+)");
    smatch matchPath, matchResponse; // regex match groups
    string response; // HTTP 200 OK or 404 Not Found
    char *c; // iterator over buffer
    ofstream file; // file to write GET data into

    // create GET request using filename
    string msgStr = "GET " + filename + " HTTP/1.1\r\n\r\n";
    char *msg = &msgStr[0];

    // send GET request to the server
    if (send(sockFd, msg, strlen(msg), 0) == -1) {
        cerr << "Failed to send GET request!\n";
        exit(-5);
    }

    // extract filename from path, create file with same name to receive data
    if (regex_search(filename, matchPath, filepathPattern)) {
        filename = matchPath.str(2);
    }
    file.open(filename);

    // add the current socket to the read set
    FD_SET(sockFd, &readFd);
    // timeout after 1 second if the server isn't sending more data
    tv.tv_sec = 1;

    // receive response from the server, display it, and store it in directory
    while (true) {
        if (select(sockFd + 1, &readFd, nullptr, nullptr, &tv) == -1) {
            cerr << "Select error!\n";
            exit(-6);
        }
        // check if the server has more data to send, if not, break the loop
        if (!FD_ISSET(sockFd, &readFd))
            break;
        // receive data from server
        if ((bytesRcv = recv(sockFd, &buffer, MAX_DATA_SIZE - 1, 0)) == -1) {
            cerr << "Failed to receive response!\n";
            exit(-7);
        }
        buffer[bytesRcv] = '\0'; // null terminate the string
        cout << buffer; // display the response
        // skip response line
        c = buffer;
        while (!skipResponse && *c++ != '\n') {
            response += *c;
        }
        // break if 404 Not Found
        if (regex_search(response, matchResponse, responsePattern)) {
            remove(filename.c_str());
            break;
        }
        skipResponse = true;
        // skip headers
        while (!skipHeaders && !(*c++ == '\r' && *c++ == '\n' && *c++ == '\r' && *c++ == '\n'));
        skipHeaders = true;
        // write the data to a file without response line or headers
        file << c;
    }

    file.close();
}

// handle client_post command
void clientPost(int sockFd, const string &filename) {
    ifstream input(filename.c_str()); // file to be posted
    ostringstream buf; // buffer to read the file into
    string postRequest;
    size_t len, // message size
    total = 0, // how many bytes we've sent
    bytesLeft, // how many we have left to send
    bytesSent; // number of bytes sent in each iteration

    // read the file to be posted
    buf << input.rdbuf();

    // create post request
    postRequest = "POST " + filename + " HTTP/1.1\r\n\r\n" + buf.str();
    char *msg = &postRequest[0];

    // make sure to send the whole message
    len = postRequest.length();
    bytesLeft = len;
    while (total < len) {
        if ((bytesSent = send(sockFd, msg + total, bytesLeft, 0)) == -1) {
            cerr << "Failed to post the whole file!";
            break;
        }
        total += bytesSent;
        bytesLeft -= bytesSent;
    }
}

int main(int argc, char *argv[]) {
    // defining all variables to be used later
    int sockFd; // socket file descriptor
//    size_t bytesRcv; // number of bytes sent and received respectively
    char serverAddress[INET_ADDRSTRLEN];  // to store the server address in presentation form
    struct addrinfo hints{}, *servInfo, *info;
    char *serverPort;
    char defaultPort[] = DEFAULT_PORT;

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
    ifstream input;
    input.open("input.txt");
    string line;
    while (getline(input, line)) {
        regex commandPattern(R"(client_(get|post)\s(.+))");
        smatch matchCommand;

        if (regex_search(line, matchCommand, commandPattern)) {
            // returns get or post
            string command = matchCommand.str(1);
            // returns the file-name argument
            string filename = matchCommand.str(2);

            if (command == "get")
                clientGet(sockFd, filename);
            else if (command == "post")
                clientPost(sockFd, filename);
        }
    }

    // close the socket
    close(sockFd);
    return 0;
}
