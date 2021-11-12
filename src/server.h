//
// Created by the-quartz on 11/11/2021.
//

#ifndef HTTP_SERVER_SOCKET_PROGRAMMING_SERVER_H
#define HTTP_SERVER_SOCKET_PROGRAMMING_SERVER_H

#include <regex>
#include <fstream>
#include <sys/stat.h>

#define MAX_DATA_SIZE 2000

using namespace std;

void *handleConnection(void *ptr) {
    int clientFd = *(int *) ptr; // client socket file descriptor
    // persistent HTTP
    while (true) {
        struct timeval tv{}, tvPost{}; // timeout for connection and post request respectively
        fd_set readFdsPersistent, postFd; // fd sets for connection and post request respectively
        size_t bytesRcv; // number of bytes received from client
        char buffer[MAX_DATA_SIZE]; // buffer to store client request
        char *c; // iterator over buffer
        string request; // request line plus headers
        regex getPattern("GET (.+)\\s.+"), postPattern("POST (.+)\\s.+"), filepathPattern("(\\/.+)*\\/(.+)");
        smatch matchGet, matchPost, matchPath;
        ostringstream buf; // buffer to read the file into
        string response;
        size_t len, // message size
        total = 0, // how many bytes we've sent
        bytesLeft, // how many we have left to send
        bytesSent; // number of bytes sent in each iteration
        struct stat exists{}; // struct to check if file exists in case of GET

        // set the connection timeout
        tv.tv_sec = 10;
        // add the client socket to the read set
        FD_SET(clientFd, &readFdsPersistent);
        // wait for client to request until timeout
        if (select(clientFd + 1, &readFdsPersistent, nullptr, nullptr, &tv) == -1) {
            cerr << "Select error!\n";
            exit(-6);
        }
        if (!FD_ISSET(clientFd, &readFdsPersistent))
            break;
        // receive request from client
        if ((bytesRcv = recv(clientFd, &buffer, MAX_DATA_SIZE - 1, 0)) == -1) {
            cerr << "Failed to receive response!\n";
            exit(-8);
        }
        buffer[bytesRcv] = '\0'; // null terminate the string
        cout << buffer; // display the request
        // skip request line
        c = buffer;
        while (!(*c == '\r' && *++c == '\n' && *++c == '\r' && *++c == '\n')) {
            request += *c++;
        }
        c++;
        // check if the request is 'get' or 'post'
        // if 'get', send OK and file or Not Found
        if (regex_search(request, matchGet, getPattern)) {
            // extract file path
            string filePath = matchGet.str(1);
            // check if file doesn't exist
            if (stat(filePath.c_str(), &exists) != 0) {
                response = "HTTP/1.1 404 Not Found\r\n\r\n";
                char *msg = &response[0];
                if (send(clientFd, msg, response.length(), 0) == -1) {
                    cerr << "Failed to send 404 Not Found!\n";
                    break;
                }
                continue;
            }

            // read the file to be sent
            ifstream getSrc(filePath.c_str(), ios::in | ios::binary);
            buf << getSrc.rdbuf();
            getSrc.close();

            // create OK response with data
            response = "HTTP/1.1 200 OK\r\n\r\n" + buf.str();
            char *msg = &response[0];

            // make sure to send the whole message
            len = response.length();
            bytesLeft = len;
            while (total < len) {
                if ((bytesSent = send(clientFd, msg + total, bytesLeft, 0)) == -1) {
                    cerr << "Failed to post the whole file!\n";
                    break;
                }
                total += bytesSent;
                bytesLeft -= bytesSent;
            }

            // if 'post', send OK and save file
        } else if (regex_search(request, matchPost, postPattern)) {
            // extract postDest path
            string filePath = matchPost.str(1);
            string filename;
            // read the postDest to be sent
            ofstream postDest;
            if (regex_search(filePath, matchPath, filepathPattern)) {
                filename = matchPath.str(2);
            }
            postDest.open(filename);
            // store in directory
            postDest << c;
            FD_SET(clientFd, &postFd);
            // timeout after 1 second if the client isn't posting more data
            tvPost.tv_sec = 1;
            // continue receiving if the message isn't over
            while (true) {
                if (select(clientFd + 1, &postFd, nullptr, nullptr, &tvPost) == -1) {
                    cerr << "Select error!\n";
                    exit(-7);
                }
                // check if the client has more data to send, if not, break the loop
                if (!FD_ISSET(clientFd, &postFd))
                    break;
                // receive request from client
                if ((bytesRcv = recv(clientFd, &buffer, MAX_DATA_SIZE - 1, 0)) == -1) {
                    cerr << "Failed to receive response!\n";
                    exit(-8);
                }
                buffer[bytesRcv] = '\0'; // null terminate the string
                cout << buffer;
                postDest << buffer;
            }
            postDest.close();

            response = "HTTP/1.1 200 OK\r\n\r\n";
            char *msg = &response[0];
            if (send(clientFd, msg, response.length(), 0) == -1) {
                cerr << "Failed to send 200 OK!\n";
                break;
            }
        }
    }
    return nullptr;
}

#endif //HTTP_SERVER_SOCKET_PROGRAMMING_SERVER_H
