
#include "common.h"
#include "Message.h"
#include "sys/stat.h"

using namespace std;
int do_grep_local(string input_cmd, int my_id);
void writeToFile(vector<string>& vmi_result, int i);


//Array of hostname of VMs
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


int main(int argc, char ** argv) {
    while(1){
        int socket_fds[NUM_VMS];                //Array of socket fs
        int num_alive = 0;                      //Number of connected VMs
        vector<vector<string> > results;        //Results of grep from all VMs
        int results_count[NUM_VMS] = {0};       //Number of lines found from grepping each VM
        bool failed[NUM_VMS] = {true};          //Array of unconnected VMs
        int sock_fd;                            //Socket fd of this VM
        char buf[BUF_SIZE];                     //Buffer to store msg received from other VMs
        fd_set r_master, w_master, r_fds, w_fds;    //set of fds
        int sock_to_vm[NUM_VMS] = {-1};         //Array to convert socket fd to VM id
        bool sent_request[NUM_VMS] = {false};   //Array of Vms that already sent request
        int receive_order[NUM_VMS] = {-1};      //Array to store order of receiving msg
        int max_fd = -1;                        //Highest fd among socket fds

        //Wait for user input
        cout << "prompt>";
        string cmd_str;
        getline(cin, cmd_str);
        
        //Check if user want to quit
        if(cmd_str == "quit")
            break;
        
        //Get id of this VM
        char my_addr[512];
        int my_id = -1;
        gethostname(my_addr,512);
        for(int i = 0 ; i < NUM_VMS; i++){
            if(strncmp(my_addr, vm_hosts[i].c_str(), vm_hosts[i].size()) == 0){
                my_id = i;
                break;
            }
        }
        
        //Reset all fd set
        FD_ZERO(&r_master);
        FD_ZERO(&w_master);
        FD_ZERO(&r_fds);
        FD_ZERO(&w_fds);

        //Make connection to all VMS
        for(int i = 0 ; i < NUM_VMS; i++){
            if(i == my_id){
                continue;
            }
            struct addrinfo hints, *p, *ai;
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            
            if(getaddrinfo(vm_hosts[i].c_str(), PORT_STR, &hints, &ai) == -1){
                continue;
            }
            
            for(p = ai; p != NULL; p = p->ai_next){
                if((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
                    continue;
                }
                if(connect(sock_fd, p->ai_addr, p->ai_addrlen) == -1){
                    close(sock_fd);
                    continue;
                }
                break;
            }
            if(p == NULL){
                continue;
            }
            if(sock_fd != -1){      //Can connect to Vmi
                socket_fds[i] = sock_fd;
                failed[i] = false;
                num_alive ++;
                sock_to_vm[sock_fd] = i;
                FD_SET(socket_fds[i], &r_master);
                FD_SET(socket_fds[i], &w_master);
                max_fd = max_fd > socket_fds[i] ? max_fd : socket_fds[i];
            }
            
            p = NULL;
            sock_fd = -1;
        }
        
       

        //Loop to send request and wait for other VMs to response
        while(results.size() <(unsigned int) num_alive) {
            w_fds = w_master;
            r_fds = r_master;
            if(select(max_fd+1, &r_fds, &w_fds, NULL, NULL) == -1){
                perror("client: select");
                exit(4);
            }
            //Go through all fds to check if can read or write from which fd
            for(int i = 1 ; i <= max_fd; i++){
                if(FD_ISSET(i, &r_fds)){    // There is a fd ready to read
                    int nbytes = 0;         // Number received bytes
                    int line_count;         // Number of lines found from grepping
                    Message my_msg;
                
                    if(recv(i, &line_count,sizeof(int), 0) <=0){
                        //There is error or connection is close
                        close(i);
                        FD_CLR(i, &w_master);
                        FD_CLR(i, &r_master);
                    }
                    else{
                        line_count = ntohl(line_count); //Convert from network byte order to host byte order
                        int length = my_msg.receive_int_msg(i);
                        int temp = 0;
                        vector<string> temp_results;
                        //Loop to receive and store msg
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
                        //Close connection and remove fd from read fd set
                        close(i);
                        FD_CLR(i, &r_master);
                        receive_order[sock_to_vm[i]] = results.size();
                        results_count[sock_to_vm[i]] = line_count;
                        results.push_back(temp_results);
                    }

                }
                
                if(FD_ISSET(i, &w_fds) && !sent_request[i] ){   // There is a fd ready to write
                    //Send request and remove fd from write fd set
                    sent_request[i] = true;
                    Message cmd_msg(cmd_str.size(), cmd_str.c_str());
                    if(cmd_msg.send_msg(i) == -1){
                        perror("Client: send");
                    }
                    FD_CLR(i, &w_master);
                }
            }
        }
        
        //Write result to file
       for(int i = 0; i < NUM_VMS; i++){
            if(results_count[i] >0 ){
                vector<string> vmi_result = results[receive_order[i]];
                writeToFile(vmi_result, i);
            }
        }
        //Do grep on local machine
        results_count[my_id] = do_grep_local(cmd_str, my_id);
        
        
        int total = 0;
        //Print out number of lines found from each VM and Calculate number of total lines
        for(int i = 0 ; i<NUM_VMS; i++){
            if(results_count[i] > 0){
                cout <<"VM" <<i+1 << " found: " << results_count[i] << " lines.\n";
            }
            total += results_count[i];
        }
        cout << "Totally found: " << total << " lines.\n";
    }
    return 0;
}

/*
 This function do grep on local machine and write the result to file
 Argument:  input_cmd:  Command from user 
            my_id:      id of current VM
 Return:    Number of lines found from doing grep in this VM
 */
int do_grep_local(string input_cmd, int my_id){
    FILE* file;             //File pointer to popen
    int count = 0;          //Number of lines found
    ostringstream stm ;     //Output string stream
    char line[MAX_LINE_SZ]; //Buffer to store result from grep
    string result;          //Result from doign grep
    ofstream out_file;      //


    //Calculate cmd from user input and local VM id
    string cmd = input_cmd + " " + "vm" + (char)(my_id+'1') + ".log";
    if(!(file = popen(cmd.c_str(), "r"))){
        return 0;
    }
    
    //Read and store result in stm and count number of lines found
    while(fgets(line, MAX_LINE_SZ, file)){
        stm << line;
        count++;
    }
    pclose(file);
    result = stm.str();
    
    //Store result to output file
    string name("output_VM");
    name += (char)(my_id+'1');
    name += ".txt";
    out_file.open (name.c_str());
    out_file << result;
    out_file.close();
    return count;
}

/*
 This function write input to a file
 Arguments: vmi_result: Array of result found from doing grep on VM
            i:  id of Vm that the result come from
 */
void writeToFile(vector<string>& vmi_result, int i){
    ofstream file;
    //result from VM-i is stored in file output_VMi.txt
    string name("output_VM");
    name += (char)(i+'1');
    name += ".txt";
    file.open (name.c_str());
    for(int j = 0; j < vmi_result.size(); j ++){
        file << vmi_result[j];
    }
    file.close();
}












