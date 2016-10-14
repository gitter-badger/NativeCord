//
// Created by wolverindev on 24.08.16.
//

#ifndef CBUNGEE_SOCKETUTIL_H
#define CBUNGEE_SOCKETUTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "../connection/Socket.h"

using namespace std;

class SocketUtil {
public:
    static Socket* createTCPSocket(const char *host, uint16_t port) {
        int sockfd, n;
        struct sockaddr_in serv_addr;
        struct hostent *server;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("ERROR opening socket");
            return (Socket*) new int(-1);
        }
        server = gethostbyname(host);
        if (server == NULL) {
            perror("ERROR, no such host\n");
            return (Socket*) new int(-2);
        }
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
        serv_addr.sin_port = htons(port);
        if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            perror("ERROR connecting");
            close(sockfd);
            return (Socket*) new int(-3);
        }
        return new Socket(sockfd);
    }

    static int createTCPServerSocket(uint16_t port) {
        int sockfd;
        struct sockaddr_in serv_addr;
        int n;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("ERROR opening socket");
            return -1;
        }
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);
        if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            perror("ERROR on binding");
            return -2;
        }
        listen(sockfd, 5);
        return sockfd;
    }
};

#endif //CBUNGEE_SOCKETUTIL_H