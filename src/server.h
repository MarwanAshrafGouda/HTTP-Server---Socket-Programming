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
        struct timeval tv{}, tvReq{}; // timeout for connection and post requestWithHeaders respectively
        fd_set readFdsPersistent, readRequest; // fd sets for connection and post requestWithHeaders respectively
        size_t bytesRcv; // number of bytes received from client
        char buffer[MAX_DATA_SIZE]; // buffer to store client requestWithHeaders
        char *c; // iterator over buffer
        bool skipRequest = false; // flag raised when the requestWithHeaders line is skipped
        string requestWithHeaders; // request line plus headers
        regex getPattern("GET (.+)\\s.+"), postPattern("POST (.+)\\s.+"), filepathPattern("(\\/.+)*\\/(.+)");
        smatch matchGet, matchPost, matchPath;
        ostringstream buf; // buffer to read the file into
        string getResponse;
        size_t len, // message size
        total = 0, // how many bytes we've sent
        bytesLeft, // how many we have left to send
        bytesSent; // number of bytes sent in each iteration
        struct stat exists{}; // struct to check if file exists in case of GET

        // set the connection timeout
        tv.tv_sec = 5;
        // add the client socket to the read set
        FD_SET(clientFd, &readFdsPersistent);
        // wait for client to requestWithHeaders until timeout
        if (select(clientFd + 1, &readFdsPersistent, nullptr, nullptr, &tv) == -1) {
            cerr << "Select error!\n";
            exit(-6);
        }
        if (!FD_ISSET(clientFd, &readFdsPersistent))
            break;

        // receive requestWithHeaders (and file using select in case of POST (copy from client))
        FD_SET(clientFd, &readRequest);
        // timeout after 1 second if the server isn't sending more data
        tvReq.tv_sec = 1;
        while (true) {
            if (select(clientFd + 1, &readRequest, nullptr, nullptr, &tvReq) == -1) {
                cerr << "Select error!\n";
                exit(-7);
            }
            // check if the client has more data to send, if not, break the loop
            if (!FD_ISSET(clientFd, &readRequest))
                break;
            // receive requestWithHeaders from client
            if ((bytesRcv = recv(clientFd, &buffer, MAX_DATA_SIZE - 1, 0)) == -1) {
                cerr << "Failed to receive response!\n";
                exit(-8);
            }
            buffer[bytesRcv] = '\0'; // null terminate the string
            cout << buffer; // display the requestWithHeaders
            // skip requestWithHeaders line
            c = buffer;
            while (!skipRequest && !(*c == '\r' && *++c == '\n' && *++c == '\r' && *++c == '\n')) {
                requestWithHeaders += *c++;
            }
            if (!skipRequest)
                c++;
            // check if the requestWithHeaders is 'get' or 'post'
            // if 'get', send OK and file or Not Found
            if (regex_search(requestWithHeaders, matchGet, getPattern)) {
                // extract file path
                string filePath = matchGet.str(1);
                // check if file doesn't exist
                if (stat(filePath.c_str(), &exists) != 0) {
                    getResponse = "HTTP/1.1 404 Not Found\r\n\r\n";
                    char *msg = &getResponse[0];
                    if(send(clientFd, msg, getResponse.length(), 0) == -1) {
                        cerr << "Failed to send 404 Not Found!\n";
                        break;
                    }
                    break;
                }

                // read the file to be sent
                ifstream input(filePath.c_str());
                buf << input.rdbuf();
                // create OK response with data
                getResponse = "HTTP/1.1 200 OK\r\n\r\n" + buf.str();
                char *msg = &getResponse[0];

                // make sure to send the whole message
                len = getResponse.length();
                bytesLeft = len;
                while (total < len) {
                    if ((bytesSent = send(clientFd, msg + total, bytesLeft, 0)) == -1) {
                        cerr << "Failed to post the whole file!\n";
                        break;
                    }
                    total += bytesSent;
                    bytesLeft -= bytesSent;
                }
                break; // since the client won't send more data

            // if 'post', send OK and save file
            } else if (regex_search(requestWithHeaders, matchPost, postPattern)) {
                // extract file path
                string filePath = matchPost.str(1);
                string filename;
                // read the file to be sent
                ofstream posted;
                if (regex_search(filePath, matchPath, filepathPattern)) {
                    filename = matchPath.str(2);
                }
                posted.open(filename);
                // store file in directory
                posted << c;
                getResponse = "HTTP/1.1 200 OK\r\n\r\n";
                char *msg = &getResponse[0];
                if(send(clientFd, msg, getResponse.length(), 0) == -1) {
                    cerr << "Failed to send 200 OK!\n";
                    break;
                }
                posted.close();
                break;
            }
            skipRequest = true;
        }
    }
    return nullptr;
}

#endif //HTTP_SERVER_SOCKET_PROGRAMMING_SERVER_H
