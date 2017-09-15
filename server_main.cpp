//
//  server_main.cpp
//  ECE428_MP1
//
//  Created by Hieu Huynh on 9/10/17.
//  Copyright © 2017 Hieu Huynh. All rights reserved.
//



#include "common.h"
#include "Message.h"
using namespace std;

string vm_hosts[NUM_VMS] = {
    "fa17-cs425-g13-01.cs.illinois.edu",
    "fa17-cs425-g13-02.cs.illinois.edu",
    "fa17-cs425-g13-03.cs.illinois.edu",
    "fa17-cs425-g13-04.cs.illinois.edu",
    "fa17-cs425-g13-05.cs.illinois.edu",
    "fa17-cs425-g13-06.cs.illinois.edu",
    "fa17-cs425-g13-07.cs.illinois.edu",
    "fa17-cs425-g13-08.cs.illinois.edu",
    "fa17-cs425-g13-09.cs.illinois.edu",
    "fa17-cs425-g13-10.cs.illinois.edu"
};
int my_id = -1;

void do_grep(string input_cmd, int socket_fd){
    FILE* file;
    char buf[BUF_SIZE];
    ostringstream stm ;
    char line[MAX_LINE_SZ] ;
    string cmd = input_cmd;
    if(!(file = popen(cmd.c_str(), "r"))){
        return ;
    }
    string c;
    int count = 0;
    while(fgets(line, MAX_LINE_SZ, file)){
        stm << line ;
        count ++;
    }
    
    pclose(file);
    string result = stm.str();
    Message my_msg(result.size(), result.c_str());
    
    
    my_msg.send_int_msg(count, socket_fd);
    
    my_msg.send_msg(socket_fd);
    return;
}

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
    
    char my_addr[512];
    gethostname(my_addr,512);
    for(int i = 0 ; i < NUM_VMS; i++){
        if(strncmp(my_addr, vm_hosts[i].c_str(), vm_hosts[i].size()) == 0){
            my_id = i;
            break;
        }
    }
    
    
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
                    int nbytes = 0;
                    Message my_msg;

                    int length = my_msg.receive_int_msg(i);
                    int temp = 0;
                    while(1 && length >0){
                        if((nbytes = (int)recv(i, buf, sizeof(buf), 0))  <= 0){
                            if(nbytes <0){
                                perror("server: recv");
                            }
                            else{
                                cout << "server: socket " << i << "hung up\n";
                            }
                            break;
                        }
                        else{
                            temp += nbytes;
                            if(temp >= length)
                                break;
                        }
                    }
                    if(length > 0 ){
                        string my_str1(buf,length);
                        do_grep(my_str1, i);
                    }

                    close(i);
                    FD_CLR(i,&master);

                }
            }
        }
    }
    
    return 0;
}












