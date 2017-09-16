//
//  common.h
//  
//
//  Created by Hieu Huynh on 9/14/17.
//
//

#ifndef common_h
#define common_h


#define PORT				4950
#define PORT_STR            "4950"
#define NUM_VMS				10
#define TIMEOUT				250
#define STDIN               0
#define MSG_LENGTH			1024
#define HOSTNAME_LENGTH		256
#define ERROR_LENGTH		4096
#define BUF_SIZE            1024
#define MAX_LINE_SZ         1024


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
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
//#include <unordered_map>




#endif /* common_h */
