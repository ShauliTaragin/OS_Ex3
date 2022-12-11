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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

#define SERVER_PORT 5090
#define BUFFER_SIZE 1024

#define SOCK_PATH "echo_sock"
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
    fclose(f);
    return sum;
}


void UDS_Stream_Socket(){
    int length;
    char *fileName = "shauli.txt";
    FILE *file;
    char buf[256];
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(sockfd == -1){
        perror("failed to open socket");
    }

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCK_PATH);
    length = strlen(addr.sun_path) + sizeof(addr.sun_family);
    int Connect = connect(sockfd, (struct sockaddr*)&addr, length);
    if (Connect == -1)
    {
        perror("error in connect to usd");
        return ;
    }

    file = fopen(fileName, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Error in opening file");
        return;
    }
    char getReply[10];
    // bzero(getReply, sizeof(getReply));
    // read(sockfd, getReply, sizeof(getReply));
    char sendbuffer[100];
    int checkSumAns = checksum(fileName,0);
    char checkSum[10];
    sprintf(checkSum,"%d",checkSumAns);
    send(sockfd, checkSum, 10, 0);
    printf("checkSum Calculated ===>   %s\n",checkSum);
    // int SendByte = send(senderSocket,checkSum,20,0);
    int b;
    int sum = 0;
    do
    {
        bzero(sendbuffer,sizeof(sendbuffer));
        b = fread(sendbuffer, 1, sizeof(sendbuffer), file);
        // printf("%s\n",sendbuffer);
        int SendByte = send(sockfd, sendbuffer, sizeof(sendbuffer), 0);
        // printf("%d\n",SendByte);
        sum = SendByte + sum;
    } while (!feof(file));
   
    sleep(1);
    //6. Close connection.
    fclose(file);
    close(sockfd);

    sleep(1);
}
void TCP(){
    char *fileName = "shauli.txt";
    FILE *file;
    char buf[256];
    socklen_t length;

    // stage 1 - open tcp socket.
    int senderSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (senderSocket == -1)
    {
        perror("failed to open socket");
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
        return ;
    }
    // stage 3/5 - send file.
    file = fopen(fileName, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Error in opening file");
        return;
    }
    char getReply[10];
    bzero(getReply, sizeof(getReply));
    read(senderSocket, getReply, sizeof(getReply));
    char sendbuffer[100];
    int checkSumAns = checksum(fileName,0);
    char checkSum[20];
    sprintf(checkSum,"%d",checkSumAns);
    send(senderSocket,checkSum,strlen(checkSum),0);
    printf("checkSum Calculated ===>   %s\n",checkSum);
    // int SendByte = send(senderSocket,checkSum,20,0);
    int b;
    int sum = 0;
    do
    {
        printf("entered");
        bzero(sendbuffer,sizeof(sendbuffer));
        b = fread(sendbuffer, 1, sizeof(sendbuffer), file);
        int SendByte = send(senderSocket, sendbuffer, strlen(sendbuffer), 0);
        sum = SendByte + sum;
    } while (!feof(file));
   
    sleep(1);
    //6. Close connection.
    close(senderSocket);

    sleep(1);
}

int main(int argc, char **argv)
{
    if(strcmp(argv[1],"a")==0){//case for tcp
        TCP();
    }
    if(strcmp(argv[1],"b")==0){//case for tcp
        UDS_Stream_Socket();
    }
    return 1;
}