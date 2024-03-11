#include "chatroom.h"

//create chatroom node, add to big list
void create_chatroom(char session_id[], User* user){
    Chatroom *current = NULL;
    Chatroom *traverse = NULL;
    
    //Check if it is the first room
    if(room_list_global->first_room == NULL){
        printf("[Server]: First room in list, creating the first room...\n");
        
        //Create new room and add to gobal list
        current = (Chatroom*) malloc(sizeof(Chatroom));
        room_list_global->first_room = current;
    }
    //Traverse to the last chatroom node and add new room at the end
    else{
        traverse = room_list_global->first_room;
        while(traverse->next != NULL){
            traverse = traverse->next;
        }

        //Create new room and add to gobal list
        current = (Chatroom*) malloc(sizeof(Chatroom));
        traverse->next = current;
    }

    //Initialize room fields
    strcpy(current->room_name, session_id);
    current->num_in_room = 1;
    current->first_member = (Member*) malloc(sizeof(Member));
    current->first_member->user = user;
    current->first_member->is_owner = true;
}

//parse username, check from linked list. broadcast: send message to all file descriptors within chatroom members
void send_message(Message* recv_message){


}