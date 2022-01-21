// Client side implementation of UDP client-server model 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h> 

#define MAXBUFLEN 1024

struct packet{
	unsigned int total_frag;
	unsigned int frag_no;
	unsigned int size;
	char* filename;
	char filedata[1000];
};

int main(int argc, char *argv[]) { 
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	char buf[MAXBUFLEN];

	// File stuff
	FILE* yo;
	int read;
	int errors;
    struct stat sb;
    long long num_packets;
	int size;
	int last_size;
	struct packet* pckt = (struct packet*) malloc(sizeof(struct packet));

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}
	
	clock_t time1 = clock();

	// If the file does not exist or cannot be read accessed
	if ((yo = fopen(argv[4], "r")) == NULL){
		if ((numbytes = sendto(sockfd, "nope", strlen("nope"), 0, p->ai_addr, p->ai_addrlen)) == -1){
			perror("talker: sendto");
			exit(1);
		}
		return 0;
	}
	
	// Else the file exists, and can be opened and read
	if ((numbytes = sendto(sockfd, argv[3], strlen(argv[3]), 0, p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}

	int len;
	numbytes = recvfrom(sockfd, (char *)buf, MAXBUFLEN-1, MSG_WAITALL, p->ai_addr, &len);
	if (numbytes == -1){
		perror("recvfrom");
		exit(1);
	}

	buf[numbytes] = '\0';
	printf("Message from server: %s\n", buf);

	if (strcmp("yes", buf) != 0){
		printf("A file transfer can not start.\n");
		return 0;
	}
	printf("A file transfer can start.\n");

	// Once a file transfer can start:
	if (stat((const char *)argv[4], &sb) == -1) perror("stat");
	last_size = (int)sb.st_size;
	num_packets = (last_size/1000) + 1;
	printf("File size: %d bytes\n", last_size);
	printf("%d\n", num_packets);

	pckt->total_frag = num_packets;
    pckt->size = 1000;
	pckt->filename = argv[4];

	char buff[MAXBUFLEN];

	// ACTUAL FILE TRANSFER
	for (int i = 1; i < num_packets; i++){
        pckt->frag_no = i;
		memset(pckt->filedata, 0, 1000);
		printf("%s\n", pckt->filedata);
        read = fread((void*)pckt->filedata, sizeof(char), 1000, yo);
		if (read != 1000){
			printf("ERROR READING THIS PACKET\n");
		}
        last_size -= read;
		printf("Leftover bytes: %d\n", last_size);

        memset(buff, 0, MAXBUFLEN);
        sprintf(buff, "%d:%d:%d:%s:", pckt->total_frag, pckt->frag_no, pckt->size, pckt->filename);
        memcpy((void*)buff + strlen(buff), (const void*)pckt->filedata, pckt->size);

		// SENDING FILE
		if ((numbytes = sendto(sockfd, buff, MAXBUFLEN, 0, p->ai_addr, p->ai_addrlen)) == -1) {
			perror("talker: sendto");
			exit(1);
		}
		printf("total: %d, frag_no: %d, size: %d, filename: %s\n", pckt->total_frag, pckt->frag_no, pckt->size, pckt->filename);
		while(1){
			// If response = i, break/continue looping
			numbytes = recvfrom(sockfd, (char *)buf, MAXBUFLEN-1, MSG_WAITALL, p->ai_addr, &len);
			if (numbytes == -1){
				perror("recvfrom");
				exit(1);
			}
			buf[numbytes] = '\0';
			if (atoi(buf) == pckt->frag_no) break;
			printf("str8 looping\n");
		}
    }
	pckt->frag_no = num_packets;
	pckt->size = last_size;
    memset(pckt->filedata, 0, 1000);
    fread((void*)pckt->filedata, 1, pckt->size, yo);

	memset(buff, 0, MAXBUFLEN);
    sprintf(buff, "%d:%d:%d:%s:", pckt->total_frag, pckt->frag_no, pckt->size, pckt->filename);
    memcpy((void*)buff + strlen(buff), (const void*)pckt->filedata, pckt->size);

	// SENDING FILE
	if ((numbytes = sendto(sockfd, buff, MAXBUFLEN, 0, p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}

	while(1){
		// If response = i, break/continue looping
		numbytes = recvfrom(sockfd, (char *)buf, MAXBUFLEN-1, MSG_WAITALL, p->ai_addr, &len);
		if (numbytes == -1){
			perror("recvfrom");
			exit(1);
		}
		buf[numbytes] = '\0';
		if (atoi(buf) == pckt->frag_no) break;
		printf("last looping\n");
	}
	printf("total: %d, frag_no: %d, size: %d, filename: %s\n", pckt->total_frag, pckt->frag_no, pckt->size, pckt->filename);
	printf("Program took %.2f ms.\n", difftime(clock(), time1));

	fclose(yo);
    free(pckt);

	return 0; 
} 
