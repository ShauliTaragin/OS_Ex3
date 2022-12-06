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


int main(int argc, char **argv)
{
    char *fileName = "200kb.txt";
    FILE *file;
    char buf[256];
    socklen_t length;

    for (int i = 0; i < 2; i++)
    {
        if (i == 0)
        {
            strcpy(buf, "cubic");
            length = sizeof(buf);
        }
        else
        {
            // stage 4 - switch algorithm to reno.
            printf("using reno\n");
            strcpy(buf, "reno");
            length = sizeof(buf);
        }
        int j = 0;
        while (j < 5)
        {
            // stage 1 - open tcp socket.
            int senderSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (senderSocket == -1)
            {
                perror("failed to open socket");
                return -1;
            }
            length = sizeof(buf);
            int setSocket = setsockopt(senderSocket, IPPROTO_TCP, TCP_CONGESTION, buf, length);
            if (setSocket == -1)
            {
                perror("error in setsocket");
                return -1;
            }
            int getSocket = getsockopt(senderSocket, IPPROTO_TCP, TCP_CONGESTION, buf, &length);
            if (getSocket == -1)
            {
                perror("error in getsocket");
                return -1;
            }
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
            j++;
        }
        sleep(1);
    }
    return 0;
}