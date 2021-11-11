//
// Created by the-quartz on 11/11/2021.
//

#ifndef HTTP_SERVER_SOCKET_PROGRAMMING_SERVER_H
#define HTTP_SERVER_SOCKET_PROGRAMMING_SERVER_H

using namespace std;

void *handleConnection(void *ptr) {
    int clientFd = *(int *) ptr;
    // wait till timeout using select
    // receive request
    // check if the request is 'get' or 'post'
    // if 'post', send OK and receive file using select (copy from client)
    // if 'get', send OK and file or Not Found (copy from client)
    return nullptr;
}

#endif //HTTP_SERVER_SOCKET_PROGRAMMING_SERVER_H
