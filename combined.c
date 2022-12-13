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
#include <sys/un.h>
#include <signal.h>
#include <inttypes.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <time.h>

#define SERVER_PORT 5090
#define BUFFER_SIZE 1024

#define SOCK_PATH "echo_sock"
#define IP_PROTOCOL 0
#define IP_ADDRESS "127.0.0.1" // localhost
#define PORT_NO 15050
#define NET_BUF_SIZE 1000
#define sendrecvflag 0
#define nofile "File Not Found!"
static int sum = 0;
void checksum(char *buffer, int n)
{
    for (int i = 0; i < n; i++)
    {
        sum += buffer[i];
    }
}
// function to clear buffer
void clearBuf(char *b)
{
    int i;
    for (i = 0; i < NET_BUF_SIZE; i++)
        b[i] = '\0';
}

int sendFile(FILE *fp, char *buf, int s)
{
    int i, len;
    if (fp == NULL)
    {
        strcpy(buf, nofile);
        len = strlen(nofile);
        buf[len] = EOF;
        return 1;
    }

    char ch, ch2;
    for (i = 0; i < s; i++)
    {
        ch = fgetc(fp);
        buf[i] = ch;

        if (ch == EOF)
            return 1;
    }
    return 0;
}
// from geeksforgeeks https://www.geeksforgeeks.org/c-program-for-file-transfer-using-udp/
// function to receive file
int recvFile(char *buf, int s)
{
    int i;
    char ch;
    int flag = 0;
    for (i = 0; i < s; i++)
    {
        ch = buf[i];
        if (ch == EOF)
        {
            flag = 1;
            break;
        }
    }
    checksum(buf, NET_BUF_SIZE);
    if (flag == 1)
    {
        return 1;
    }
    return 0;
} // from geeksforgeeks https://www.geeksforgeeks.org/c-program-for-file-transfer-using-udp/

int checksumFile(char *filepath, int n)
{
    int sum = 0;
    char buffer[BUFFER_SIZE];
    FILE *f = fopen(filepath, "r");
    if (f == NULL)
    {
        perror("Error opening file");
        return -1;
    }
    while (fgets(buffer, BUFFER_SIZE, f) != NULL)
    {
        int i;
        for (i = 0; buffer[i] != '\0'; i++)
        {
            sum += buffer[i];
        }
    }
    fclose(f);
    return sum;
}

void TCP()
{
    printf("TCP\n");
    pid_t childpid;
    if ((childpid = fork()) == -1)
    {

        perror("fork");
        exit(1);
    }
    if (childpid == 0)
    {
        sleep(1);
        char *fileName = "shauli.txt";
        FILE *file;
        char buf[256];
        socklen_t length;
        int senderSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (senderSocket == -1)
        {
            perror("failed to open socket");
        }
        length = sizeof(buf);
        struct sockaddr_in serverAddress;
        memset(&serverAddress, 0, sizeof(serverAddress));
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(SERVER_PORT); // network order
        inet_pton(AF_INET, "127.0.0.1", &(serverAddress.sin_addr));
        int Connect = connect(senderSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
        if (Connect == -1)
        {
            perror("error");
            return;
        }
        file = fopen(fileName, "r");
        if (file == NULL)
        {
            fprintf(stderr, "Error in opening file");
            return;
        }
        char sendbuffer[100];
        int checkSumAns = checksumFile(fileName, 0);
        char checkSum[20];
        sprintf(checkSum, "%d", checkSumAns);
        send(senderSocket, checkSum, strlen(checkSum), 0);
        printf("checkSum Calculated ===>   %s\n", checkSum);
        int b;
        int sum = 0;
        do
        {
            bzero(sendbuffer, sizeof(sendbuffer));
            b = fread(sendbuffer, 1, sizeof(sendbuffer), file);
            int SendByte = send(senderSocket, sendbuffer, strlen(sendbuffer), 0);
            sum = SendByte + sum;
        } while (!feof(file));
        close(senderSocket);
        fclose(file);
        exit(0);
    }
    else
    {
        char buffer[100];
        // printf("%d\n",checksum("100mb.txt",0));
        //  1. open a new listening socket.
        int listeningSocket = -1;
        listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (listeningSocket == -1)
        {
            perror("Error: Cannot open a new socket\n");
            return;
        }
        //  2. Listening to incoming connections.
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_port = htons(SERVER_PORT);
        serverAddr.sin_family = AF_INET;
        // inet_pton(AF_INET, "127.0.0.1", &(serverAddr.sin_addr));
        struct timeval begin, end;
        if (bind(listeningSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            printf("Bind() has failed with the error code: %d", errno);
            return;
        }
        if (listen(listeningSocket, 500) == -1) // 500 is a Maximum size of queue connection requests
                                                // number of concurrent connections
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
            return;
        }
        // char response[10] = "OK";
        // write(clientSocket, response, sizeof(response));
        gettimeofday(&begin, NULL);
        // receive checksum from sender
        char sum_recieved_string[20];
        bzero(sum_recieved_string, sizeof(sum_recieved_string));
        int checkSumSize = recv(clientSocket, sum_recieved_string, sizeof(sum_recieved_string), 0);
        printf("checksum received  ===>  %s\n", sum_recieved_string);
        int numOfBytes = -1;
        // FILE *file = fopen("receiver.txt", "w");
        while (numOfBytes != 0)
        {
            // stage 4/7 - recieve the massege from the client.
            bzero(buffer, sizeof(buffer));
            numOfBytes = recv(clientSocket, buffer, sizeof(buffer), 0);
            char *buf = buffer;
            // buffer[100] = '\0';
            // buffer[101] = '\0';
            checksum(buffer, 100);
            // fputs(buffer,file);
        }
        printf("checksum  calculated  ===>  %d\n", sum);
        gettimeofday(&end, NULL);
        double time_current = (double)((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec) / 1000000;
        time += time_current;
        buffer[numOfBytes] = '\0';
        printf("time took %f\n", time_current);
        sleep(1);
        // stage 10 - close connection.
        close(listeningSocket);
        // fclose(file);
        sum = 0;
        // exit(0);
    }
}
void UDS_SOCK_STREAM()
{
    printf("\nUDS_SOCK_STREAM\n");
    pid_t childpid;
    if ((childpid = fork()) == -1)
    {

        perror("fork");
        exit(1);
    }
    if (childpid == 0)
    {
        sleep(1);
        int length;
        char *fileName = "shauli.txt";
        FILE *file;
        char buf[256];
        int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sockfd == -1)
        {
            perror("failed to open socket");
        }
        struct sockaddr_un addr;
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, SOCK_PATH);
        length = strlen(addr.sun_path) + sizeof(addr.sun_family);
        int Connect = connect(sockfd, (struct sockaddr *)&addr, length);
        if (Connect == -1)
        {
            perror("error in connect to usd");
            return;
        }

        file = fopen(fileName, "r");
        if (file == NULL)
        {
            fprintf(stderr, "Error in opening file");
            return;
        }
        char getReply[10];
        char sendbuffer[100];
        int checkSumAns = checksumFile(fileName, 0);
        char checkSum[10];
        sprintf(checkSum, "%d", checkSumAns);
        send(sockfd, checkSum, 10, 0);
        printf("checkSum Calculated ===>   %s\n", checkSum);
        int b;
        int sum = 0;
        do
        {
            bzero(sendbuffer, sizeof(sendbuffer));
            b = fread(sendbuffer, 1, sizeof(sendbuffer), file);
            int SendByte = send(sockfd, sendbuffer, sizeof(sendbuffer), 0);
            sum = SendByte + sum;
        } while (!feof(file));

        sleep(1);
        fclose(file);
        close(sockfd);

        sleep(1);
        exit(0);
    }
    else
    {
        struct sockaddr_un sock_in, sock_out;
        char buff[100];
        int listeningSocket = socket(AF_UNIX, SOCK_STREAM, 0);
        if (listeningSocket == -1)
        {
            perror("socket");
        }
        sock_in.sun_family = AF_UNIX;
        strcpy(sock_in.sun_path, SOCK_PATH);
        unlink(sock_in.sun_path);
        int len_of_sock = strlen(sock_in.sun_path) + sizeof(sock_in.sun_family);
        struct timeval begin, end;
        double time = 0.0;
        gettimeofday(&begin, NULL);
        if (bind(listeningSocket, (struct sockaddr *)&sock_in, len_of_sock) == -1)
        {
            perror("error in binding");
            exit(1);
        }
        if (listen(listeningSocket, 10) == -1)
        {
            perror("error in listening");
            exit(1);
        }
        int done, n;
        printf("Waiting for a connection...\n");
        len_of_sock = sizeof(sock_in);
        int clientSocket = accept(listeningSocket, (struct sockaddr *)&sock_in, (socklen_t *)&len_of_sock);
        if (clientSocket == -1)
        {
            perror("error in accepting");
            exit(1);
        }
        printf("Connected.\n");
        char checkSum[10];
        recv(clientSocket, checkSum, 10, 0);
        printf("got checksum ===>>> %s\n", checkSum);
        int numOfBytes = -1;
        while (numOfBytes != 0)
        {
            bzero(buff, sizeof(buff));
            numOfBytes = recv(clientSocket, buff, sizeof(buff), 0);
            checksum(buff, 100);
        }
        printf("checksum ====> %d\n", sum);
        gettimeofday(&end, NULL);
        double time_current = (double)((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec) / 1000000;
        time += time_current;
        printf("time took %f\n", time_current);
        close(clientSocket);
        close(listeningSocket);
        remove("echo_soc");
        remove("echo_sock");
        sum = 0;
    }
}
void UDP()
{
    printf("\nUDP\n");
    pid_t childpid;
    if ((childpid = fork()) == -1)
    {

        perror("fork");
        exit(1);
    }
    if (childpid == 0)
    {
        sleep(1);
        int sockfd, nBytes;
        // struct sockaddr_in6 addr_con;
        // int addrlen = sizeof(addr_con);
        // addr_con.sin6_family = AF_INET6;
        // addr_con.sin6_port = htons(PORT_NO);
        // addr_con.sin6_addr = in6addr_any;
        struct sockaddr_in addr_con;
        int addrlen = sizeof(addr_con);
        addr_con.sin_family = AF_INET;
        addr_con.sin_port = htons(PORT_NO);
        addr_con.sin_addr.s_addr = INADDR_ANY;
        char net_buf[NET_BUF_SIZE];
        FILE *fp;
        char *fileName = "shauli.txt";
        // socket()
        // sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0)
            printf("\nfile descriptor not received!!\n");
        else
            printf("\nfile descriptor %d received\n", sockfd);

        // checksum calculation and sending
        int checkSumAns = checksumFile(fileName, 0);
        char checkSum[20];
        sprintf(checkSum, "%d", checkSumAns);
        printf("checkSum Calculated ===>   %s\n", checkSum);
        int SendByte = sendto(sockfd, checkSum, 20,
                              sendrecvflag, (struct sockaddr *)&addr_con,
                              addrlen);

        fp = fopen(fileName, "r");
        if (fp == NULL)
        {
            fprintf(stderr, "Error in opening file");
            return;
        }
        while (1)
        {
            // process
            if (sendFile(fp, net_buf, NET_BUF_SIZE) == 0)
            {
                sendto(sockfd, net_buf, NET_BUF_SIZE,
                       sendrecvflag,
                       (struct sockaddr *)&addr_con, addrlen);
            }
            else
            {
                sendto(sockfd, net_buf, NET_BUF_SIZE,
                       sendrecvflag,
                       (struct sockaddr *)&addr_con, addrlen);
                break;
            }
            clearBuf(net_buf);
        }
        clearBuf(net_buf);
        for (int i = 0; i < 100; i++)
        {
            sendto(sockfd, "", 0,
                   sendrecvflag,
                   (struct sockaddr *)&addr_con, addrlen);
        }
        close(sockfd);
        fclose(fp);
        printf("\n-------------------------------\n");
        exit(0);
    }
    else
    {
        // https://www.geeksforgeeks.org/c-program-for-file-transfer-using-udp/
        int sockfd, nBytes;
        // struct sockaddr_in6 addr_con;
        // int addrlen = sizeof(addr_con);
        // addr_con.sin6_family = AF_INET6;
        // addr_con.sin6_port = htons(PORT_NO);
        // addr_con.sin6_addr = in6addr_any;
        struct sockaddr_in addr_con;
        int addrlen = sizeof(addr_con);
        addr_con.sin_family = AF_INET;
        addr_con.sin_port = htons(PORT_NO);
        addr_con.sin_addr.s_addr = INADDR_ANY;
        char net_buf[NET_BUF_SIZE];
        struct timeval begin, end;

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        // sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
        double time = 0.0;
        gettimeofday(&begin, NULL);
        if (sockfd < 0)
            printf("\nfile descriptor not received!!\n");
        else
            printf("\nfile descriptor %d received\n", sockfd);
        // bind()
        if (bind(sockfd, (struct sockaddr *)&addr_con, sizeof(addr_con)) == 0)
            printf("\nSuccessfully binded!\n");
        else
            printf("\nBinding Failed!\n");
        // receive checksum from sender
        char sum_recieved_string[20];
        bzero(sum_recieved_string, sizeof(sum_recieved_string));
        int checkSumSize = recvfrom(sockfd, &sum_recieved_string, 20, 0, (struct sockaddr *)&addr_con, (socklen_t *)&addrlen);
        printf("checksum received  ===>  %s\n", sum_recieved_string);
        while (nBytes != 0)
        {
            clearBuf(net_buf);
            nBytes = recvfrom(sockfd, net_buf, NET_BUF_SIZE,
                              sendrecvflag, (struct sockaddr *)&addr_con,
                              (socklen_t *)&addrlen);
            checksum(net_buf, NET_BUF_SIZE);
            // process
        }
        gettimeofday(&end, NULL);
        double time_current = (double)((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec) / 1000000;
        time += time_current;
        char sum_in_string[20];
        printf("checksum calculated  ===>  %d\n", sum);
        sprintf(sum_in_string, "%d", sum + 1);
        // only if the checksum is correct then we print the
        if (strcmp(sum_in_string, sum_recieved_string) == 0)
        {
            printf("time took %f\n", time_current);
        }
        else
        {
            printf("%d", -1);
        }
        sum = 0;
        close(sockfd);
    }
}
void PIPE()
{
    printf("\nPIPE\n");
    int fd[2], bytes_read, b;
    pid_t childpid;
    // char string[] = "Hello, world!\n";
    // char readbuffer[80];
    struct timeval begin, end;
    double time = 0.0;
    gettimeofday(&begin, NULL);
    pipe(fd);
    if ((childpid = fork()) == -1)
    {
        perror("fork");
        exit(1);
    }
    if (childpid == 0)
    {
        // sleep(1);
        char sendbuffer[100];
        char *fileName = "shauli.txt";
        FILE *file;
        file = fopen(fileName, "r");
        if (file == NULL)
        {
            fprintf(stderr, "Error in opening file");
            return;
        }
        /* Child process closes up input side of pipe */
        close(fd[0]);
        printf("checkSum received ===> %d\n", checksumFile(fileName, 0));
        do
        {
            bzero(sendbuffer, sizeof(sendbuffer));
            b = fread(sendbuffer, 1, sizeof(sendbuffer), file);
            write(fd[1], sendbuffer, sizeof(sendbuffer));
            // sum = SendByte + sum;
        } while (!feof(file));
        fclose(file);
        /* Send "string" through the output side of pipe */
        // write(fd[1], string, (strlen(string) + 1));
        exit(0);
    }
    else
    {
        /* Parent process closes up output side of pipe */
        close(fd[1]);
        char readbuffer[100];
        do
        {
            /* Read in a string from the pipe */
            bzero(readbuffer, sizeof(readbuffer));
            bytes_read = read(fd[0], readbuffer, sizeof(readbuffer));
            checksum(readbuffer, sizeof(readbuffer));
            // printf("Received string: %s", readbuffer);
        } while (bytes_read != 0);
        gettimeofday(&end, NULL);
        double time_current = (double)((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec) / 1000000;
        time += time_current;
        printf("CheckSum calculated ===>  %d\n", sum);
        printf("time took %f\n", time_current);
        sum=0;
    }
}
int main()
{
    // TCP();
    // UDS_SOCK_STREAM();
    UDP();
    // PIPE();
    return 0;
}