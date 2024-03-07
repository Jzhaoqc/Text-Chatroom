#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>

// Part of the code is cited from https://beej.us/guide/bgnet/

#define BACKLOG 10  //Max amount of backlog for listen

void signal_setup(int sock_fd);
void pthread_setup(pthread_attr_t pthread_attr);

int main(int argc, char *argv[]){
    
    //Check command line input size
    char* port = argv[1];
    if(argc != 2){
        perror("ERROR: Wrong number of input: server <TCP listen port>\n");
        exit(EXIT_FAILURE);
    }

    int sock_fd, new_sock_fd;
    int return_value;
    struct addrinfo address, *address_info;

    //Init and populate the structure that specifies the criteria for selecting socket address
    memset(&address, 0, sizeof address);
    address.ai_family = AF_INET; // set to AF_INET to use IPv4
    address.ai_socktype = SOCK_STREAM;
    address.ai_flags = AI_PASSIVE; // use my IP

    //Get list of available addresses
    if ((return_value = getaddrinfo(NULL, port, &address, &address_info)) != 0) {   //should return 0 unless error
        fprintf(stderr, "ERROR: getaddrinfo: %s\n", gai_strerror(return_value));
        exit(EXIT_FAILURE);
    }

    //Create TCP Socket
    if((sock_fd = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol)) == -1){
        perror("ERROR: socket\n");
        exit(EXIT_FAILURE);
    }

    //Bind address to socket
    if (bind(sock_fd, address_info->ai_addr, address_info->ai_addrlen) == -1) {
            close(sock_fd);
            perror("ERROR: bind\n");
            exit(EXIT_FAILURE);
    }

    //Listen to socket
    if(listen(sock_fd, BACKLOG) == -1){
        perror("ERROR: listen\n");
        exit(EXIT_FAILURE);
    }
    
    //Add signals to sighandler
    signal_setup(sock_fd);

    pthread_attr_t pthread_attr;
    //pthread_client_arg_t *pthread_arg;
    pthread_t pthread;
    socklen_t client_address_len;
    pthread_mutex_t mux = PTHREAD_MUTEX_INITIALIZER;

    //Init pthread param
    pthread_setup(pthread_attr);





    return 0;
}

void signal_setup(int sock_fd){
    /* Assign signal handlers to signals. */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        close(sock_fd);
        perror("signal");
        exit(1);
    }
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        close(sock_fd);
        perror("signal");
        exit(1);
    }
}

void signal_handler(int signal){
    exit(EXIT_FAILURE);
}

void pthread_setup(pthread_attr_t pthread_attr){
    //Set up pthread parameter for detached threads
    if (pthread_attr_init(&pthread_attr) != 0) {
        perror("pthread_attr_init");
        exit(1);
    }
    if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED) != 0) {
        perror("pthread_attr_setdetachstate");
        exit(1);
    }

}
