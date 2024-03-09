#ifndef chatroom
#define chatroom

#include "stdio.h"
#include "pthread.h"

typedef struct user{
    char username[100];
    int sock_id;
    int status;
}User;
