#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>

#define SERVER_PORT 5090
#define BUFFER_SIZE 1024

int checksum(char * filepath, int n)
{
    int sum =0 ;
    char buffer[BUFFER_SIZE];
    FILE *f = fopen(filepath, "r");
    if (f == NULL) {
        perror("Error opening file");
        return -1;
    }
    while (fgets(buffer, BUFFER_SIZE, f) != NULL) {
        int i;
        for (i = 0; buffer[i] != '\0'; i++) {
            sum += buffer[i];
        }
    }
    return sum;
}

int main(int argc, char **argv)
{
    char *fileName = "200kb.txt";
    FILE *file;
    char buf[256];
    socklen_t length;

    // stage 1 - open tcp socket.
    int senderSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (senderSocket == -1)
    {
        perror("failed to open socket");
        return -1;
    }
    length = sizeof(buf);
    // stage 2 - create connection with measure.
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT); //network order

    int Connect = connect(senderSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (Connect == -1)
    {
        perror("error");
        return -1;
    }
    // stage 3/5 - send file.
    file = fopen(fileName, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Error in opening file");
        return 1;
    }
    char getReply[10];
    bzero(getReply, sizeof(getReply));
    read(senderSocket, getReply, sizeof(getReply));
    char sendbuffer[100];
    int b;
    int sum = 0;
    do
    {
        b = fread(sendbuffer, 1, sizeof(sendbuffer), file);
        int SendByte = send(senderSocket, sendbuffer, b, 0);
        sum = SendByte + sum;
    } while (!feof(file));
    sleep(1);
    //6. Close connection.
    close(senderSocket);

    sleep(1);
    
    return 0;
}