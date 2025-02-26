#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

// Client Commands
const char *LOGIN_CMD = "/login";
const char *LOGOUT_CMD = "/logout";
const char *JOINSESSION_CMD = "/joinsession";
const char *LEAVESESSION_CMD = "/leavesession";
const char *CREATESESSION_CMD = "/createsession";
const char *LIST_CMD = "/list";
const char *QUIT_CMD = "/quit";
const char *PRIV_MESSAGE_CMD = "/p-msg";

//defines constants for message types
#define TYPE_LOGIN 1
#define TYPE_LO_ACK 2
#define TYPE_LO_NACK 3
#define TYPE_EXIT 4
#define TYPE_JOIN 5
#define TYPE_JN_ACK 6
#define TYPE_JN_NACK 7
#define TYPE_LEAVE_SESS 8
#define TYPE_NEW_SESS 9
#define TYPE_NS_ACK 10
#define TYPE_MESSAGE 11
#define TYPE_QUERY 12
#define TYPE_QU_ACK 13
#define TYPE_PRIV_MESSAGE 14

// State of the client
#define START 0
#define LOGIN 1
#define JOINED 2

// Other defines
#define BUF_SIZE 1024

// Client State
int state = START;
int current_type;
int threadLoop;

char USER_IN_SAVE[BUF_SIZE];

// Struct
typedef struct Message {
	unsigned int type;
	unsigned int size; 
	unsigned char source[BUF_SIZE]; 
	unsigned char data[BUF_SIZE];
}Message;

typedef struct pthread_arg_t {
    int socket_fd;
    char clientID[BUF_SIZE];
} pthread_arg_t;

void *recv_login(void * arg);

// /login user1 123 128.100.13.166 123456

int main(){

    printf("client\n");

    char USER_ID[BUF_SIZE];
    char USER_PWD[BUF_SIZE];
    char USER_IP[16];
    int USER_PORT;

    Message client_message;
    int sock_fd;
    struct sockaddr_in server_addr;

    pthread_arg_t *recv_thread_arg;
    pthread_t recv_thread;
    recv_thread_arg = (pthread_arg_t *)malloc(sizeof *recv_thread_arg); 

    while (1) {

        char *token;
        char USER_INPUT[BUF_SIZE];
        
        memset(&USER_IN_SAVE, 0, BUF_SIZE);

        memset(&client_message.size, 0, sizeof(client_message.size));
        memset(&client_message.data, 0, sizeof(client_message.data));

        if (fgets(USER_INPUT, sizeof(USER_INPUT), stdin) == NULL) {
            perror("Input error!\n");
            exit(1);                
        }
        strcpy(USER_IN_SAVE, USER_INPUT);

        USER_INPUT[strlen(USER_INPUT) - 1] = 0; // Remove ending character
        token = strtok(USER_INPUT, " ");

        switch (state) {
        case START:
        /*
        Choices at START stage 
        1. Login
        2. Quit
        */
            if (strcmp(token, LOGIN_CMD) == 0) {
                
                // Get all the input from the client
                token = strtok(NULL, " ");
                if (token == NULL) {
                    printf("Not enough arguments.\n");
                    break;
                }
                strcpy(USER_ID, token);

                token = strtok(NULL, " ");
                if (token == NULL) {
                    printf("Not enough arguments.\n");
                    break;
                }
                strcpy(USER_PWD, token);

                token = strtok(NULL, " ");
                if (token == NULL) {
                    printf("Not enough arguments.\n");
                    break;
                }
                strcpy(USER_IP, token);

                token = strtok(NULL, " ");
                if (token == NULL) {
                    printf("Not enough arguments.\n");
                    break;
                }
                USER_PORT = atoi(token);

                //printf("%s %s %s %d\n", USER_ID, USER_PWD, USER_IP, USER_PORT);

                // Create client message
                client_message.type = TYPE_LOGIN;
                client_message.size = strlen(USER_PWD);
                strcpy(client_message.data, USER_PWD);
                strcpy(client_message.source, USER_ID);

                sock_fd = socket(AF_INET, SOCK_STREAM, 0);
                if (sock_fd < 0) {
                    perror("Socket error!\n");
                    exit(1);
                }

                server_addr.sin_family = AF_INET;
                server_addr.sin_port = htons(USER_PORT);
                server_addr.sin_addr.s_addr = inet_addr(USER_IP);
                
                // Connect to server
                if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                    perror("TCP connection error!\n");
                    exit(1);
                }

                printf("Connected!\n");

                strcpy(recv_thread_arg->clientID, USER_ID);
                recv_thread_arg->socket_fd = sock_fd;
                threadLoop = true;
                
                // Create thread to listen for server reply
                if (pthread_create(&recv_thread, NULL, recv_login, (void *) recv_thread_arg) != 0){
                    perror("pthread create error!\n");
                    exit(1);
                }
                
                if (pthread_detach(recv_thread) != 0) {
                    perror("detach error!\n");
                    exit(1);
                }
                
                // Send the message to server
                if (send(sock_fd, &client_message, sizeof(client_message), 0) != sizeof(Message)) {
                    perror("Send error!\n");
                    exit(1);
                }
                // Change type for reading ACK
                current_type = TYPE_LOGIN;

                printf("User Message Sent!\n");
                
            } else if (strcmp(token, QUIT_CMD) == 0) {
                // Create a exit message
                client_message.type = TYPE_EXIT;
                strcpy(client_message.source, USER_ID);

                // Send the message to server
                if (send(sock_fd, &client_message, sizeof(client_message), 0) != sizeof(Message)) {
                    perror("Send error!\n");
                    exit(1);
                }
                printf("Message sent!\n");

                printf("Program exited.\n");
                exit(0);
            }

            break;

        case LOGIN: 
        /*
        Choices at this stage:
        1. Logout
        2. Join
        3. Create
        4. List
        5. Private Message
        6. Quit
        */
            if (strcmp(token, LOGOUT_CMD) == 0) {
                
                threadLoop = false;

                // Create a exit message
                client_message.type = TYPE_EXIT;
                strcpy(client_message.source, USER_ID);

                // Send the message to server
                if (send(sock_fd, &client_message, sizeof(client_message), 0) != sizeof(Message)) {
                    perror("Send error!\n");
                    exit(1);
                }
                printf("Message sent!\n");

                state = START;
                current_type = TYPE_LOGIN;
                printf("You have logged out, please re-login.\n");

            } else if (strcmp(token, JOINSESSION_CMD) == 0) {
                
                // Change type for reading ACK
                current_type = TYPE_JOIN;

                // Get the session id
                token = strtok(NULL, " ");
                char *session_id = token;
                
                // Create client message
                client_message.type = TYPE_JOIN;
                strcpy(client_message.source, USER_ID);
                strcpy(client_message.data, session_id);
                client_message.size = sizeof(client_message.data);

                // Send the message to server
                if (send(sock_fd, &client_message, sizeof(client_message), 0) != sizeof(Message)) {
                    perror("Send error!\n");
                    exit(1);
                }
                
                printf("Message sent!\n");

            } else if (strcmp(token, CREATESESSION_CMD) == 0) {

                // Change type for reading ACK
                current_type = TYPE_NEW_SESS;

                // Get the session id
                token = strtok(NULL, " ");
                char *session_id = token;
                
                // Create client message
                client_message.type = TYPE_NEW_SESS;
                strcpy(client_message.source, USER_ID);
                strcpy(client_message.data, session_id);
                client_message.size = sizeof(client_message.data);

                // Send the message to server
                if (send(sock_fd, &client_message, sizeof(client_message), 0) != sizeof(Message)) {
                    perror("Send error!\n");
                    exit(1);
                }
                printf("Message sent!\n");

            } else if (strcmp(token, LIST_CMD) == 0) {

                // Create client message
                current_type = TYPE_QUERY;

                client_message.type = TYPE_QUERY;
                strcpy(client_message.source, USER_ID);
                client_message.size = sizeof(client_message.data);

                // Send the message to server
                if (send(sock_fd, &client_message, sizeof(client_message), 0) != sizeof(Message)) {
                    perror("Send error!\n");
                    exit(1);
                }
                printf("Message sent!\n");

            } else if (strcmp(token, PRIV_MESSAGE_CMD) == 0) {

                char target[BUF_SIZE];
                char msg[BUF_SIZE];

                token = strtok(NULL, " "); 
                if (token == NULL) {
                    printf("Not enough arguments.\n");
                    break;
                }
                strcpy(target, token);

                token = strtok(NULL, "");
                if (token == NULL) {
                    printf("Not enough arguments.\n");
                    break;
                }
                strcpy(msg, token);

                strcat(target, " ");
                strcat(target, msg);
                printf("target: %s\n", target);

                client_message.type = TYPE_PRIV_MESSAGE;
                strcpy(client_message.source, USER_ID);
                strcpy(client_message.data, target);
                client_message.size = sizeof(client_message.data);

                // Send the message to server
                if (send(sock_fd, &client_message, sizeof(client_message), 0) != sizeof(Message)) {
                    perror("Send error!\n");
                    exit(1);
                }

                printf("Message sent!\n");

            } else if (strcmp(token, QUIT_CMD) == 0) {
                // Create a exit message
                client_message.type = TYPE_EXIT;
                strcpy(client_message.source, USER_ID);

                // Send the message to server
                if (send(sock_fd, &client_message, sizeof(client_message), 0) != sizeof(Message)) {
                    perror("Send error!\n");
                    exit(1);
                }
                printf("Message sent!\n");

                printf("Program exited.\n");
                exit(0);
            }

            break;

        case JOINED:
        /*
        Choices at this stage:
        1. Logout
        2. Leave
        3. List
        4. Private Message
        5. Join another
        6. Quit
        7. Message
        */
            if (strcmp(token, LOGOUT_CMD) == 0) {

                threadLoop = false;

                // Create a exit message
                client_message.type = TYPE_EXIT;
                strcpy(client_message.source, USER_ID);

                // Send the message to server
                if (send(sock_fd, &client_message, sizeof(client_message), 0) != sizeof(Message)) {
                    perror("Send error!\n");
                    exit(1);
                }
                printf("Message sent!\n");
                state = START;
                current_type = TYPE_LOGIN;
                printf("You have logged out, please re-login.\n");

            } else if (strcmp(token, LEAVESESSION_CMD) == 0) {

                // Create a exit message
                client_message.type = TYPE_LEAVE_SESS;
                strcpy(client_message.source, USER_ID);

                // Send the message to server
                if (send(sock_fd, &client_message, sizeof(client_message), 0) != sizeof(Message)) {
                    perror("Send error!\n");
                    exit(1);
                }

                state = LOGIN;
                
                printf("Message sent!\n");

            } else if (strcmp(token, LIST_CMD) == 0) {

                client_message.type = TYPE_QUERY;
                strcpy(client_message.source, USER_ID);
                client_message.size = sizeof(client_message.data);

                // Send the message to server
                if (send(sock_fd, &client_message, sizeof(client_message), 0) != sizeof(Message)) {
                    perror("Send error!\n");
                    exit(1);
                }
                // Create client message
                current_type = TYPE_QUERY;
                printf("Message sent!\n");

            } else if (strcmp(token, PRIV_MESSAGE_CMD) == 0) {

                char target[BUF_SIZE];
                char msg[BUF_SIZE];

                token = strtok(NULL, " "); 
                if (token == NULL) {
                    printf("Not enough arguments.\n");
                    break;
                }
                strcpy(target, token);

                token = strtok(NULL, "");
                if (token == NULL) {
                    printf("Not enough arguments.\n");
                    break;
                }
                strcpy(msg, token);

                strcat(target, " ");
                strcat(target, msg);
                printf("target: %s\n", target);

                client_message.type = TYPE_PRIV_MESSAGE;
                strcpy(client_message.source, USER_ID);
                strcpy(client_message.data, target);
                client_message.size = sizeof(client_message.data);

                // Send the message to server
                if (send(sock_fd, &client_message, sizeof(client_message), 0) != sizeof(Message)) {
                    perror("Send error!\n");
                    exit(1);
                }

                printf("Message sent!\n");

            } else if (strcmp(token, JOINSESSION_CMD) == 0) {
                
                // Change type for reading ACK
                current_type = TYPE_JOIN;

                // Get the session id
                token = strtok(NULL, " ");
                char *session_id = token;
                
                // Create client message
                client_message.type = TYPE_JOIN;
                strcpy(client_message.source, USER_ID);
                strcpy(client_message.data, session_id);
                client_message.size = sizeof(client_message.data);

                // Send the message to server
                if (send(sock_fd, &client_message, sizeof(client_message), 0) != sizeof(Message)) {
                    perror("Send error!\n");
                    exit(1);
                }
                
                printf("Message sent!\n");

            } else if (strcmp(token, QUIT_CMD) == 0) {

                // Create a exit message
                client_message.type = TYPE_EXIT;
                strcpy(client_message.source, USER_ID);

                // Send the message to server
                if (send(sock_fd, &client_message, sizeof(client_message), 0) != sizeof(Message)) {
                    perror("Send error!\n");
                    exit(1);
                }
                printf("Message sent!\n");

                printf("Program exited.\n");
                exit(0);
            } else {

                client_message.type = TYPE_MESSAGE;
                strcpy(client_message.source, USER_ID);
                strcpy(client_message.data, USER_IN_SAVE);
                client_message.size = sizeof(client_message.data);

                // Send the message to server
                if (send(sock_fd, &client_message, sizeof(client_message), 0) != sizeof(Message)) {
                    perror("Send error!\n");
                    exit(1);
                }
                //printf("Message: %s\n", client_message.data);
                printf("Message sent!\n");

            }
            break;

        }
    }

}

void *recv_login(void * arg) {

    pthread_arg_t *arguments = (pthread_arg_t*) arg;
    Message server_message;

    while (threadLoop) {
        memset(&server_message, 0, sizeof(Message));

        read(arguments->socket_fd, &server_message, sizeof(server_message));

        // if (read(arguments->socket_fd, &server_message, sizeof(server_message)) <= 0) {
        //     perror("Read error!\n");
        //     exit(1);
        // }
        
        if (server_message.type == TYPE_MESSAGE) {
            printf("%s\n", server_message.data);
        } else {

            switch (current_type) {
            case TYPE_LOGIN:
            
                // if (read(arguments->socket_fd, &server_message, sizeof(server_message)) <= 0) {
                //     perror("Read error!\n");
                //     exit(1);
                // }   

                if (server_message.type == TYPE_LO_ACK) {
                    printf("You are logged in!\n");
                    state = LOGIN;
                } else if (server_message.type == TYPE_LO_NACK) {
                    printf("Login failed for %s due to: %s\n", server_message.source, server_message.data);
                }   
                current_type = -1;
                break;

            case TYPE_JOIN:
                // if (read(arguments->socket_fd, &server_message, sizeof(server_message)) <= 0) {
                //     perror("Read error!\n");
                //     exit(1);
                // }

                if (server_message.type == TYPE_JN_ACK) {
                    printf("You have joined session: %s!\n", server_message.data);
                    state = JOINED;
                } else if (server_message.type == TYPE_JN_NACK) {
                    char *session = strtok(server_message.data, " ");
                    char *reason = strtok(NULL, " ");
                    printf("Join failed for session %s due to: %s\n", session, reason);
                }   
                current_type = -1;
                break;
            
            case TYPE_NEW_SESS:
                
                // if (read(arguments->socket_fd, &server_message, sizeof(server_message)) <= 0) {
                //     perror("Read error!\n");
                //     exit(1);
                // }

                if (server_message.type == TYPE_NS_ACK) {
                    printf("You have created and joined session: %s!\n", server_message.data);
                    state = JOINED;
                }
                current_type = -1;
                break;

            case TYPE_QUERY:
                // if (read(arguments->socket_fd, &server_message, sizeof(server_message)) <= 0) {
                //     perror("Read error!\n");
                //     exit(1);
                // }

                if (server_message.type == TYPE_QU_ACK) {
                    printf("List of Users and Sessions: \n%s", server_message.data);
                }
                current_type = -1;
                break;
            }

        }
        
    }
    printf("thread exiting\n");
    return NULL;
}

// /login user1 123 128.100.13.166 123456
// /login user2 123 128.100.13.166 123456
// /login user3 123 128.100.13.166 123456