//
//  server_main.cpp
//  ECE428_MP1
//
//  Created by Hieu Huynh on 9/10/17.
//  Copyright © 2017 Hieu Huynh. All rights reserved.
//


#include <stdio.h>
#include <iostream>
#include <vector>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

using namespace std;

#define PORT				4950
#define PORT_STR            "4950"
#define NUM_VMS				10
#define TIMEOUT				250
#define STDIN               0

#define MSG_LENGTH			1024
#define HOSTNAME_LENGTH		256
#define ERROR_LENGTH		4096


int main(int argc, char ** argv) {
    //Set up socket 
    int socket_fd, new_fd, max_fd;
    struct addrinfo hints, *ai, *p;
    int yes = 1;
    struct sigaction sa;
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    socklen_t addr_len;
    struct sockaddr_storage remoteaddr; // client address

    char buf[1024];
    
    //Set up Socket
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    if(getaddrinfo(NULL, PORT_STR, &hints, &ai) == -1){
        perror("server: getaddrinfo");
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next){
        if((socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
            perror("server: socket");
            continue;
        }
        if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
            perror("server: setsockpot");
            exit(1);
        }
        if(bind(socket_fd, p->ai_addr, p->ai_addrlen) == -1){
            close(socket_fd);
            perror("server: bind");
            exit(1);
        }
        break;
    }
    
    if(p == NULL){
        perror("server: fail to bind");
        exit(1);
    }
    
    freeaddrinfo(ai);
    //Listen for clients
    if (listen(socket_fd, 20) == -1) {
        perror("listen");
        exit(1);
    }
    
    
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    
    //Waiting for clients
    FD_SET(socket_fd, &master);
    max_fd = socket_fd;

    socklen_t sin_size;
    struct sockaddr_storage their_addr; // connector's address information

    while(1){
        read_fds = master;
        
        if(select(max_fd+1, &read_fds, NULL, NULL, NULL) == -1){
            perror("server: select");
            exit(4);
        }
        for(int i = 0 ; i <= max_fd; i++){
            if(FD_ISSET(i, &read_fds)){ //Have one to fd ready
                if(i == socket_fd){
                    //Have new connection
                    addr_len = sizeof(remoteaddr);
                    new_fd = accept(socket_fd, (struct sockaddr*) &remoteaddr, &addr_len);
                    if(new_fd == -1){
                        perror("server: accept");
                    }
                    else{
                        FD_SET(new_fd, &master);
                        max_fd = max_fd > new_fd ? max_fd : new_fd;
                    }
                }
                else{
                    //Receive msg from clients
                    int nbytes;
                    if((nbytes = (int)recv(i, buf, sizeof(buf), 0))  <= 0){
                        if(nbytes <0){
                            perror("server: recv");
                        }
                        FD_CLR(i, &master);
                    }
                    else{
                        //Do grep & send back result
                        cout <<"Received: " << nbytes << "Bytes\n";
                        for(int k = 0; k < nbytes; k++){
                            cout << buf[k];
                        }
                        cout <<"\n";
                        char temp[] ="Result from Grep";
                        if(send(i, &temp, sizeof(temp), 0) == -1){
                            perror("server: send");
                        }
                    }
                }
            }
        }
    }
    
    return 0;
}












