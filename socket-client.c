#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

struct metadata{
    size_t size;
};

void die(int retvalue, char* msg){
    if(-1 == retvalue){
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

int secure_write(int socket, void* buffer, size_t size_msg){
    ssize_t ret;
    size_t sent = 0;
    do{
        ret = write(socket, (char*)buffer + sent, size_msg - sent);
        die(ret, "Error while writing");
        sent += ret;
    }while(sent != size_msg);
    return sent;
}

int write_with_protocol(int socket, void* buffer, size_t size_msg){
    struct metadata m;
    m.size = size_msg;
    int ret = secure_write(socket, &m, sizeof(m));
    ret = secure_write(socket, buffer, size_msg);
    return ret;
}

int main(){
    int s = socket(AF_INET, SOCK_STREAM, 0);
    die(s, "Error while creating socket");
    struct sockaddr_in sockaddr_client = {0};

    sockaddr_client.sin_family = AF_INET;
    sockaddr_client.sin_port = htons(6666);
    inet_aton("127.0.0.1", &sockaddr_client.sin_addr);

    int ret = connect(s, (const struct sockaddr*)&sockaddr_client, sizeof(sockaddr_client));
    die(ret, "Error while connecting");

    char buffer[256];

    while(fgets(buffer, 256, stdin) != NULL){
        write_with_protocol(s, buffer, strlen(buffer));
    }

    close(s);

    return EXIT_SUCCESS;
}
