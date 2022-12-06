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

int main()
{
    char buffer[100];
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
    for (int i = 0; i < 2; i++)
    {
        printf("Waiting for incoming TCP-connections...\n");
        int enableReuse = 1;
        // enable reuse in port.
        if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0)
        {
            printf("setsockopt() failed with error code : %d", errno);
        }
        int j = 0;
        double time = 0.0;
        while (j < 5)
        {
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
            int numOfBytes = -1;
            while (numOfBytes != 0)
            {
                // stage 4/7 - recieve the massege from the client.
                numOfBytes = recv(clientSocket, buffer, 100, 0);
            }
            gettimeofday(&end, NULL);
            double time_current = (double)((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec) / 1000000;
            time += time_current;
            buffer[numOfBytes] = '\0';
            sleep(1);
            j++;
        }
        // stage 5/8 - calculate avarage time for reno and cubic.
        sleep(1);
        if (i == 0)
        {
            printf("Total time taken for cubic: %f seconds\n", time);
            printf("Avarage time taken for cubic: %f seconds\n", time / 5);
        }
        else
        {
            printf("Total time taken for reno: %f seconds\n", time);
            printf("Avarage time taken for reno: %f seconds\n", time / 5);
        }
    }
    // stage 10 - close connection.
    close(listeningSocket);
}