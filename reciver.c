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




#define SERVER_PORT 5090
#define BUFFER_SIZE 1024

#define SOCK_PATH "echo_sock"
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

// int main(void)
// {
//     int s, s2, t, len;
//     struct sockaddr_un local, remote;
//     char str[100];
//     if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
//         perror("socket");
//         exit(1);
//     }
//     local.sun_family = AF_UNIX;
//     strcpy(local.sun_path, SOCK_PATH);
//     unlink(local.sun_path);
//     len = strlen(local.sun_path) + sizeof(local.sun_family);
//     if (bind(s, (struct sockaddr *)&local, len) == -1) {
//         perror("bind");
//         exit(1);
//     }
//     if (listen(s, 5) == -1) {
//         perror("listen");
//         exit(1);
//     }
//     for(;;) {
//         int done, n;
//         printf("Waiting for a connection...\n");
//         t = sizeof(remote);
//         if ((s2 = accept(s, (struct sockaddr *)&remote, &t)) == -1) {
//             perror("accept");
//             exit(1);
//         }
//         printf("Connected.\n");
//         done = 0;
//         do {
//             n = recv(s2, str, 100, 0);
//             if (n <= 0) {
//                 if (n < 0) perror("recv");
//                 done = 1;
//             }
//             if (!done) 
//                 if (send(s2, str, n, 0) < 0) {
//                     perror("send");
//                     done = 1;
//                 }
//         } while (!done);
//         close(s2);
//     }
//     return 0;
// }


void UDS_Stream_Socket(){
    int  s2, t, len;
    struct sockaddr_un sock_in,sock_out;
    char buff[100];
    int listeningSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listeningSocket == -1) {
        perror("socket");
    }
    sock_in.sun_family = AF_UNIX;
    strcpy(sock_in.sun_path, SOCK_PATH);
    unlink(sock_in.sun_path);
    len = strlen(sock_in.sun_path) + sizeof(sock_in.sun_family);
    if (bind(listeningSocket, (struct sockaddr *)&sock_in, len) == -1) {
        perror("error in binding");
        exit(1);
    }
    if (listen(listeningSocket, 10) == -1) {
        perror("error in listening");
        exit(1);
    }
     int done, n;
        printf("Waiting for a connection...\n");
        len = sizeof(sock_in);
        int clientSocket= accept(listeningSocket, (struct sockaddr *)&sock_in,&len);
        if (clientSocket == -1) {
            perror("error in accepting");
            exit(1);
        }
        printf("Connected.\n");
    char checkSum[10];
    recv(clientSocket, checkSum, 10, 0);
    printf("got checksum ===>>> %s\n",checkSum);
    int numOfBytes = -1;
    while (numOfBytes!=0){
        bzero(buff,sizeof(buff));
        numOfBytes = recv(clientSocket, buff, sizeof(buff), 0);
        checksum(buff,100);
        // printf("%d",numOfBytes);
    }
    printf("checksum ====> %d", sum);
    close(clientSocket);
    close(listeningSocket);
}

void TCP(){
    char buffer[100];
    // printf("%d\n",checksum("100mb.txt",0));
    //  1. open a new listening socket.
    int listeningSocket = -1;
    listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listeningSocket == -1)
    {
        perror("Error: Cannot open a new socket\n");
        return ;
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
        // stage 3 - accept the socket from client.
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
    sum=0;
}


//from geeksforgeeks https://www.geeksforgeeks.org/c-program-for-file-transfer-using-udp/

#define IP_PROTOCOL 0
#define PORT_NO 15050
#define NET_BUF_SIZE 32
#define sendrecvflag 0
#define nofile "File Not Found!"
  
// function to clear buffer
void clearBuf(char* b)
{
    int i;
    for (i = 0; i < NET_BUF_SIZE; i++)
        b[i] = '\0';
}
  

// function to receive file
int recvFile(char* buf, int s)
{
    int i;
    char ch;
    int flag =0 ;
    for (i = 0; i < s; i++) {
        ch = buf[i];
        if (ch == EOF){
            flag=1;
            break;
        }
    }
    checksum(buf, NET_BUF_SIZE);
    if(flag==1){
        return 1;
    }
    return 0;
}

void UDP(){
    //https://www.geeksforgeeks.org/c-program-for-file-transfer-using-udp/

    int sockfd, nBytes;
    struct sockaddr_in addr_con;
    int addrlen = sizeof(addr_con);
    addr_con.sin_family = AF_INET;
    addr_con.sin_port = htons(PORT_NO);
    addr_con.sin_addr.s_addr = INADDR_ANY;
    char net_buf[NET_BUF_SIZE];
    struct timeval begin, end;
    // socket()
    sockfd = socket(AF_INET, SOCK_DGRAM, IP_PROTOCOL);
    double time = 0.0;
    gettimeofday(&begin, NULL);
    if (sockfd < 0)
        printf("\nfile descriptor not received!!\n");
    else
        printf("\nfile descriptor %d received\n", sockfd);

    // bind()
    if (bind(sockfd, (struct sockaddr*)&addr_con, sizeof(addr_con)) == 0)
        printf("\nSuccessfully binded!\n");
    else
        printf("\nBinding Failed!\n");
    
    //receive checksum from sender
    char sum_recieved_string[20];
    bzero(sum_recieved_string,sizeof(sum_recieved_string));
    int checkSumSize = recvfrom(sockfd,sum_recieved_string,20,0,(struct sockaddr*)&addr_con, &addrlen);
    printf("checksum received  ===>  %s\n",sum_recieved_string);



    while (nBytes!=0) {


        clearBuf(net_buf);
        nBytes = recvfrom(sockfd, net_buf, NET_BUF_SIZE,
                            sendrecvflag, (struct sockaddr*)&addr_con,
                            &addrlen);
        checksum(net_buf,NET_BUF_SIZE);
        // process
    }
    
    

    gettimeofday(&end, NULL);
    double time_current = (double)((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec) / 1000000;
    time += time_current;
    char sum_in_string [20];
    printf("checksum calculated  ===>  %d\n",sum);
    sprintf(sum_in_string,"%d",sum+1);
    //only if the checksum is correct then we print the 
    if(strcmp(sum_in_string,sum_recieved_string)==0){
        printf("time took %f\n",time_current);
    }
    else{
        printf("%d" , -1);
    }
    sum=0;
    close(sockfd);



}







int main(int argc, char **argv)
{
    if(strcmp(argv[1],"a")==0){//case for tcp
        TCP();
    }
    if(strcmp(argv[1],"c")==0){//case for tcp
        UDP();
    }
    if(strcmp(argv[1],"b")==0){//case for tcp
        UDS_Stream_Socket();
    }
    return 1;
}