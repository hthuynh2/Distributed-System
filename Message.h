//
//  Message.h
//  
//
//  Created by Hieu Huynh on 9/13/17.
//
//

#ifndef Message_h
#define Message_h


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
#include "limits.h"


#define ERROR INT_MIN
using namespace std;
class Message{
    int length;
    char * msg_ptr;
    
public:
    Message(){
        msg_ptr = NULL;
        length = 0;
    }
    
    Message(int input_length, const char* buf){
        length = input_length;
        msg_ptr = new char[length];
        memcpy(msg_ptr, buf, length);
//        for(int i = 0 ; i < length; i++){
//            cout << msg_ptr[i];
//        }
//        cout<< "\n";

    }
    
    ~Message(){
        if (msg_ptr){
            delete msg_ptr;
        }
        length = 0;
        msg_ptr = NULL;
    }
    
    void append(int input_length,  char* buf){
        char* t = new char[input_length + length];
        if(length != 0 && msg_ptr!= NULL){
            memcpy(t, msg_ptr, length);
        }
        memcpy(t + length, buf, input_length);
        if(msg_ptr != NULL)
            delete msg_ptr;
        msg_ptr = t;
        return;
    }
    
    int send_int_msg(int number , int socket_fd){
        int ret;
        //Convert from host byte order to network byte order
        int net_number = htonl(number);
        if((ret = send(socket_fd, &net_number, sizeof(net_number), 0)) == -1){
            return -1;
        }
        return ret;
    }
    
    int receive_int_msg(int socket_fd){
        int number;
        if(recv(socket_fd, &number,sizeof(int), 0) == -1){
            perror("Message: receive_int");
            return ERROR;
        }
        number = ntohl(number);
        return number;
    }
    
    
    int send_msg(int socket_fd){
        int ret;
        if(send_int_msg(length, socket_fd) == -1){
            perror("Message: send_int");
            return -1;
        }
        while(length >0){
            if((ret = send(socket_fd, msg_ptr, length, 0)) == -1){
                perror("Message: send");
                return -1;
            }
//            cout << ret;
            length -= ret;
        }
//        cout << length;
        return length;
    }
    
    
};




#endif /* Message_h */
