#include "chatroom.h"

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


#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

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
