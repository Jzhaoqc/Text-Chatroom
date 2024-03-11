#ifndef chatroom
#define chatroom

#include "stdio.h"
#include "pthread.h"
#include "stdbool.h"
#include "string.h"
#include "stdlib.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "unistd.h"
#include "errno.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "netdb.h"
#include "signal.h"
#include "pthread.h"

//User status
#define LOGOUT 0
#define LOGIN 1
#define JOINED 2

#define BUF_SIZE 1024

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

typedef struct member{
    User* user;
    bool is_owner;
    struct member* prev;
    struct member* next;
}Member;


typedef struct chat_room{
    Member* first_member;
    int num_in_room;
    char room_name[1024];
    struct chat_room* prev;
    struct chat_room* next;
}Chatroom;

typedef struct chatroomlist{
    Chatroom* first_room;
}Chatroom_List;


extern Chatroom_List* room_list_global;
extern pthread_mutex_t mux;

void delete_user(User* user);
void query(char buff[]); //travers 2D linked list
bool join_user(User* user, char session_id[]);  //check session exit, if so add user as member to chatroom
void create_chatroom(char session_id[], User* user);    //create chatroom node, add to big list
void send_message(Message* recv_message);   //parse username, check from linked list. broadcast: send message to all file descriptors within chatroom members

//debug func
void print_all_room();

#endif