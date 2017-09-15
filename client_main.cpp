
#include "common.h"
#include "Message.h"
#include "sys/stat.h"

using namespace std;


////Change This
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



int do_grep_local(string cmd, int my_id){
    
    FILE* file;
    char buf[BUF_SIZE];
    ostringstream stm ;
    char line[MAX_LINE_SZ] ;
    if(!(file = popen(cmd.c_str(), "r"))){
        return 0;
    }
    string c;
    int count = 0;

    while(fgets(line, MAX_LINE_SZ, file)){
        stm << line;
        count++;
    }
    
    pclose(file);
    
    string result = stm.str();
    cout <<"Found " << count << " lines from VM" << my_id <<":\n";

    cout <<stm.str();
    return count;
}
//
//void handle_listen(){
//    while(){
//        
//    }
//    
//}



int main(int argc, char ** argv) {
    while(1){
        int socket_fds[NUM_VMS];
        int num_alive = 0;
        vector<vector<string> > results;
        int results_count[NUM_VMS];
        bool failed[NUM_VMS] = {true};
        int sock_fd;
        char buf[BUF_SIZE];
        
        fd_set r_master, w_master, r_fds, w_fds;
        //Wait for user input
        cout << "prompt>";
        string cmd_str;
        getline(cin, cmd_str);
        if(cmd_str == "quit")
            break;
        
        char my_addr[512];
        int my_id = -1;
        gethostname(my_addr,512);
        for(int i = 0 ; i < NUM_VMS; i++){
            if(strncmp(my_addr, vm_hosts[i].c_str(), vm_hosts[i].size()) == 0){
                my_id = i;
                break;
            }
        }
        
        cout<< "My name is : " << my_addr << "\n" ;
        int sock_to_vm[NUM_VMS] = {-1};
        
        //Connect to all VMS
        for(int i = 0 ; i < NUM_VMS; i++){
            if(i == my_id){
                continue;
            }
            struct addrinfo hints, *p, *ai;
            
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            
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
//                cout << "Fail to connect to VM" << i << "\n";
                continue;
            }
            if(sock_fd != -1){
                socket_fds[i] = sock_fd;
                failed[i] = false;
                num_alive ++;
                sock_to_vm[sock_fd] = i;
            }
            
            p = NULL;
            sock_fd = -1;
        }
        
        FD_ZERO(&r_master);
        FD_ZERO(&w_master);
        FD_ZERO(&r_fds);
        FD_ZERO(&w_fds);
        cout << "Number of alive: " << num_alive;
        int max_fd = -1;
        for(int i = 0 ; i < NUM_VMS; i++){
            if(failed[i] == false){
                FD_SET(socket_fds[i], &r_master);
                FD_SET(socket_fds[i], &w_master);
                max_fd = max_fd > socket_fds[i] ? max_fd : socket_fds[i];
            }
        }
        bool sent_request[NUM_VMS] = {false};
        int receive_order[NUM_VMS] = {-1};
        
        
        
        
        
        while(results.size() <(unsigned int) num_alive) {
            w_fds = w_master;
            r_fds = r_master;
            if(select(max_fd+1, &r_fds, &w_fds, NULL, NULL) == -1){
                cout << "erro num = " << errno << "\n";
	/*	if(errno == EBADF){
			// Handle bad file descriptor
			cout << "STOP" << "\n";
			for(int i = 0 ; i <= max_fd; i++){
				struct stat stat_buf;
				if(fstat(i, &stat_buf) == -1){
				cout << "Inside fstat\n";
					FD_CLR(i, &w_master);
					FD_CLR(i, &r_master);
					FD_CLR(i, &r_fds);
					FD_CLR(i, &w_fds);
					num_alive--;
				}
			}
			continue;
		}
		else{
		*/	perror("client: select");
        	        exit(4);
		//}
            }
            for(int i = 1 ; i <= max_fd; i++){
                if(FD_ISSET(i, &r_fds)){
                    int nbytes = 0;
                    Message my_msg;
                    
                    int line_count;
                    if(recv(i, &line_count,sizeof(int), 0) <=0){
                        close(i);
                        FD_CLR(i, &w_master);
                        close(i);
                        FD_CLR(i, &r_master);
                    }
                    else{
                        line_count = ntohl(line_count);
                        //  int line_count = my_msg.receive_int_msg(i);
                        int length = my_msg.receive_int_msg(i);
                        
                        int temp = 0;
                        vector<string> temp_results;
                        
                        while(1 && length!=0){
                            if((nbytes = (int)recv(i, buf, sizeof(buf), 0))  <= 0){
                                if(nbytes <0){
                                    perror("client: recv");
                                }
                                else{
                                    cout << "client: socket " << i << "hung up\n";
                                }
                                break;
                            }
                            else{
                                temp += nbytes;
                                string temp_str(buf,nbytes);
                                temp_results.push_back(temp_str);
                                if(temp >= length)
                                    break;
                            }
                        }
                        close(i);
                        FD_CLR(i, &r_master);
                        receive_order[sock_to_vm[i]] = results.size();
                        results_count[sock_to_vm[i]] = line_count;
                        results.push_back(temp_results);
                    }

                }
                
                
                if(FD_ISSET(i, &w_fds) && !sent_request[i] ){
                        sent_request[i] = true;
                        Message cmd_msg(cmd_str.size(), cmd_str.c_str());
                        
                        if(cmd_msg.send_msg(i) == -1){
                            perror("Client: send");
                        }
                        
                        FD_CLR(i, &w_master);


                }
            }
        }
        
        for(int i = 0; i < NUM_VMS; i++){
            if(results_count[i] >0 ){
                cout <<"Found " << results_count[i] << " lines from VM" << i <<":\n";
                vector<string> vmi_result = results[receive_order[i]];
                for(int j = 0; j < vmi_result.size(); j ++){
                    cout << vmi_result[j];
                }
            }
        }
        
        
        int total = do_grep_local(cmd_str, my_id);
        for(int i = 0 ; i<NUM_VMS; i++){
            total += results_count[i];
        }
        cout << "Totally found: " << total << " lines\n";
        
    }
    return 0;
    
}
















