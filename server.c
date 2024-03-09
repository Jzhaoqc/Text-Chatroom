#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>
#include "chatroom.h"

#define BACKLOG 10  //Max amount of backlog for listen

typedef struct client{
    char username[100];
    char password[100];
}Client;

Client clients[3] = { {"user1", "123"}, {"user2", "123"}, {"user3", "123"} };

typedef struct pthread_client_info{
    int client_sock_fd;
    struct addrinfo client_address;
    int size;
}pthread_client_arg;

// Part of the code is cited from https://beej.us/guide/bgnet/

//Function declarations
void signal_setup(int sock_fd);
void signal_handler(int signal);
void pthread_setup(pthread_attr_t pthread_attr);
void client_routine(void* arg);

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
    pthread_client_arg *pthread_arg;
    pthread_t pthread;
    pthread_mutex_t mux = PTHREAD_MUTEX_INITIALIZER;
    socklen_t client_address_len;

    //Init pthread param
    pthread_setup(pthread_attr);

    while(1){

        pthread_arg = (pthread_client_arg*) malloc(sizeof (pthread_client_arg));
        if(pthread_arg == NULL){
            perror("ERROR: server pthread_client_arg MALLOC\n");
            continue;
        }

        //Accept new connection
        if((new_sock_fd = accept(sock_fd, (struct sockaddr*)&pthread_arg->client_address, &client_address_len)) == -1){
            free(pthread_arg);
            perror("ERROR: server accept\n");
            continue;
        }

        pthread_arg->client_sock_fd = new_sock_fd;

        if(pthread_create(&pthread, NULL, (void *) &client_routine, pthread_arg) != 0){
            free(pthread_arg);
            perror("ERROR: server pthread_create\n");
            continue;
        }

    }

    return 0;
}

void client_routine(void* arg){
    User user;
    pthread_client_arg *pthread_arg = (pthread_client_arg*) arg;

    int client_sock_fd = pthread_arg->client_sock_fd;
    struct addrinfo client_address = pthread_arg->client_address;
    int queue_id;
    int n;

    char* client_id;
    char* client_password;

    user.sock_fd = pthread_arg->client_sock_fd;
    user.status = LOGOUT;

    Message recv_message, reply_message;
    char data_buff[1024];

    bool loop = true;
    while(loop){

        //Reset receive message and reply message
        memset(&recv_message, 0, sizeof (Message));
        memset(&reply_message, 0, sizeof (Message));
        memset(data_buff, 0, 1024);

        n = read(client_sock_fd, &recv_message, sizeof (Message));
        if(n < 0){
            perror("ERROR: server read\n");
            exit(EXIT_FAILURE);
        }
        else if(n == 0){
            delete_user(&user);
        }
        else if(n != sizeof (Message)){
            perror("WARNING: Message wrong size\n");
            continue;
        }
        else{

            //User log in log out
            switch (user.status){
                
                case LOGOUT:
                    
                    //Check login credential
                    switch (recv_message.type){
                        
                        //User login
                        case TYPE_LOGIN:
                            //Extract source information
                            client_id = (char*) recv_message.source;
                            client_password = (char*) recv_message.data;
                            printf("Client ID: %s, Password: %s\n", client_id, client_password);

                            //Check if user exist
                            bool user_exists = false;
                            for(int i=0; i<3; i++){
                                //Logging in if username and password matches
                                if((strcmp(clients[i].username, client_id)==0) && (strcmp(clients[i].password, client_password)==0)){
                                    user_exists = true;
                                    user.status = LOGIN;
<<<<<<< HEAD
                                    strcpy(user.username, clients[i].username); // Changed to strcpy
                                    
=======
                                    strcpy(user.username, clients[i].username);
>>>>>>> 5e4ff90319f1e73283fd6d16ab8a623f8f3529cc
                                    break;
                                }
                            }
                            
                            //Construct response message
                            if(user_exists){
                                reply_message.type = TYPE_LO_ACK;
                                reply_message.size = 0;
                                strcpy(reply_message.data,"0");
                                strcpy(reply_message.source,"0");
                            }else{
                                char* msg = "Invalid user name or password\n";
                                reply_message.type = TYPE_LO_NACK;
                                reply_message.size = sizeof(msg);
                                strcpy(reply_message.data,msg);
                            }

                            // printf("%d\n",reply_message.type);
                            // printf("%d\n",reply_message.size);
                            // printf("%s\n",reply_message.source);
                            // printf("%s\n",reply_message.data);

                            write(client_sock_fd, &reply_message, sizeof (Message));
                        break;

                        default: ;
                            char* msg = "ERROR: user login switch defaulting\n";
                            reply_message.type = TYPE_LO_NACK;
                            reply_message.size = sizeof(msg);
                            strcpy(reply_message.data,msg);
                            write(client_sock_fd, &reply_message, sizeof (Message));
                        break;
                    }
                break;


                case LOGIN:
                    //Check message type for logged in users
                    switch(recv_message.type){

                        //User exit
                        case TYPE_EXIT:
                            printf("User %s just exited from server\n", user.username);
                            delete_user(&user);
                            close(client_sock_fd);
                            loop = false;
                        break;


                        //User querying active users
                        case TYPE_QUERY:
                            query(data_buff);
                            reply_message.type = TYPE_QU_ACK;
                            reply_message.size = sizeof(data_buff);
                            strcpy(reply_message.data, data_buff);

                            write(client_sock_fd, &reply_message, sizeof(Message));
                        break;


                        //User new session
                        case TYPE_NEW_SESS:

                        break;


                        //User message
                        case TYPE_MESSAGE:
                            
                        break;


                        //User leave session
                        case TYPE_LEAVE_SESS:

                        break;


                        //User join
                        case TYPE_JOIN:

                        break;
                    }
                break;
                

                default:
                    perror("ERROR: user status switch defaulting\n");
                break;
            }



        }

    }
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
