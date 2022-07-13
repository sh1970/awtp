#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define UCENTRAL_REDIRECTOR		"/etc/ucentral/redirector.json"
#define UCENTRAL_FORMAT			"{\"Name\":\"%s\",\"Redirector\":\"%s:%d\"}"
#define UCENTRAL_DEFAULT_PORT		15002
#define DISCOVERY_INTERVAL		60
#define DISCOVERY_INTERVAL_FAIL		5
#define DISCOVERY_ONLY			1

int discovery_only = DISCOVERY_ONLY;
int discovery_interval = DISCOVERY_INTERVAL;
int ucentral_port = UCENTRAL_DEFAULT_PORT;
int debug_flag = 0;

unsigned char capwap_discovery[220] = {0x00,0x10,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0xcf,0x00,0x00,0x14,0x00,0x01,0x01,0x00,0x26,0x00,0x33,0x00,0x01,0xe2,0x40,0x00,0x00,0x00,0x07,0x57,0x69,0x41,0x50,0x38,0x32,0x30,0x00,0x01,0x00,0x08,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x00,0x02,0x00,0x03,0x33,0x2e,0x30,0x00,0x03,0x00,0x03,0x34,0x2e,0x30,0x00,0x04,0x00,0x06,0x3c,0x97,0x0e,0xc0,0xe2,0xe0,0x00,0x27,0x00,0x42,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x5b,0xa0,0x00,0x00,0x00,0x07,0x57,0x69,0x41,0x50,0x38,0x35,0x30,0x00,0x00,0x82,0xb1,0x00,0x01,0x00,0x04,0x57,0x69,0x41,0x43,0x00,0x00,0xa9,0xc2,0x00,0x02,0x00,0x0b,0x4c,0x69,0x6e,0x75,0x78,0x20,0x35,0x2e,0x30,0x2e,0x34,0x00,0x00,0xd0,0xd3,0x00,0x03,0x00,0x06,0x4f,0x74,0x68,0x65,0x72,0x31,0x00,0x29,0x00,0x01,0x0c,0x00,0x2c,0x00,0x01,0x00,0x04,0x18,0x00,0x05,0x01,0x00,0x00,0x00,0x04,0x00,0x25,0x00,0x33,0x00,0x00,0x48,0xf9,0x00,0x12,0x01,0x00,0x0f,0xac,0x01,0x00,0x0f,0xac,0x05,0x00,0x0f,0xac,0x02,0x00,0x0f,0xac,0x04,0x00,0x0f,0xac,0x0a,0x00,0x0f,0xac,0x08,0x00,0x0f,0xac,0x09,0x00,0x0f,0xac,0x06,0x00,0x0f,0xac,0x0d,0x00,0x0f,0xac,0x0b,0x00,0x0f,0xac,0x0c};

static int parse_args(int argc, char *argv[])
{
	int c;
	opterr = 1;

	while ( (c = getopt (argc, argv, "i:P:vodh")) != -1) {
		
		switch (c) {
			case 'v':
				printf("awtp version 0.0.1\n");
				exit(0);
				break;
			case 'o':
				discovery_only = DISCOVERY_ONLY;
				break;
			case 'i':
				discovery_interval = atoi(optarg);		
				break;
			case 'P':
				ucentral_port = atoi(optarg);		
				break;
			case 'd':
				debug_flag = 1;
				break;
			case '?':
				exit(0);
			default:
			case 'h':
				printf("%s: -voiPh\n", argv[0]);
				exit(0);
				break;
		}
	}
	return 0;
}

/*
 * 
 *  get ac ip from discovery response
 *  save ac ip to /etc/ucentral/redirector.json
 *  format: {"redirector": "10.10.7.180:15002"}
 *
 */
void save_ac_ip_to_ucentral(char *ip)
{
	FILE *fp = NULL;

	printf("AC ip = %s\n", ip);

	if ( NULL != (fp = fopen(UCENTRAL_REDIRECTOR, "w"))) {
		fprintf(fp, UCENTRAL_FORMAT, "WiAC", ip, ucentral_port);
		fclose(fp);
	}
	return;
}

int main (int argc, char **argv)
{
	/* read command line args, results are in bootcfg */
	parse_args(argc, argv);

	int sockfd;
	int opt;
	char ip[16];
	char recvbuff[2048];
	struct timeval tv;
	int ret = 0;

	if ( debug_flag == 0 ) {
		daemon(0, 0);
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1) {
		printf("Can't create socket for: %s", strerror(errno));
		return -1;
	}

	opt = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0) {
		printf("Can't set broadcast sockopt.");
		return -2;
	}

	tv.tv_sec = 5;
	tv.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		printf("can't set socket timeout.");
		return -2;
	}

	struct sockaddr_in addrto;
	bzero(&addrto, sizeof(struct sockaddr_in));
	addrto.sin_family = AF_INET;  
	addrto.sin_addr.s_addr = inet_addr("255.255.255.255");
	addrto.sin_port = htons(5246);  
	int nlen = sizeof(addrto); 

	struct sockaddr_in addrfrom;
	int flen = sizeof(addrfrom);

	while(1) {  

		ret = sendto(sockfd, capwap_discovery, sizeof(capwap_discovery), 0, (struct sockaddr *)&addrto, nlen);
		printf("send ret= %d.", ret);
		if ( ret < 0 ) {  
			printf("send fail, sleep.");
			sleep(DISCOVERY_INTERVAL_FAIL);
			continue;
	       	} else {         
			memset((void *)recvbuff, 0, sizeof(recvbuff));
			ret = recvfrom(sockfd, recvbuff, sizeof(recvbuff), 0, (struct sockaddr *)&addrfrom, &flen);
			printf("recv ret= %d.", ret);
			if ( ret >= 0 ) {
				printf("recv success, sleep.");
				save_ac_ip_to_ucentral(inet_ntoa(addrfrom.sin_addr));
				sleep(discovery_interval);
			}else {
				printf("recv fail, sleep.");
				sleep(DISCOVERY_INTERVAL_FAIL);
				continue;
			}
		}  
	} 

	return(0);
}
