//
// Created by the-quartz on 11/11/2021.
//

#ifndef HTTP_SERVER_SOCKET_PROGRAMMING_CLIENT_H
#define HTTP_SERVER_SOCKET_PROGRAMMING_CLIENT_H

#include <iostream>
#include <arpa/inet.h>
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
    bool skipHeaders = false; // flag raised when the char ptr passes the response line and headers
    regex filepathPattern("(\\/.+)*\\/(.+)"), responsePattern(R"(HTTP\/1.1 404 .+)");
    smatch matchPath, matchResponse; // regex match groups
    string response; // HTTP 200 OK or 404 Not Found
    char *c; // iterator over buffer
    ofstream getDest; // getDest to write GET data into

    // create GET request using filename
    string msgStr = "GET " + filename + " HTTP/1.1\r\n\r\n";
    char *msg = &msgStr[0];

    // send GET request to the server
    if (send(sockFd, msg, strlen(msg), 0) == -1) {
        cerr << "Failed to send GET request!\n";
        exit(-5);
    }
    // extract filename from path, create getDest with same name to receive data
    if (regex_search(filename, matchPath, filepathPattern)) {
        filename = matchPath.str(2);
    }
    getDest.open(filename);

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
        cout << buffer;
        // skip response line
        c = buffer;
        while (!skipHeaders && !(*c == '\r' && *++c == '\n' && *++c == '\r' && *++c == '\n')) {
            response += *c++;
        }
        // break if 404 Not Found
        if (regex_search(response, matchResponse, responsePattern)) {
            getDest.close();
            remove(filename.c_str());
            break;
        }
        // skip the last '\n'
        if (!skipHeaders)
            c++;
        skipHeaders = true;
        // write the data to a getDest without response line or headers
        getDest << c;
    }
    getDest.close();
}

// handle client_post command
void clientPost(int sockFd, const string &filename) {
    ifstream postSrc(filename.c_str()); // file to be posted
    ostringstream buf; // buffer to read the file into
    string postRequest;
    size_t len, // message size
    total = 0, // how many bytes we've sent
    bytesLeft, // how many we have left to send
    bytesSent, // number of bytes sent in each iteration
    bytesRcv; // how many bytes received from server
    char buff[MAX_DATA_SIZE];

    // read the file to be posted
    buf << postSrc.rdbuf();

    // create post request
    postRequest = "POST " + filename + " HTTP/1.1\r\n\r\n" + buf.str();
    char *msg = &postRequest[0];

    // make sure to send the whole message
    len = postRequest.length();
    bytesLeft = len;
    while (total < len) {
        if ((bytesSent = send(sockFd, msg + total, bytesLeft, 0)) == -1) {
            cerr << "Failed to post the whole file!\n";
            break;
        }
        total += bytesSent;
        bytesLeft -= bytesSent;
    }

    // receive response from server
    if ((bytesRcv = recv(sockFd, buff, MAX_DATA_SIZE - 1, 0)) == -1) {
        cerr << "Failed to receive response from server!\n";
        exit(-8);
    }
    buff[bytesRcv] = '\0';
    cout << buff;
}

#endif //HTTP_SERVER_SOCKET_PROGRAMMING_CLIENT_H
