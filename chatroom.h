#ifndef chatroom
#define chatroom

#include "stdio.h"
#include "pthread.h"
#include "stdbool.h"

//User status
#define LOGOUT 0
#define LOGIN 1
#define JOINED 2

//Message status
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
#define TYPE_REG 14

typedef struct message{
    unsigned int type;
    unsigned int size;
    unsigned char source[1024];
    unsigned char data[1024];
}Message;

typedef struct user{
    char username[100];
    int sock_fd;
    int status;
}User;

void delete_user(User* user);
void query(char buff[]);
bool join_user(User* user, char session_id[]);
void create_chatroom(char session_id[]);

#endif