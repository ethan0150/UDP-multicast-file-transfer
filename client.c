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

int rcvSock, reuse=1;
struct sockaddr_in cliAddr, serAddr;
socklen_t addrLen;
ssize_t rcvLen;
struct ip_mreq mcastGroup;
struct ifreq eth0Addr;
char buf[BUFLEN];
struct timeval timeo={DLY,0};
FILE* fp;
long fileSize;

static inline void errExit(){
	close(rcvSock);
	exit(1);
}

int main(int argc, char *argv[]){	
	BUFCLR
	bzero(&cliAddr, sizeof(cliAddr));
	bzero(&serAddr, sizeof(serAddr));
	bzero(&mcastGroup, sizeof(mcastGroup));

	cliAddr.sin_family=AF_INET;
	cliAddr.sin_port=htons(10000);
	cliAddr.sin_addr.s_addr=htonl(INADDR_ANY);	
	addrLen=sizeof(serAddr);

	eth0Addr.ifr_addr.sa_family=AF_INET;
	strncpy(eth0Addr.ifr_name, "eth0", IFNAMSIZ-1);
	
	rcvSock=socket(AF_INET, SOCK_DGRAM, 0);
	if(rcvSock==-1){
		perror("socket()");
		errExit();
	}
	else printf("Opening datagram socket....OK.\n");

	if(ioctl(rcvSock, SIOCGIFADDR, &eth0Addr)<0){
		perror("ioctl()");
		errExit();
	}

	mcastGroup.imr_multiaddr.s_addr=inet_addr("224.0.0.3");
	mcastGroup.imr_interface.s_addr=((struct sockaddr_in *)&eth0Addr.ifr_addr)->sin_addr.s_addr;
	printf("%s\n", inet_ntoa(((struct sockaddr_in *)&eth0Addr.ifr_addr)->sin_addr));
	
	if(setsockopt(rcvSock, SOL_SOCKET, SO_REUSEADDR, &reuse, (socklen_t)sizeof(reuse))<0){
		perror("setsockopt() for SO_REUSEADDR");
		errExit();
	}
	else printf("Setting SO_REUSEADDR...OK.\n");

	/*if(setsockopt(rcvSock, SOL_SOCKET, SO_RCVTIMEO, &timeo, (socklen_t)sizeof(struct timeval))<0){    
		perror("seteockopt() for timeo");    
		errExit();
	}*/

	if(bind(rcvSock, (struct sockaddr*)&cliAddr, (socklen_t)sizeof(cliAddr))<0){
		perror("bind()");
		errExit();
	}
	else printf("Binding datagram socket...OK.\n");

	if(setsockopt(rcvSock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcastGroup, (socklen_t)sizeof(mcastGroup))<0){
		perror("setsockopt() for joining group");
		errExit();
	}
	else printf("Adding multicast group...OK.\n");

	printf("W8ting for server...\n");
	
	//get filename
	rcvLen=recvfrom(rcvSock, (void *)buf, BUFLEN, 0, (struct sockaddr*)&serAddr, &addrLen);
	if(rcvLen<0){
		perror("recvfrom()");
		errExit();
	}
	printf("filename: %s\n",buf);

	//open file
	if((fp=fopen(buf, "wb"))==NULL){
		perror("fopen()");
		fclose(fp);
		errExit();
	}
	BUFCLR
	//recieve file
	while((rcvLen=recvfrom(rcvSock, (void *)buf, BUFLEN, 0, (struct sockaddr*)&serAddr, &addrLen))>0){
		fwrite((const void *)buf,1,rcvLen,fp);
		BUFCLR
	}
	fflush(fp);
	printf("Reading datagram message...OK.\n");

	rewind(fp);
	fseek(fp, 0L, SEEK_END);
	fileSize=ftell(fp);
	printf("file size: %ld bytes\n", fileSize);

	fclose(fp);
	close(rcvSock);
	return 0;
}
