#define _GNU_SOURCE
#define BUFLEN 1000
#define BACKLOG 1
#define BUFCLR bzero(buf,BUFLEN);
#define DLY 1
#include<sys/timeb.h>
#include<time.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<stdbool.h>
#include <sys/ioctl.h>
#include <net/if.h>

int sndSock;
struct sockaddr_in cliAddr, serAddr;
struct in_addr lanAddr;
socklen_t addrLen;
size_t sndLen;
char buf[BUFLEN];
FILE* fp;
long fileSize;

static inline void errExit(){
	close(sndSock);
	exit(1);
}

int main(int argc, char *argv[]){
	BUFCLR
	bzero(&cliAddr, sizeof(cliAddr));
	
	cliAddr.sin_family=AF_INET;
	cliAddr.sin_port=htons(10000);
	cliAddr.sin_addr.s_addr=inet_addr("224.0.0.3");
	addrLen=sizeof(serAddr);

	lanAddr.s_addr=inet_addr("10.76.36.1");
	
	sndSock=socket(AF_INET, SOCK_DGRAM, 0);
	if(sndSock==-1){
		perror("socket()");
		errExit();
	}
	printf("Opening the datagram socket...OK.\n");
	
	if(setsockopt(sndSock, IPPROTO_IP, IP_MULTICAST_IF, &lanAddr, (socklen_t)sizeof(lanAddr))<0){
		perror("setsockopt() for multicast iface");
		errExit();
	}
	printf("Setting the local interface...OK\n");
	
	//send filename
	strncpy(buf, argv[1], BUFLEN);
	sndLen=strnlen(buf, BUFLEN);
	if(sendto(sndSock, buf, sndLen, 0, (struct sockaddr*)&cliAddr, (socklen_t)sizeof(cliAddr))<0){
		perror("sendto() for filename");
		errExit();
	}

	//get fileSize
	if((fp=fopen(argv[1], "r"))==NULL){
		perror("fopen()");
		fclose(fp);
		errExit();
	}
	fseek(fp, 0L, SEEK_END);
	fileSize=ftell(fp);
	printf("file size: %ld bytes\n", fileSize);
	rewind(fp);
		
	//send fileSize
	/*BUFCLR
	sprintf(buf,"%ld",fileSize);
	sndLen=strnlen(buf, BUFLEN);
	if(sendto(sndSock, buf, sndLen, 0, (struct sockaddr*)&cliAddr, (socklen_t)sizeof(cliAddr))<0){
		perror("sendto() for filename");
		errExit();
	}*/
	
	//send the file
	while(!feof(fp)){	
		BUFCLR
		sndLen=fread((void *)buf,1,BUFLEN,fp);
		if(sendto(sndSock, (const void *)buf, sndLen, 0,(struct sockaddr*)&cliAddr,(socklen_t)sizeof(cliAddr))<0){
			perror("sendto()");
			fclose(fp);
			errExit();
		}
	}
	sendto(sndSock, (const void *)buf, 0, 0, (struct sockaddr*)&cliAddr, (socklen_t)sizeof(cliAddr));
	printf("Transmission successful!\n");

	fclose(fp);
	close(sndSock);
	return 0;
}
