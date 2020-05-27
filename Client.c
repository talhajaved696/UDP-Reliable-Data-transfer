#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <libgen.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "transfer.h"

// Client Side for Reliable Video File Transmission over UDP. 

// Client Side Functions

void Packets(FILE *f, char window[No_of_Buffer][PACKET_SIZE], int *seqID, bool *File_end, int *data_len);
void sending_packet(int sockf, struct sockaddr_in server, int size, char window[No_of_Buffer][PACKET_SIZE], int seqID, bool File_end, int *data_len);
bool lost_ack(bool *ack, int seqID);
void resetting_data(int *seqID, bool *ack);

// Main function

int main(int argc, char* argv[]){

    // Check for no of args on command line.
    if(argc != 3) {
        perror("Usage => <ClientFile> <Filepath> <Port>");
        exit(1);
    }

    int PORT = atoi(argv[2]); // PORT given on command line.
    int sockf;
    struct sockaddr_in server;
    // Creating Socket
    sockf = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockf < 0){
        perror("Can't allocate sockf !\n");
        exit(1);
    }else
	    printf("Socket Created.\n");

    char dataBuff[PACKET_SIZE]; // 500
    char window[No_of_Buffer][PACKET_SIZE];
    bool ack[WINDOW_SIZE];
    int data_len[WINDOW_SIZE];// 5
    bool File_end = false;
    int seqID = 0;
    int packet_num = 0;
    int size;

    // Intiating with null values.
    memset(&server, 0, sizeof(server));
    memset(&ack, 0, 5);
    memset(&dataBuff, 0, sizeof(dataBuff));

    // Address Allocation
    server.sin_family = AF_INET; //Internet Protocol v4 addresses
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    size = sizeof(server);

    // Get filename
    char *f_name = basename(argv[1]);
    if (f_name == NULL)
    {
        perror("Unable to get filename");
        exit(1);
    }
    // Send filename to server.
    strncpy(dataBuff, f_name, strlen(f_name));
    if (sendto(sockf, dataBuff, PACKET_SIZE, 0, (struct sockaddr *) &server, sizeof(server)) == -1)
    {
        perror("Unable to send filename");
        exit(1);
    }

    // Opening file to read.
    FILE *f = fopen(argv[1], "rb");
    if (f == NULL)
    {
        perror("Unable to open file.");
        exit(1);
    }
    while(1){

        // Preparing packets(500 bytes) to send.
        Packets(f, window, &seqID, &File_end, data_len);
       
        // Sending number of packets(5).
        char packet_num[5];
        memset(&packet_num, 0, sizeof(packet_num));
        packet_num[0] = seqID + '0';
        sendto(sockf, packet_num, sizeof(packet_num), 0, (struct sockaddr *)&server, size);
        
        // Start sending packets.
        sending_packet(sockf, server, size, window, seqID, File_end, data_len);
        
        // Accepting Acknowledgments from server.
        printf("Client: Accepting Acks\n");
        recvfrom(sockf, ack, 5, 0, (struct sockaddr *)&server, &size);  

        // Stop N Wait for Acknowledgments.
        while(lost_ack(ack, seqID)){
            // Resending packets.
            printf("Client: Sending lost packets\n");
            for(int x=0; x<seqID; x++){
            if(!ack[x]){
            sendto(sockf, window[x], PACKET_SIZE, 0, (struct sockaddr *)&server, size);
            }
            }  
            // Accepting Acks for lost packets. 
            printf("Client: Accepting Acks\n");
            recvfrom(sockf, ack, 5, 0, (struct sockaddr *)&server, &size); 
        }
        // Reset ack data.
        resetting_data(&seqID, ack);

        if(File_end){
            break;
        }
    }
    printf("\nFile sent successfully.\n");
    memset(&server, 0, sizeof(server));
    return 0;

}


void Packets(FILE *f,char window[No_of_Buffer][PACKET_SIZE], int *seqID, bool *File_end, int *data_len){

    // Reset Window
    for(int x=0; x<6; x++){
        memset(&window[x], 0, sizeof(window[x]));
    } 
    
    for(int x=0; x<5; x++){
        window[x][0] = *seqID + '0';
        strcat(window[x], " ");
        data_len[x] = fread(window[x]+2, sizeof(char), PACKET_SIZE-2, f);
        *seqID += 1;

        if(feof(f)){
            *File_end = true;
            strcpy(window[5],"file end");
            break;
        }    
    }

}

void resetting_data(int *seqID, bool *ack){
    *seqID = 0;
    memset(&ack, 0, 5);
}

void sending_packet(int sockf, struct sockaddr_in server, int size, char window[No_of_Buffer][PACKET_SIZE], int seqID, bool File_end, int *data_len){

    char finish[10];
    memset(&finish, 0, sizeof(finish));
    strcpy(finish, "finish");
    printf("Client: Sending Packets\n");
    for(int x=0; x<seqID; x++){
        
        sendto(sockf, window[x], data_len[x]+2, 0, (struct sockaddr *)&server, size); 
    }

    if(File_end){
        sendto(sockf, window[5], sizeof(window[5]), 0, (struct sockaddr *)&server, size);
    }else{
        sendto(sockf, finish, sizeof(finish), 0, (struct sockaddr *)&server, size);
    
    }

}

bool lost_ack(bool *ack, int seqID){

    printf("Client: Checking for lost Acks\n");
    for(int x=0; x<seqID; x++){
        if(!ack[x]){
            return true;
        }
    }
    return false;

}



