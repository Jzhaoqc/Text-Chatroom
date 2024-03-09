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

// State of the client
#define START 0
#define LOGIN 1
#define JOINED 2

// Other defines
#define BUF_SIZE 1024

// Client State
int state = START;

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

int main(){

    printf("client\n");

    char USER_ID[BUF_SIZE];
    char USER_PWD[BUF_SIZE];
    char USER_IP[16];
    int USER_PORT;

    Message client_message;

    pthread_arg_t *recv_thread_arg;
    pthread_t recv_thread;
    memset(&recv_thread_arg, 0, sizeof(recv_thread_arg));

    while (1) {

        char *token;
        char USER_INPUT[BUF_SIZE];

        int sock_fd;
        struct sockaddr_in server_addr;

        if (fgets(USER_INPUT, sizeof(USER_INPUT), stdin) == NULL) {
            perror("Input error!\n");
            exit(1);                
        }
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

                printf("%s %s %s %d\n", USER_ID, USER_PWD, USER_IP, USER_PORT);

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

                printf("Before Connect\n");
                // Connect to server
                if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                    perror("TCP connection error!\n");
                    exit(1);
                }
                printf("Connected\n");
                // Create thread to listen for server reply
                if (pthread_create(&recv_thread, NULL, &recv_login, (void *) recv_thread_arg) != 0){
                    perror("pthread create error!\n");
                    exit(1);
                }
                printf("Created\n");
                if (pthread_detach(recv_thread) != 0) {
                    perror("detach error!\n");
                    exit(1);
                }
                
                // Send the message to server
                if (send(sock_fd, &client_message, sizeof(client_message), 0) != sizeof(client_message)) {
                    perror("Send error!\n");
                    exit(1);
                }
                printf("Sent\n");
                

            } else if (strcmp(token, QUIT_CMD) == 0) {
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
        4. Quit
        */
            // if (strcmp(token, LOGOUT_CMD) == 0) {

            //     // Create a exit message
            //     client_message.type = TYPE_EXIT;
            //     strcpy(client_message.data, NULL);
            //     client_message.size = sizeof(client_message.data);
            //     strcpy(client_message.source, USER_ID);

            //     // Send the message to server
            //     if (send(sock_fd, &client_message, sizeof(client_message), 0) != sizeof(client_message)) {
            //         perror("Send error!\n");
            //         exit(1);
            //     }

            // } else if (strcmp(token, JOINSESSION_CMD) == 0) {

            //     // Get the session id
            //     token = strtok(NULL, " ");
            //     int session_id = atoi(token);
            // }



            printf("Arrived at login!\n");
            break;

        }
    }

}

void *recv_login(void * arg) {

    pthread_arg_t *arguments = (pthread_arg_t*) arg;
    Message server_message;

    while (1) {
        if (read(arguments->socket_fd, &server_message, sizeof(server_message)) <= 0) {
            perror("Read error!\n");
            exit(1);
        }
        printf("Read\n");
        if (server_message.type == TYPE_LO_ACK) {
            printf("You are logged in!\n");
            state = LOGIN;
        } else if (server_message.type == TYPE_LO_NACK) {
            printf("Login failed for %s due to: %s\n", server_message.source, server_message.data);
        }   
    }
    
    return NULL;
}