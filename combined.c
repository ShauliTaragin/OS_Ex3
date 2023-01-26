#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <strings.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <inttypes.h>
#include <netdb.h>
#include <ctype.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h> /* mmap() is defined in this header */
#include <pthread.h>
#include <semaphore.h>

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
ssize_t bytes = 0;
char *bufffer;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
char *filenameof100mb = "100mb.txt";
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

    char ch;
    for (i = 0; i < s; i++)
    {
        ch = fgetc(fp);
        buf[i] = ch;

        if (ch == EOF)
            return 1;
    }
    return 0;
}

int recvFile(char *buf, int s)
{
    // function to receive file
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
}

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

void Tcp()
{
    struct timeval begin, end;
    gettimeofday(&begin, NULL);
    printf("\nTCP Start: -> %f\n", ((double)clock()) / CLOCKS_PER_SEC);
    pid_t childpid;
    if ((childpid = fork()) == -1)
    {

        perror("fork");
        exit(1);
    }
    if (childpid == 0)
    {
        sleep(1);
        char *fileName = "100mb.txt";
        FILE *file;
        char buf[1024];
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
        char sendbuffer[1024];
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
        char buffer[1024];
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

        int numOfBytes = -1;
        while (numOfBytes != 0)
        {
            // stage 4/7 - recieve the massege from the client.
            bzero(buffer, 1024);
            numOfBytes = recv(clientSocket, buffer, sizeof(buffer), 0);
            checksum(buffer, 1024);
        }
        gettimeofday(&end, NULL);
        double time_current = (double)((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec) / 1000000;
        time += time_current;
        buffer[numOfBytes] = '\0';
        // only if checksum is equal then print
        printf("TCP End: -> %f\n", ((double)clock()) / CLOCKS_PER_SEC);

        if (sum == checksumFile("100mb.txt", 0))
        {
            printf("Time took %f seconds\n", time);
        }
        else
        {
            printf("Differrent: %d\n", -1);
        }
        sleep(1);
        // stage 10 - close connection.
        close(listeningSocket);
        sum = 0;
    }
}
void Uds_Sock_Stream()
{
    printf("\nUDS_SOCK_STRAM Start: -> %f\n", ((double)clock()) / CLOCKS_PER_SEC);
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
        char *fileName = "100mb.txt";
        FILE *file;
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
        char sendbuffer[BUFFER_SIZE];
        int checkSumAns = checksumFile(fileName, 0);
        char checkSum[10];
        sprintf(checkSum, "%d", checkSumAns);
        send(sockfd, checkSum, 10, 0);
        // printf("checkSum Calculated ===>   %s\n", checkSum);
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
        struct sockaddr_un sock_in;
        char buff[BUFFER_SIZE];
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
        len_of_sock = sizeof(sock_in);
        int clientSocket = accept(listeningSocket, (struct sockaddr *)&sock_in, (socklen_t *)&len_of_sock);
        if (clientSocket == -1)
        {
            perror("error in accepting");
            exit(1);
        }
        char checkSum[10];
        recv(clientSocket, checkSum, 10, 0);
        // printf("got checksum ===> %s\n", checkSum);
        int numOfBytes = -1;
        while (numOfBytes != 0)
        {
            bzero(buff, sizeof(buff));
            numOfBytes = recv(clientSocket, buff, sizeof(buff), 0);
            checksum(buff, BUFFER_SIZE);
        }
        // printf("checksum ====> %d\n", sum);
        gettimeofday(&end, NULL);
        double time_current = (double)((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec) / 1000000;
        time += time_current;
        // only if checksum is equal then print
        printf("UDS_SOCK_STRAM End: -> %f\n", ((double)clock()) / CLOCKS_PER_SEC);
        if (sum == checksumFile("100mb.txt", 0))
        {
            printf("Time took %f seconds\n", time_current);
        }
        else
        {
            printf("Different: %d\n", -1);
        }
        close(clientSocket);
        close(listeningSocket);
        remove("echo_soc");
        remove("echo_sock");
        sum = 0;
    }
}
void Udp()
{
    // from geeksforgeeks https://www.geeksforgeeks.org/c-program-for-file-transfer-using-udp/
    printf("\nUDP Start: -> %f\n", ((double)clock()) / CLOCKS_PER_SEC);
    pid_t childpid;
    if ((childpid = fork()) == -1)
    {

        perror("fork");
        exit(1);
    }
    if (childpid == 0)
    {
        sleep(1);
        int sockfd;
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
        char *fileName = "100mb.txt";
        // socket()
        // sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0)
            printf("\nfile descriptor not received!!\n");
        // checksum calculation and sending
        int checkSumAns = checksumFile(fileName, 0);
        char checkSum[20];
        sprintf(checkSum, "%d", checkSumAns);
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
        // bind()
        if (bind(sockfd, (struct sockaddr *)&addr_con, sizeof(addr_con)) == 0)
        {
            // printf("\nSuccessfully binded!\n");
        }
        else
            printf("\nBinding Failed!\n");
        // receive checksum from sender
        char sum_recieved_string[20];
        bzero(sum_recieved_string, sizeof(sum_recieved_string));
        int checkSumSize = recvfrom(sockfd, &sum_recieved_string, 20, 0, (struct sockaddr *)&addr_con, (socklen_t *)&addrlen);
        // printf("checksum received  ===>  %s\n", sum_recieved_string);
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
        sprintf(sum_in_string, "%d", sum + 1);
        printf("UDP End: -> %f\n", ((double)clock()) / CLOCKS_PER_SEC);
        // only if the checksum is correct then we print the
        if (strcmp(sum_in_string, sum_recieved_string) == 0)
        {
            printf("Time took %f seconds\n", time_current);
        }
        else
        {
            printf("Different: %d\n", -1);
        }
        sum = 0;
        close(sockfd);
    }
}
void Pipe()
{
    // Used : https://www.geeksforgeeks.org/c-program-demonstrate-fork-and-pipe/
    printf("\nPipe Start: -> %f\n", ((double)clock()) / CLOCKS_PER_SEC);
    int fd[2], bytes_read, b;
    pid_t childpid;
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
        sleep(1);
        char sendbuffer[1000];
        char *fileName = "100mb.txt";
        FILE *file;
        file = fopen(fileName, "r");
        if (file == NULL)
        {
            fprintf(stderr, "Error in opening file");
            return;
        }
        /* Child process closes up input side of pipe */
        close(fd[0]);
        do
        {
            bzero(sendbuffer, sizeof(sendbuffer));
            b = fread(sendbuffer, 1, sizeof(sendbuffer), file);
            write(fd[1], sendbuffer, sizeof(sendbuffer));
        } while (!feof(file));
        fclose(file);
        exit(0);
    }
    else
    {
        /* Parent process closes up output side of pipe */
        close(fd[1]);
        char readbuffer[1000];
        do
        {
            /* Read in a string from the pipe */
            bzero(readbuffer, sizeof(readbuffer));
            bytes_read = read(fd[0], readbuffer, sizeof(readbuffer));
            checksum(readbuffer, sizeof(readbuffer));
        } while (bytes_read != 0);
        gettimeofday(&end, NULL);
        double time_current = (double)((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec) / 1000000;
        time += time_current;
        printf("Pipe End: -> %f\n", ((double)clock()) / CLOCKS_PER_SEC);
        if (sum == checksumFile("100mb.txt", 0))
        {
            printf("Time took %f seconds\n", time_current);
        }
        else
        {
            printf("Different: %d\n", -1);
        }
        sum = 0;
    }
}

void UDS_Dgram_Socket()
{
    printf("\nUDS_Dgram_Socket Start: -> %f\n", ((double)clock()) / CLOCKS_PER_SEC);
    pid_t childpid;
    struct timeval begin, end;
    double time = 0.0;
    gettimeofday(&begin, NULL);
    if ((childpid = fork()) == -1)
    {
        perror("fork");
        exit(1);
    }
    if (childpid == 0)
    {
        sleep(1);
        const char *socket_path = "./socket";
        const int buffer_size = 1024;
        char buffer[buffer_size];
        struct sockaddr_un addr;
        int fd, cl, rc;
        char *filename = "100mb.txt";

        if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        {
            perror("socket error");
            exit(-1);
        }

        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        {
            perror("connect error");
            exit(-1);
        }

        // checksum calculation and sending
        int checkSumAns = checksumFile(filename, 0);
        char checkSum[20];
        sprintf(checkSum, "%d", checkSumAns);
        // printf("checkSum Calculated ===>   %s\n", checkSum);
        FILE *file = fopen(filename, "r");
        if (file == NULL)
        {
            perror("error opening file");
            exit(-1);
        }

        while ((cl = fread(buffer, 1, buffer_size, file)) > 0)
        {
            rc = write(fd, buffer, cl);
            if (rc < 0)
            {
                perror("write error");
                exit(-1);
            }
        }

        fclose(file);
        close(fd);
        exit(0);
    }
    else
    {
        const char *socket_path = "./socket";
        const int buffer_size = 1024;
        char buffer[buffer_size];
        struct sockaddr_un addr;
        int fd, cl, rc;

        if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        {
            perror("socket error");
            exit(-1);
        }

        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

        unlink(socket_path);
        if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        {
            perror("bind error");
            exit(-1);
        }

        if (listen(fd, 5) == -1)
        {
            perror("listen error");
            exit(-1);
        }

        if ((cl = accept(fd, NULL, NULL)) == -1)
        {
            perror("accept error");
        }
        while ((rc = read(cl, buffer, buffer_size)) > 0)
        {
            checksum(buffer, rc);
        }
        char sum_in_string[20];
        // printf("checksum calculated receiver  ===>  %d\n", sum);
        sprintf(sum_in_string, "%d", sum);
        gettimeofday(&end, NULL);
        double time_current = (double)((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec) / 1000000;
        time += time_current;
        int checkSumAns = checksumFile("100mb.txt", 0);
        char checkSum[20];
        sprintf(checkSum, "%d", checkSumAns);
        printf("UDS_Dgram_Socket End: -> %f\n", ((double)clock()) / CLOCKS_PER_SEC);
        // only if the checksum is correct then we print the time
        if (strcmp(sum_in_string, checkSum) == 0)
        {
            printf("Time took %f seconds\n", time_current);
        }
        else
        {
            printf("Different: %d\n", -1);
        }
        sum = 0;
        close(cl);
        remove("socket");
    }
}
void Mmap()
{
    // used : https://stackoverflow.com/questions/26259421/use-mmap-in-c-to-write-into-memory
    printf("\nMMAP Start: -> %f\n", ((double)clock()) / CLOCKS_PER_SEC);
    struct timeval begin, end;
    double time = 0.0;
    gettimeofday(&begin, NULL);
    int check_sum_to_check = checksumFile("100mb.txt", 0);
    struct stat statbuf;
    int fd;
    char *map; /* mmapped array of int's */
    fd = open("100mb.txt", O_RDWR);
    if (fd == -1)
    {
        perror("Error opening file for writing");
        exit(EXIT_FAILURE);
    }
    if (fstat(fd, &statbuf) < 0)
    {
        printf("fstat error");
        return;
    }
    map = mmap(0, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED)
    {
        close(fd);
        perror("Error mmapping the file");
        exit(EXIT_FAILURE);
    }

    pid_t childpid;
    if ((childpid = fork()) == -1)
    {
        perror("fork");
        exit(1);
    }
    if (childpid == 0)
    {
        char buffer[100];
        int i = 0;
        while (i <= statbuf.st_size)
        {
            bzero(buffer, sizeof(buffer));
            for (int j = 0; j < 99; j++)
            {
                if (statbuf.st_size < i)
                {
                    break;
                }
                buffer[j] = map[i++];
            }
            checksum(buffer, sizeof(buffer));
        }
        gettimeofday(&end, NULL);
        double time_current = (double)((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec) / 1000000;
        time += time_current;
        printf("MMAP End: -> %f\n", ((double)clock()) / CLOCKS_PER_SEC);
        if (sum == check_sum_to_check)
        {
            printf("Time took %f seconds\n", time_current);
        }
        else
        {
            printf("Different: %d\n", -1);
        }
        sum = 0;
        exit(0);
    }
    else
    {
        waitpid(childpid, NULL, 0);
    }
    /* Write to the file int-by-int from the mmap*/
    if (munmap(map, statbuf.st_size) == -1)
    {
        perror("Error un-mmapping the file");
    }
    close(fd);
}

// helper thread function reciever
void *thread_function2(void *arg)
{

    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&cond, &mutex);
    FILE *file;
    file = fopen("received.txt", "w");
    if (file == NULL)
    {
        perror("couldnt open file\n");
    }

    fwrite(bufffer, sizeof(char), bytes, file);
    (&mutex);

    free(bufffer);
    fclose(file);
    int checsum = checksumFile("received.txt", 0);
}
// helper thread function sender
void *thread_function1(void *arg)
{
    struct stat stust;
    FILE *file;
    file = fopen(filenameof100mb, "r");
    if (file == NULL)
    {
        perror("Couldnt open file\n");
    }
    pthread_mutex_lock(&mutex);

    stat(filenameof100mb, &stust);
    ssize_t size_of_stust = stust.st_size;
    int checsum = checksumFile(filenameof100mb, 0);

    bufffer = (char *)calloc(size_of_stust * sizeof(char), sizeof(char));
    bytes = fread(bufffer, sizeof(char), size_of_stust, file);
    checksum(bufffer, sizeof(bufffer));
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    fclose(file);
}

void Shared_Memory_Between_Threads()
{
    // https://stackoverflow.com/questions/40181096/c-linux-pthreads-sending-data-from-one-thread-to-another-using-shared-memory-gi
    // https://www.geeksforgeeks.org/producer-consumer-problem-in-c/
    struct timeval begin, end;

    // sleep a second to catch up with proccesses
    sleep(1);

    printf("\nShared_Memory_Between_Threads Start: -> %f\n", ((double)clock()) / CLOCKS_PER_SEC);

    pthread_t thread1, thread2;
    double time = 0.0;
    gettimeofday(&begin, NULL);
    pthread_create(&thread1, NULL, thread_function1, NULL);
    pthread_create(&thread2, NULL, thread_function2, NULL);
    // threads are locked until both finish
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    gettimeofday(&end, NULL);
    double time_current = (double)((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec) / 1000000;
    time += time_current;
    printf("Shared_Memory_Between_Threads End: -> %f\n", ((double)clock()) / CLOCKS_PER_SEC);
    if (checksumFile(filenameof100mb, 0) == checksumFile("received.txt", 0))
    {
        printf("Time took %f seconds\n", time_current);
    }
    else
    {
        printf("Different: %d\n", -1);
    }
    sum = 0;
}

int main()
{
    Udp();
    Tcp();
    Uds_Sock_Stream();
    UDS_Dgram_Socket();
    Pipe();
    Mmap();
    Shared_Memory_Between_Threads();
    return 0;
}