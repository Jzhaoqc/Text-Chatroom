#include "chatroom.h"

//create chatroom node, add to big list
void create_chatroom(char session_id[], User* user){
    Chatroom *current = NULL;
    Chatroom *traverse = NULL;

    pthread_mutex_lock(mux);
    
    //Check if it is the first room
    if(room_list_global->first_room == NULL){
        printf("[Server Function]: First room in list, creating the first room...\n");
        
        //Create new room and add to gobal list
        current = (Chatroom*) malloc(sizeof(Chatroom));
        room_list_global->first_room = current;
    }
    //Traverse to the last chatroom node and add new room at the end
    else{
        int count = 0;
        traverse = room_list_global->first_room;
        while(traverse->next != NULL){
            traverse = traverse->next;
            count++;
        }
        printf("[Server Function]: %d rooms in list, creating the another one...\n", count);

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

    pthread_mutex_unlock(mux);
}

//parse username, check from linked list. broadcast: send message to all file descriptors within chatroom members
void send_message(Message* recv_message){
    char* source_username = recv_message->source;
    char* message_content = recv_message->data;
    Chatroom* current_room = room_list_global->first_room;
    Member* current_member = room_list_global->first_room->first_member;
    bool found_source = false;

    //Find which room the user is from 
    while( (current_room != NULL) && !(found_source) ){
        //Find if source exist within current chatroom
        while(current_member != NULL){
            if( (strcmp(current_member->user->username, source_username)) == 0){
                found_source = true;
                break;
            }

            current_member = current_member->next;
        }

        current_room = current_room->next;
    }

    if(!found_source){
        printf("ERROR: send_message func should find the user but couldn't.\n");
        return;
    }

    //Sending message to everyone in the room
    current_member = current_room->first_member;
    while(current_member != NULL){
        
        //Send to every user in the room except for the source
        if(strcmp(current_member->user->username, source_username) == 0){
            continue;
        }else{
            //Construct reply message
            Message reply_msg;
            reply_msg.type = TYPE_MESSAGE;
            strcpy(reply_msg.source, source_username);
            reply_msg.size = sizeof(message_content);
            strcpy(reply_msg.data,message_content);

            write(current_member->user->sock_fd, &reply_msg, sizeof(Message));
        }

        current_member = current_member->next;
    }

}

void query(char buff[]) { 

    buff[0] = '\0'; // Ensure the buffer is initially empty

    Chatroom* current_room = room_list_global->first_room;

    while (current_room != NULL) {
        // Append the chat room name to the buffer
        snprintf(buff + strlen(buff), BUF_SIZE - strlen(buff), "%s\n", current_room->room_name);

        Member* current_member = current_room->first_member;
        while (current_member != NULL) {
            // Append the member's name to the buffer with a tab for indentation
            snprintf(buff + strlen(buff), BUF_SIZE - strlen(buff), "\t%s\n", current_member->user->username);

            current_member = current_member->next;
        }

        current_room = current_room->next;
    }

} //travers 2D linked list


// Assuming the structs and global variables you've defined earlier are available here.

bool join_user(User* user, char session_id[]) {
    
    pthread_mutex_lock(mux); 
    
    Chatroom* current_room = room_list_global->first_room;

    while (current_room != NULL) {

        if (strcmp(current_room->room_name, session_id) == 0) {
            
            Member* new_member = (Member*)malloc(sizeof(Member));
            
            new_member->user = user;
            new_member->is_owner = false; // Assuming not the owner
            new_member->next = NULL;
            
            // Insert in the beginning
            if (current_room->first_member != NULL) {
                new_member->next = current_room->first_member;
                current_room->first_member->prev = new_member;
            }
            current_room->first_member = new_member;
            new_member->prev = NULL;
            
            current_room->num_in_room += 1;
            
            pthread_mutex_unlock(mux); 
            return true; 
        }
        current_room = current_room->next;
    }
    
    pthread_mutex_unlock(mux); 
    return false; // Chat room not found
}
