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
#include <inttypes.h>
#include <netdb.h>




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
    return 1;
}






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
#include <inttypes.h>
#include <netdb.h>
#include <arpa/inet.h>

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
    fclose(f);
    return sum;
}
void TCP(){
    //prepare file to sent to receiver
    char *fileName = "shauli.txt";
    FILE *file;
    char buf[256];
    socklen_t length;

    //  open tcp socket.
    int senderSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (senderSocket == -1)
    {
        perror("failed to open socket");
    }
    length = sizeof(buf);
    //  create connection with measure.
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
    //  send file.
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
    printf("checkSum Calculated ===>   %s\n",checkSum);
    int SendByte = send(senderSocket,checkSum,20,0);
    int b;
    int sum = 0;
    do
    {
        bzero(sendbuffer,sizeof(sendbuffer));
        b = fread(sendbuffer, 1, sizeof(sendbuffer), file);
        int SendByte = send(senderSocket, sendbuffer, b, 0);
        sum = SendByte + sum;
    } while (!feof(file));
   
    sleep(1);
    //Close connection.
    close(senderSocket);
    fclose(file);
    sleep(1);
}

//from geeksforgeeks https://www.geeksforgeeks.org/c-program-for-file-transfer-using-udp/

#define IP_PROTOCOL 0
#define IP_ADDRESS "127.0.0.1" // localhost
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
  

int sendFile(FILE* fp, char* buf, int s)
{   
    int i, len;
    if (fp == NULL) {
        strcpy(buf, nofile);
        len = strlen(nofile);
        buf[len] = EOF;
        return 1;
    }
  
    char ch, ch2;
    for (i = 0; i < s; i++) {
        ch = fgetc(fp);
        buf[i] = ch;
        
        if (ch == EOF)
            return 1;
    }
    return 0;
}




void UDP(){
    int sockfd, nBytes;
    struct sockaddr_in addr_con;
    int addrlen = sizeof(addr_con);
    addr_con.sin_family = AF_INET;
    addr_con.sin_port = htons(PORT_NO);
    addr_con.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    char net_buf[NET_BUF_SIZE];
    FILE* fp ;
    char *fileName = "shauli.txt";
    // socket()
    sockfd = socket(AF_INET, SOCK_DGRAM,
                    IP_PROTOCOL);   

    if (sockfd < 0)
        printf("\nfile descriptor not received!!\n");
    else
        printf("\nfile descriptor %d received\n", sockfd);
    
    //checksum calculation and sending
    int checkSumAns = checksum(fileName,0);
    char checkSum[20];
    sprintf(checkSum,"%d",checkSumAns);
    printf("checkSum Calculated ===>   %s\n",checkSum);
    int SendByte =         sendto(sockfd, checkSum, 20,
               sendrecvflag, (struct sockaddr*)&addr_con,
               addrlen);

    fp = fopen(fileName, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error in opening file");
        return;
    }
    while (1) {
        // process
        if (sendFile(fp, net_buf, NET_BUF_SIZE)==0) {
            sendto(sockfd, net_buf, NET_BUF_SIZE,
                sendrecvflag, 
                (struct sockaddr*)&addr_con, addrlen);
        }
        else{
            sendto(sockfd, net_buf, NET_BUF_SIZE,
                sendrecvflag, 
                (struct sockaddr*)&addr_con, addrlen);
            break;
        }
        clearBuf(net_buf);
    }
    clearBuf(net_buf);
    sendto(sockfd, "", 0,
    sendrecvflag, 
    (struct sockaddr*)&addr_con, addrlen);
    close(sockfd);
    fclose(fp);
    printf("\n-------------------------------\n");
}





int main(int argc, char **argv)
{
    if(strcmp(argv[1],"a")==0){//case for tcp
        TCP();
    }
    if(strcmp(argv[1],"c")==0){//case for tcp
        UDP();
    }
    return 1;
}


