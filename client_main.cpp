
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

////Change This
string vm_hosts[NUM_VMS] = {
    "192.168.56.102",
    "fa17-cs425-g13-03.cs.illinois.edu",
    "fa17-cs425-g13-03.cs.illinois.edu",
    "fa17-cs425-g13-04.cs.illinois.edu",
    "fa17-cs425-g13-05.cs.illinois.edu",
    "fa17-cs425-g13-06.cs.illinois.edu",
    "fa17-cs425-g13-07.cs.illinois.edu",
    "fa17-cs425-g13-08.cs.illinois.edu",
    "fa17-cs425-g13-09.cs.illinois.edu",
    "fa17-cs425-g13-10.cs.illinois.edu"
};



int socket_fds[NUM_VMS];
int num_alive = 0;
bool failed[NUM_VMS] = {true};

int main(int argc, char ** argv) {
    int sock_fd;
    fd_set r_master, w_master, r_fds, w_fds;
    //Wait for user input
    cout << "prompt>";
    string cmd_str;
    cin >> cmd_str;
    char cmd_buf[1024];
    for(int i = 0 ; i < (int)cmd_str.size(); i++){
        cmd_buf[i] = cmd_str[i];
    }
    char buf[1024];

    //Connect to all VMS
    for(int i = 0 ; i < NUM_VMS; i++){
        struct addrinfo hints, *p, *ai;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        memset(ai, 0, sizeof(struct addrinfo));

        if(getaddrinfo(vm_hosts[i].c_str(), PORT_STR, &hints, &ai) == -1){
            cout << "Cannot getaddrinfo for VM " << i;
            continue;
        }
        for(p = ai; p != NULL; p = p->ai_next){
            if((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
                perror("client: socket");
                continue;
            }
            if(connect(sock_fd, p->ai_addr, p->ai_addrlen) == -1){
                close(sock_fd);
                perror("client: connect");
                continue;
            }
            break;
        }
        if(p == NULL){
            cout << "Fail to connect to VM" << i << "\n";
            continue;
        }
        if(sock_fd != -1){
            socket_fds[i] = sock_fd;
            failed[i] = false;
            num_alive ++;
        }

        p = NULL;
        sock_fd = -1;
    }

    FD_ZERO(&r_master);
    FD_ZERO(&w_master);
    FD_ZERO(&r_fds);
    FD_ZERO(&w_fds);
    
    int max_fd = -1;
    for(int i = 0 ; i < NUM_VMS; i++){
        if(failed[i] == false){
            FD_SET(socket_fds[i], &r_master);
            FD_SET(socket_fds[i], &w_master);
            max_fd = max_fd > socket_fds[i] ? max_fd : socket_fds[i];
        }
    }
    vector<string> results;
    bool sent_request[NUM_VMS] = {false};
    
    //timepnt begin = clk::now();
    
    while(/*std::chrono::duration_cast<unit_milliseconds>(clk::now() - begin).count() < TIMEOUT &&*/ results.size() <(unsigned int) num_alive) {
        w_fds = w_master;
        r_fds = r_master;
        if(select(max_fd+1, &r_fds, &w_fds, NULL, NULL) == -1){
            perror("client: select");
            exit(4);
        }
        for(int i = 1 ; i <= max_fd; i++){
            if(FD_ISSET(i, &r_fds)){
                int nbytes;
                if((nbytes = (int)recv(i, buf, sizeof(buf), 0))  <= 0){
                    if(nbytes <0){
                        perror("client: recv");
                    }
                    else{
                        cout << "client: socket " << i << "hung up\n";
                    }
                    close(i);
                    FD_CLR(i, &r_master);
                }
                else{
                    //store results from other VMs
                    cout <<"Receive "<<nbytes << "from server\n" ;
                    for(int k = 0 ; k < nbytes; k++){
                        cout << buf[k];
                    }
                    cout <<"\n";
                    results.push_back(buf);
                }
            }
            
            if(FD_ISSET(i, &w_fds) && !sent_request[i] ){
                //Send request to other VMs
                sent_request[i] = true;
                if(send(i, cmd_buf, cmd_str.size(), 0) == -1){
                    perror("client: send");
                }
                FD_CLR(i, &w_master);
            }
        }
    }
    //Print out the results
    for(int i = 0 ; i < (int)results.size(); i++){
        for(int j = 0 ; j < (int)results[i].size(); j++){
            cout << results[i][j];
        }
    }
    cout << "\nProgram ended!";
    return 0;
    
}
















