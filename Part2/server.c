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


typedef struct pthread_client_info{
    int client_sock_fd;
    struct addrinfo client_address;
    int size;
}pthread_client_arg;

// Part of the code is cited from https://beej.us/guide/bgnet/

//Function declarations
void signal_setup(int sock_fd);
void signal_handler(int signal);
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

    //pthread_attr_t pthread_attr;
    pthread_client_arg *pthread_arg;
    pthread_t pthread;
    //pthread_mutex_t mux = PTHREAD_MUTEX_INITIALIZER;
    socklen_t client_address_len;

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

        if(pthread_detach(pthread) != 0){
            perror("ERROR: thread pthread_detach\n");
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
    char* priv_msg_target;
    char* priv_msg_content;

    user.sock_fd = pthread_arg->client_sock_fd;
    user.status = LOGOUT;

    Message recv_message, reply_message;
    char data_buff[1024];
    char session_id[1024];
    bool can_join = false;

    bool loop = true;
    while(loop){

        //Reset receive message and reply message
        memset(&recv_message, 0, sizeof (Message));
        memset(&reply_message, 0, sizeof (Message));
        memset(data_buff, 0, 1024);
        memset(session_id, 0, 1024);

        //printf("\n\n[Server]: Waiting for packet...\n");
        n = read(client_sock_fd, &recv_message, sizeof (Message));
        if(n < 0){
            perror("ERROR: server read\n");
            exit(EXIT_FAILURE);
        }
        else if(n == 0){
            //need implementation
            delete_user(&user, false);
        }
        else if(n != sizeof (Message)){
            perror("WARNING: Message wrong size\n");
            continue;
        }
        else{

            printf("----Packet Content----\n");
            printf("Message source: %s\n", recv_message.source);
            printf("Message type: %d\n", recv_message.type);
            printf("Message content: %s\n", recv_message.data);
            printf("----------------------\n");

            //User log in log out
            switch (user.status){
                
                case LOGOUT:
                    
                    //Check login credential
                    switch (recv_message.type){
                        
                        //User login
                        case TYPE_LOGIN:
                            printf("[Server]: Received Login request.\n");

                            //Extract source information
                            client_id = (char*) recv_message.source;
                            client_password = (char*) recv_message.data;
                            printf("    Client ID: %s, Password: %s\n\n\n", client_id, client_password);

                            //Check if user exist
                            bool user_exists = false;
                            for(int i=0; i<3; i++){
                                //Logging in if username and password matches
                                if((strcmp(clients[i].username, client_id)==0) && (strcmp(clients[i].password, client_password)==0)){
                                    clients[i].isOnline = true;
                                    user_exists = true;
                                    user.status = LOGIN;
                                    strcpy(user.username, clients[i].username);
                                    break;
                                }
                            }
                            
                            //Construct response message
                            if(user_exists){
                                reply_message.type = TYPE_LO_ACK;
                            }else{
                                char* msg = "Invalid user name or password\n";
                                reply_message.type = TYPE_LO_NACK;
                                reply_message.size = sizeof(msg);
                                strcpy(reply_message.data,msg);
                            }
                            write(client_sock_fd, &reply_message, sizeof (Message));

                            //debug
                            printf("\nUser list:\n");
                            for(int i=0; i<3; i++){
                                printf("%s\n",clients[i].username);
                                printf("%d\n",clients[i].isOnline);
                            }

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
                            printf("[Server]: Received exit request.\n");
                            printf("    User %s just exited from server\n\n\n", user.username);
                            //need implementation
                            delete_user(&user,true);
                            close(client_sock_fd);
                            loop = false;

                            client_id = (char*) recv_message.source;
                            for(int i=0; i<3; i++){
                                //Logging in if username and password matches
                                if((strcmp(clients[i].username, client_id))==0){
                                    clients[i].isOnline = false;
                                    user.status = LOGIN;
                                    break;
                                }
                            }

                        break;


                        //User querying active users
                        case TYPE_QUERY:
                            printf("[Server]: Received query request.\n");
                            //need testing
                            query(data_buff);
                            reply_message.type = TYPE_QU_ACK;
                            reply_message.size = sizeof(data_buff);
                            strcpy(reply_message.data, data_buff);

                            write(client_sock_fd, &reply_message, sizeof(Message));
                        break;


                        //User new session
                        case TYPE_NEW_SESS:
                            printf("[Server]: Received new session request.\n");

                            strcpy(session_id,recv_message.data);               //might need client to add \0 at the end of the string
                            reply_message.size = sizeof(session_id);
                            reply_message.type = TYPE_NS_ACK;
                            strcpy(reply_message.data,session_id);

                            //need testing
                            create_chatroom(session_id, &user);
                            user.status = JOINED;

                            write(client_sock_fd, &reply_message, sizeof (Message));
                            printf("    New session: %s, created.\n\n\n", session_id);
                        break;


                        //User join
                        case TYPE_JOIN:
                            printf("[Server]: Received join request.\n");

                            //Get session id and join user
                            strcpy(session_id,recv_message.data);
                            //need testing
                            can_join = join_user(&user, session_id);
                            if(can_join){
                                user.status = JOINED;
                                reply_message.type = TYPE_JN_ACK;
                                reply_message.size = strlen(session_id);
                                strcpy(reply_message.data,session_id);
                                printf("    User: %s joined %s.\n\n\n", user.username, session_id);
                            }else{
                                printf("    User: %s failed to joined %s.\n\n\n", user.username, session_id);
                                reply_message.type = TYPE_JN_NACK;
                                strcat(session_id, ", ERROR: unable to join, invalid session ID.\n");
                                strcpy(reply_message.data, session_id);
                                reply_message.size = strlen(session_id);
                            }
                            write(client_sock_fd, &reply_message, sizeof(Message));
                        break;

                        case TYPE_PRIV_MESSAGE:
                            printf("[Server]: Received private message from %s.\n", recv_message.source);
                            strcpy(data_buff, recv_message.data);
                            
                            //Extract private message target and content
                            priv_msg_target = strtok(data_buff," ");
                            priv_msg_content = strtok(NULL, " ");
                            printf("    priv target: %s\n", priv_msg_target);
                            printf("    priv content: %s\n", priv_msg_content);

                            //Check if target logged in
                            bool is_logged_in = false;
                            for(int i=0; i<3; i++){
                                if(strcmp(clients[i].username, priv_msg_target) == 0 ){
                                    if(clients[i].isOnline){
                                        is_logged_in = true;
                                    }
                                    break;
                                }
                            }
                            if(is_logged_in){
                                priv_message(&recv_message);
                            }else{
                                printf("    target is not logged in, discarding message..\n");
                            }
                        break;

                    }
                break;
                

                case JOINED:
                    switch(recv_message.type){
                        
                        //User querying active users
                        case TYPE_QUERY:
                            printf("[Server]: Received query request.\n");

                            //need testing
                            query(data_buff);
                            reply_message.type = TYPE_QU_ACK;
                            reply_message.size = sizeof(data_buff);
                            strcpy(reply_message.data, data_buff);

                            write(client_sock_fd, &reply_message, sizeof(Message));
                        break;


                        //User message
                        case TYPE_MESSAGE:
                            printf("[Server]: Received message from %s.\n", recv_message.source);
                            //need testing
                            send_message(&recv_message);
                            printf("loop: %d\n", loop);
                            printf("user status: %d\n", user.status);
                        break;

                        //User private message
                        case TYPE_PRIV_MESSAGE:
                            printf("[Server]: Received private message from %s.\n", recv_message.source);
                            strcpy(data_buff, recv_message.data);
                            
                            //Extract private message target and content
                            priv_msg_target = strtok(data_buff," ");
                            priv_msg_content = strtok(NULL, " ");
                            printf("    priv target: %s\n", priv_msg_target);
                            printf("    priv content: %s\n", priv_msg_content);

                            //Check if target logged in
                            bool is_logged_in = false;
                            for(int i=0; i<3; i++){
                                if(strcmp(clients[i].username, priv_msg_target) == 0 ){
                                    if(clients[i].isOnline){
                                        is_logged_in = true;
                                    }
                                    break;
                                }
                            }
                            if(is_logged_in){
                                priv_message(&recv_message);
                            }else{
                                printf("    target is not logged in, discarding message..\n");
                            }
                        break;


                        //User leave session
                        case TYPE_LEAVE_SESS:
                            printf("[Server]: Received leave session request.\n");

                            strcpy(session_id, recv_message.data);
                            //need implementation
                            delete_user(&user,false);
                            user.status = LOGIN;

                            printf("    User: %s left session: %s.\n\n\n", user.username, session_id);
                        break;


                        //User leave server
                        case TYPE_EXIT:
                            printf("[Server]: Received exit request.\n");
                        
                            printf("    User %s just exited from server\n\n\n", user.username);
                            //need implementation
                            delete_user(&user, true);
                            close(client_sock_fd);
                            loop = false;

                            client_id = (char*) recv_message.source;
                            for(int i=0; i<3; i++){
                                //Logging in if username and password matches
                                if((strcmp(clients[i].username, client_id))==0){
                                    clients[i].isOnline = false;
                                    user.status = LOGIN;
                                    break;
                                }
                            }


                        break;
                    }
                break;


                default:
                    perror("ERROR: user status switch defaulting\n");
                break;
            }



        }
    }
    printf("Thread exiting\n");
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
