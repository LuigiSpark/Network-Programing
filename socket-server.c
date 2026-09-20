#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

#define MAX_CONNECTION 2

int nbConnectionAlive = 0;

struct metadata{
    size_t size;
};

void die(int retvalue, char* msg){
    if(-1 == retvalue){
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

int secure_read(int socket, void* buffer, size_t size_buffer){
    int ret;
    size_t received = 0;
    while(received < size_buffer){
        ret = read(socket, (char*)buffer + received, size_buffer - received);
        die(ret, "Error while reading");
        if(ret == 0){
            break;
        }
        received += ret;
    }
    return received;
}
int secure_write(int socket, void*buffer, size_t size_msg){
    int ret;
    size_t sent = 0;
    do{
        ret = write(socket, (char*)buffer + sent, size_msg - sent);
        sent += ret;
        die(ret, "Error while wrinting");
    }while(sent != size_msg);
    return ret;
}

int read_and_print_with_protocol(int socket){
    // Receive the size through the structure. 
    struct metadata m; 
    int ret = secure_read(socket, &m, sizeof(m));
    if (ret == 0) {
        return 0; // client disconnected properly. 
    }

    // Receive the rest of the message. 
    char* buffer = malloc(m.size);
    ret = secure_read(socket, buffer, m.size);
    printf("%s", buffer);
    free(buffer);
    return ret;
}

// Update the poll structure (fds) when new connection occurs through the main socket (s_connection).
 void accept_from_connection(int s_connection, struct pollfd fds[]){
        struct sockaddr_in socket_client = {0};
        socklen_t socket_client_len = sizeof(socket_client);
    
        int ret = accept(s_connection, (struct sockaddr* restrict)&socket_client, &socket_client_len);  
        die(ret, "Error while accepting");
        int s_client = ret;

        //Search for the first free case in fds. 
        for(int i = 0; i < MAX_CONNECTION + 1; i++){
            if(fds[i].fd == -1){
                fds[i].fd = s_client;
                fds[i].events = POLLIN;
                fds[i].revents = 0;
                nbConnectionAlive++;
                printf("Client %d has been accepted.\n", i);
                return;
            }
        }
        // If fds is full.
        printf("Maximal of connection reached : client refused.\n");
        close(s_client);

    }


int main(){
    printf("Server is starting...\n");
    int s_connection = socket(AF_INET, SOCK_STREAM, 0);

    int yes = 1;
    setsockopt(s_connection, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in socket_server = {0};

    socket_server.sin_port = htons(6666);
    socket_server.sin_family = AF_INET;
    inet_aton("127.0.0.1", &socket_server.sin_addr);

    int ret = bind(s_connection, (const struct sockaddr*)&socket_server, sizeof(socket_server));
    die(ret, "Error while binding");

    ret = listen(s_connection, MAX_CONNECTION);
    die(ret, "Error while listening");

    struct pollfd fds[MAX_CONNECTION+1];
    for(int i = 1; i < MAX_CONNECTION+1; i++){
        fds[i].fd = -1;
    }

    fds[0].fd = s_connection; 
    fds[0].events = POLLIN; 
    fds[0].revents = 0;

    // Wait for the first connection to put nbConnectionAlive to 1 to enter in the while loop.
    accept_from_connection(s_connection, fds);

    while(nbConnectionAlive > 0){
        int ret = poll(fds, MAX_CONNECTION+1, -1); // -1 : we wait indefinitively.
        die(ret, "Error while polling");
        for(int i = 0; i < MAX_CONNECTION+1; i++){
            if(fds[i].revents & POLLIN){
                if(0 == i){
                    accept_from_connection(s_connection, fds);
                }else{
                    int ret = read_and_print_with_protocol(fds[i].fd);
                    die(ret, "Error while reading");
                    if(ret == 0){
                        close(fds[i].fd);
                        nbConnectionAlive--;
                        fds[i].fd = -1; // Inform to the poll function to skip this case of the array.
                        printf("Client %d has been disconnected.\n", i);
                    }
                    fds[i].revents = 0;
                }
            }
        }
    }

    close(s_connection);
    
    return EXIT_SUCCESS;

}
