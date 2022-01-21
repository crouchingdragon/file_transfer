// Server side implementation of UDP client-server model 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAXBUFLEN 1024

struct packet{
	unsigned int total_frag;
	unsigned int frag_no;
	unsigned int size;
	char* filename;
	char filedata[1000];
};

// Driver code 
int main(int argc, char *argv[]){

	char *myport = argv[argc-1]; // port number string will be here
	struct addrinfo hints;
    struct addrinfo *servinfo, *p; // will point to the results
	struct sockaddr_storage their_addr;

	int sockfd; // socket file descriptor
	int rv; // return var 
	char buf[MAXBUFLEN];
	socklen_t addr_len;

	char* saveptr;
	FILE* yo;
	char ack[MAXBUFLEN];
	struct packet* pckt = (struct packet*) malloc(sizeof(struct packet));

	memset(&hints, 0, sizeof hints);
	hints.ai_family =  AF_INET; //IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, myport, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);

	printf("listener: waiting to recvfrom...\n");

	addr_len = sizeof their_addr;
	int numbytes;
	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}

	buf[numbytes] = '\0';
	printf("Client said: %s\n", buf);

	if (strcmp("ftp", buf) == 0){
		numbytes = sendto(sockfd, "yes", strlen("yes"), MSG_CONFIRM, (const struct sockaddr*)&their_addr, addr_len);
		if (numbytes == -1){
			perror("talker: sendto");
			exit(1);
		}
		printf("Sent 'yes' to the client!\n");
	}
	
	else {
		numbytes = sendto(sockfd, "no", strlen("no"), MSG_CONFIRM, (const struct sockaddr*)&their_addr, addr_len);
		if (numbytes == -1){
			perror("talker: sendto");
			exit(1);
		}
		printf("Wasn't valid\n");
	}

	// ACTUAL FILE TRANSFER
	int i = 1;
	while (1){
		// FIRST FILE
		printf("WAITING ON A FILE\n");
		if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			exit(1);
		}

		// After you get the first file, figure out how many things you're waiting for, then send the ack for the first one
		pckt->total_frag = atoi(strtok_r(buf, ":", &saveptr));
		pckt->frag_no = atoi(strtok_r(NULL, ":", &saveptr));
		pckt->size = atoi(strtok_r(NULL, ":", &saveptr));
		pckt->filename = strtok_r(NULL, ":", &saveptr);
		printf("total: %d, frag_no: %d, size: %d, filename: %s\n", pckt->total_frag, pckt->frag_no, pckt->size, pckt->filename);
		memset(pckt->filedata, 0, 1000);
		memcpy((void*)pckt->filedata, (const void*)saveptr, pckt->size); // was pckt_size-1 

		if (pckt->frag_no == 1) yo = fopen(pckt->filename, "w");
		fwrite(pckt->filedata, sizeof(char), pckt->size, yo);

		while (pckt->frag_no != i);
		sprintf(ack, "%d", pckt->frag_no);
		numbytes = sendto(sockfd, ack, strlen(ack), MSG_CONFIRM, (const struct sockaddr*)&their_addr, addr_len);
		if (numbytes == -1){
			perror("talker: sendto");
			exit(1);
		}

		if(pckt->frag_no == pckt->total_frag){
			fclose(yo);
			break;
		}
		i++;
	}

	close(sockfd);
    free(pckt);

	return 0; 
} 
