#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "transfer.h"

// Server Side for Reliable Video File Transmission over UDP. 

// Server Side Functions

int packet_no_recv(int sockf, struct sockaddr_in client, int size);
void packet_recv(int sockf, struct sockaddr_in client, int size, int packet_num,char window[6][PACKET_SIZE], bool *transmission_end, bool *Ack, int *len);
int lost_packet(int packet_num, bool *Ack); 
int f_lost_packet(int *lost_packs, bool *Ack, int packet_num);


// Main function
int main(int argc, char *argv[])
{   
    // Check for no of args on command line.
    if(argc != 2){
        perror("Usage => <Server> <PORT>\n");
        exit(1);
    }
    // PORT given on command line.
    int PORT = atoi(argv[1]); 
    int sockf;
    struct sockaddr_in server, client;
    // Creating Socket
    sockf = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockf < 0){
        perror("Can't allocate sockf !\n");
        exit(1);
    }else
	printf("Socket Created.\n");

    char window[6][PACKET_SIZE];
    char f_name[PACKET_SIZE];
    char new_name[PACKET_SIZE];
    bool Ack[WINDOW_SIZE];
    int lost_packs[WINDOW_SIZE];
    bool transmission_end = false;
    int len[WINDOW_SIZE + 1];
    int packet_num = 0;

    // Intiating with null values.
    memset(&client, 0, sizeof(client));
    memset(&server, 0, sizeof(server));

    // Address Allocation
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Binding PORT to a unique address.
    if(bind(sockf, (struct sockaddr *)&server, sizeof(server))){ // 
        perror("Bind Error\n");
        exit(1);
    }else
        printf("Socket binded successfully.\n");

    // Recieve filename
    strcpy(f_name,"NEW_");
    int size = sizeof(client);
    if (recvfrom(sockf, new_name, PACKET_SIZE, 0, (struct sockaddr *) &client, &size) == -1)
    {
        perror("Unable to filename");
        exit(1);
    }
    // Changing filename to a new name.
    strcat(f_name, new_name);

    // Opening file to write.
    FILE *f = fopen(f_name, "wb");
    if (f == NULL)
    {
        perror("Unable to open file.");
        exit(1);
    }

    while(1){
        // Recieving packet number.
        packet_num = packet_no_recv(sockf, client, size);
        
        // Recieving packets.
        packet_recv(sockf, client, size, packet_num, window, &transmission_end, Ack, len);
        
        // Sending Acknowledgement.
        printf("Server: Sending Acknowledgement.\n");
        sendto(sockf, Ack, 5, 0, (struct sockaddr*)&client, size);

        // Checking for lost packets.
        while(lost_packet(packet_num, Ack)){
            //Recieving lost packets.
            int no_of_lost_packets = 0;
            no_of_lost_packets = f_lost_packet(lost_packs, Ack , packet_num);
            printf("Server: Receiving lost packets.\n");
            packet_recv(sockf, client, size, no_of_lost_packets, window, &transmission_end, Ack, len);
            
            printf("Server: Sending Acknowledgement.\n");
            sendto(sockf, Ack, 5, 0, (struct sockaddr*)&client, size);
        }
        // Writing to a new file.
        int pack_num = 0;
        for(int x=0; x<packet_num; x++){
            for(int j=0; j<packet_num; j++){
            pack_num = window[j][0] - '0';
            if(pack_num == x){
                if (fwrite(window[j] + 2, sizeof(char), (len[x]-2), f) <= 0){
                        perror("Write File Error");
                        exit(1);
                    }
                    break;
                }
        }                
    }
        // Reseting window.
        for(int x=0; x<6; x++){

        memset(&window[x], 0, sizeof(window[x]));
        } 
        memset(&Ack, 0, 5);

        if(transmission_end){
            break;
        }
         
    }
    printf("\nFile received successfully.\n");
    close(sockf);
    fclose(f);
    memset(&client, 0, sizeof(client));
    memset(&server, 0, sizeof(server));
    return 0;
}


int packet_no_recv(int sockf, struct sockaddr_in client, int size){

    char no_of_packets[5];
    memset(&no_of_packets, 0, sizeof(no_of_packets));
    recvfrom(sockf,no_of_packets, sizeof(no_of_packets), 0,(struct sockaddr *)&client, &size);
    return no_of_packets[0] - '0';

}


void packet_recv(int sockf, struct sockaddr_in client,int size, int packet_num,char window[6][PACKET_SIZE], bool *transmission_end, bool *Ack, int *len){

    int pack_num = 0;
    printf("Server: Accepting packets.\n");

    for(int x=0; x<=packet_num; x++){
        
        len[x] = recvfrom(sockf,window[x],PACKET_SIZE, 0, (struct sockaddr *)&client, &size);
        
        if(strcmp(window[x],"file end") == 0){
            *transmission_end = true;
            break;
        }

        if(strcmp(window[x],"finish") == 0){
            break;
        }

        pack_num = window[x][0] - '0';
        Ack[pack_num] = 1;
    }



}

int lost_packet(int packet_num,bool *Ack){

    printf("Server: Checking for missing packets...\n");
    for(int x=0; x<packet_num; x++){
        if(!Ack[x]){
            return 1;
        }
    }
    return 0;

}

int f_lost_packet(int *lost_packs, bool *Ack, int packet_num){

    int no_of_lost_packets = 0;
    memset(&lost_packs, 0, 20);

    for(int x=0; x<packet_num; x++){
        if(!Ack[x]){
            lost_packs[x] = 1;
            no_of_lost_packets += 1;
        }
    }

    return no_of_lost_packets;


}
