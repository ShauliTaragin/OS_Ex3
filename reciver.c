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
    for (int i = 0; i<n; i++) {
        sum += buffer[i];
    }

}

void TCP(){
    char buffer[100];
    //  open a new listening socket.
    int listeningSocket = -1;
    listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listeningSocket == -1)
    {
        perror("Error: Cannot open a new socket\n");
        return ;
    }
    //  Listening to incoming connections.
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_family = AF_INET;
    struct timeval begin, end;
    if (bind(listeningSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        printf("Bind() has failed with the error code: %d", errno);
        return ;
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
        // accept the socket from client.
        int clientSocket = accept(listeningSocket, (struct sockaddr *)&clientAddress, &clientAddressLen);
        if (clientSocket == -1)
        {
            printf("listen has failed\n");
            close(listeningSocket);
            return ;
        }
        char response[10] = "OK";
        write(clientSocket, response, sizeof(response));
        gettimeofday(&begin, NULL);
        

        //receive checksum from sender
        char sum_recieved_string[20];
        bzero(sum_recieved_string,sizeof(sum_recieved_string));
        int checkSumSize = recv(clientSocket,sum_recieved_string,20,0);
        printf("checksum received  ===>  %s\n",sum_recieved_string);
        
        

        int numOfBytes = -1;
        FILE *file = fopen("receiver.txt","w");
        while (numOfBytes != 0)
        {
            // recieve the message from the client.
            bzero(buffer,sizeof(buffer));
            numOfBytes = recv(clientSocket, buffer, 100, 0);
            char* buf = buffer;
            buffer[100]='\0';
            buffer[101]='\0';
            checksum(buffer,100);
            fputs(buffer,file);
        }
        printf("checksum  calculated  ===>  %d\n",sum);
        gettimeofday(&end, NULL);
        double time_current = (double)((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec) / 1000000;
        time += time_current;
        buffer[numOfBytes] = '\0';
        char sum_in_string [20];
        sprintf(sum_in_string,"%d",sum);
        //only if the checksum is correct then we print the 
        if(strcmp(sum_in_string,sum_recieved_string)==0){
            printf("time took %f\n",time_current);
        }
        else{
            printf("%d" , -1);
        }
        sleep(1);
    // close connection.
    close(listeningSocket);
    fclose(file);
}
int main(int argc, char **argv)
{
    if(strcmp(argv[1],"a")==0){//case for tcp
        TCP();
    }
    return 1;
}