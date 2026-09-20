#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

struct metadata{
    size_t size_message;
    int listener;
};

void die(int ret_val, char* msg){
    if(ret_val == -1){
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

void secure_write(int socket, void* message, size_t size_message){

    size_t to_send = size_message;
    size_t sent = 0; 
    ssize_t ret;

    do{
        ret = write(socket, (char *)message + sent, to_send - sent); // we cast message to be sur to write octet by octet.
        if(ret != -1){
            sent += ret; 
        }else{
            // On recommence. 
        }
    }while(to_send != sent);
}

int read_http_request(int socket, char * buffer, size_t buffer_size){
    //read until \r\n\r\n is received. 
    size_t received = 0; 
    ssize_t ret;
    do{
        ret = read(socket, buffer + received, buffer_size - received);
        if(ret != -1){
            received += ret;
        }
        else if(ret == 0){
            exit(EXIT_SUCCESS);
        }
    }while(received < buffer_size && (received < 4 || strcmp("\r\n\r\n", (char*)buffer + received - 4) != 0));

    return ret; 
}

int read_and_print(int socket){

    char message[1024] = {"\0"};

    int ret = read_http_request(socket, message, sizeof(message));
    printf("%s", (char *)message);

    char answer[1024];

    char html_body[] = "<html>Hello !</html>";
    sprintf(answer, 
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: %zu\r\n"
            "\r\n\%s", 
            strlen(html_body), //
            html_body);


    secure_write(socket, answer, strlen(answer));
    return ret;
}


int main(){
    printf("Serveur is starting...\n");
    int s_listening = socket(AF_INET, SOCK_STREAM, 0);
    die(s_listening, "Error while creating socket");

    int yes = 1;
    setsockopt(s_listening, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in sockaddr_listening = {0};

    sockaddr_listening.sin_family = AF_INET;
    sockaddr_listening.sin_port = htons(1234);
    inet_aton("127.0.0.1", &sockaddr_listening.sin_addr);

    int ret_val = bind(s_listening, (const struct sockaddr *)&sockaddr_listening, sizeof(sockaddr_listening));
    die(ret_val, "Error while binding");

    ret_val = listen(s_listening, 20);
    die(ret_val, "Error while listening");
    
    struct sockaddr_in sockaddr_client = {0};
    socklen_t address_len = sizeof(sockaddr_client);

    ret_val = accept(s_listening, (struct sockaddr *)&sockaddr_client,  &address_len);
    die(ret_val, "Error while accepting.");

    int s_client = ret_val;

    while(1){
        read_and_print(s_client);
    }

    close(s_listening);
    close(s_client);


    return EXIT_SUCCESS;
}
