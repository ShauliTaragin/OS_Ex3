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
#include <strings.h>
#include <arpa/inet.h>
#include <signal.h>

#define SERVER_PORT 5090
#define BUFFER_SIZE 1024
// function to calculate the checksum
static int sum = 0;
void checksum(char * buffer, int n)
{
    // int sum =0 ;
    // char buffer[BUFFER_SIZE];
    // FILE *f = fopen(filepath, "r");
    // if (f == NULL) {
    //     perror("Error opening file");
    //     return -1;
    // }
    // while (fgets(buffer, BUFFER_SIZE, f) != NULL) {
    //     int i;
        for (int i = 0; i<n; i++) {
            sum += buffer[i];
        }
    
    // fclose(f);
    // return sum;
}
int main()
{
    char buffer[100];
    // printf("%d\n",checksum("100mb.txt",0));
    //  1. open a new listening socket.
    int listeningSocket = -1;
    listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listeningSocket == -1)
    {
        perror("Error: Cannot open a new socket\n");
        return -1;
    }
    //  2. Listening to incoming connections.
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_family = AF_INET;
    struct timeval begin, end;
    if (bind(listeningSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        printf("Bind() has failed with the error code: %d", errno);
        return -1;
    }
    if (listen(listeningSocket, 500) == -1) //500 is a Maximum size of queue connection requests
                                            //number of concurrent connections
    {
        printf("listen failed");
    }
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLen = sizeof(clientAddress);

        printf("Waiting for incoming TCP-connections...\n");
        double time = 0.0;
        memset(&clientAddress, 0, sizeof(clientAddress));
        clientAddressLen = sizeof(clientAddress);
        // stage 3 - accept the socket from client.
        int clientSocket = accept(listeningSocket, (struct sockaddr *)&clientAddress, &clientAddressLen);
        if (clientSocket == -1)
        {
            printf("listen has failed\n");
            close(listeningSocket);
            return -1;
        }
        char response[10] = "OK";
        write(clientSocket, response, sizeof(response));
        gettimeofday(&begin, NULL);
        char checkSum[20];
        // int checkSumSize = recv(clientSocket,checkSum,20,0);
        printf("checksum received  ===>  %s\n",checkSum);
        
        int numOfBytes = -1;
        FILE *file = fopen("receiver.txt","w");
        while (numOfBytes != 0)
        {
            // stage 4/7 - recieve the massege from the client.
            bzero(buffer,sizeof(buffer));
            numOfBytes = recv(clientSocket, buffer, 100, 0);
            char* buf = buffer;
            buffer[100]='\0';
            buffer[101]='\0';
            // printf("%c",buffer[100]);
            // printf("buffer ==> %s\n",buffer);
            // break;
            checksum(buffer,100);
            fputs(buffer,file);
            // printf("got => %s",buffer);
        }
        // int checkSumAns = checksum("receiver.txt",0);
        // char checkSumString[20];
        // sprintf(checkSumString,"%d",checkSumAns);
        printf("checksum  calculated  ===>  %d\n",sum);
        gettimeofday(&end, NULL);
        double time_current = (double)((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec) / 1000000;
        time += time_current;
        buffer[numOfBytes] = '\0';
        printf("time took %f\n",time_current);

        sleep(1);
    // stage 10 - close connection.
    close(listeningSocket);
    fclose(file);
}