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
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

#define UCENTRAL_CONFIG_PATH 		"/etc/ucentral"
#define UCENTRAL_REDIRECTOR_IP		"/etc/ucentral/redirector.ip"
#define UCENTRAL_REDIRECTOR		"/etc/ucentral/redirector.json"
#define UCENTRAL_FORMAT			"{\"Name\":\"%s\",\"Redirector\":\"%s:%d\"}"
#define UCENTRAL_DEFAULT_PORT		15002
#define DISCOVERY_INTERVAL		60
#define DISCOVERY_INTERVAL_FAIL		5
#define DISCOVERY_ONLY			1
#define MAX_FILES_TO_KEEP 		5
#define MAX_CLEANS	 		10

int discovery_only = DISCOVERY_ONLY;
int discovery_interval = DISCOVERY_INTERVAL;
int ucentral_port = UCENTRAL_DEFAULT_PORT;
int debug_flag = 0;
int cleans = 0;
char ac_ip[16];
char ac_command[1024];

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

int clean_ucentral_config(char *path)
{
    const char *dir_path = path;
    struct dirent **files;
    int file_count = 0;

    // Open the directory
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("opendir");
        return 1;
    }

    // Count files in the directory
    struct dirent *entry;
    while ((entry = readdir(dir))) {
		if (strcmp(entry->d_name, "ucentral.cfg.0000000001") == 0)
			continue;
        if (strncmp(entry->d_name, "ucentral.cfg.", 13) == 0) {
            file_count++;
        }
    }

    // Check if there are more files than the maximum allowed
    if (file_count <= MAX_FILES_TO_KEEP) {
        // No need to remove any files
        closedir(dir);
        return 0;
    }

    // Allocate memory for file list
    files = (struct dirent **)malloc(file_count * sizeof(struct dirent *));
    if (!files) {
        perror("malloc");
        closedir(dir);
        return 1;
    }

    // Rewind the directory and populate the file list
    rewinddir(dir);
    int i = 0;
    while ((entry = readdir(dir))) {
		if (strcmp(entry->d_name, "ucentral.cfg.0000000001") == 0)
			continue;
        if (strncmp(entry->d_name, "ucentral.cfg.", 13) == 0) {
            files[i] = entry;
            i++;
        }
    }

    // Get modification times for each file
    struct stat file_stat;
    for (i = 0; i < file_count; i++) {
        char file_path[PATH_MAX];
        snprintf(file_path, PATH_MAX, "%s/%s", dir_path, files[i]->d_name);
        if (stat(file_path, &file_stat) == 0) {
            files[i]->d_ino = file_stat.st_mtime;
        } else {
            perror("stat");
        }
    }

    // Manual sorting (bubble sort) based on modification time (newest first)
    for (int i = 0; i < file_count - 1; i++) {
        for (int j = 0; j < file_count - i - 1; j++) {
            if (files[j]->d_ino < files[j + 1]->d_ino) {
                struct dirent *temp = files[j];
                files[j] = files[j + 1];
                files[j + 1] = temp;
            }
        }
    }

    // Delete excess files beyond the maximum allowed
    for (i = MAX_FILES_TO_KEEP; i < file_count; i++) {
        char file_to_remove[PATH_MAX];
        snprintf(file_to_remove, PATH_MAX, "%s/%s", dir_path, files[i]->d_name);
        if (remove(file_to_remove) != 0) {
            perror("remove");
            free(files);
            closedir(dir);
            return 1;
        }
        //printf("Removed: %s\n", files[i]->d_name);
    }

    // Clean up and close the directory
    free(files);
    closedir(dir);

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

	//printf("AC ip = %s\n", ip);

	if ( NULL != (fp = fopen(UCENTRAL_REDIRECTOR, "w"))) {
		fprintf(fp, UCENTRAL_FORMAT, "WiAC", ip, ucentral_port);
		fclose(fp);
	}

	/* not same */
	if ( 0 != strcmp(ip, ac_ip) ) {
		memset((void *)ac_command, 0, sizeof(ac_command));
		sprintf(ac_command, "sed -i \"/option server/c\ \toption server '%s'\" /etc/config-shadow/ucentral", ip);
		system(ac_command);
		sprintf(ac_command, "sed -i \"/option port/c\ \toption port '%d'\" /etc/config-shadow/ucentral", ucentral_port);
		system(ac_command);

		/* save ip */
		if ( NULL != (fp = fopen(UCENTRAL_REDIRECTOR_IP, "w"))) {
			fprintf(fp, "%s", ip);
			fclose(fp);
			/* set it */
			strcpy(ac_ip, ip);
		}

		/* reboot it */
		/*
		system("/etc/init.d/ucentral restart");   
		*/
		system("reload_config");
		sleep(DISCOVERY_INTERVAL);
		system("/usr/bin/killall -9 flock");
		system("/sbin/reboot");
	}
	return;
}

void get_ac_ip_from_ucentral()
{
	FILE *fp = NULL;
	char *line = NULL;
	char sline[1024];

	if ( NULL != (fp = fopen(UCENTRAL_REDIRECTOR_IP, "r"))) {
		line = fgets(sline, 1024, fp);
		if ( line != NULL ) {
			strcpy(ac_ip, sline);
		}
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
	char recvbuff[512];
	struct timeval tv;
	int ret = 0;
	int retry = 0;

#if 0
	/*
	 * daemon may cause openwrt spawan fail
	 */
	if ( debug_flag == 0 ) {
		daemon(0, 0);
	}
#endif

	memset((void *)ac_ip, 0, sizeof(ac_ip));
	get_ac_ip_from_ucentral();

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

	tv.tv_sec = 3;
	tv.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		printf("can't set socket timeout.");
		return -2;
	}

	struct sockaddr_in addrlocal;
	bzero(&addrlocal, sizeof(struct sockaddr_in));
	addrlocal.sin_family = AF_INET;  
	addrlocal.sin_addr.s_addr = INADDR_ANY;
	addrlocal.sin_port = htons(0);  
	int llen = sizeof(addrlocal); 

	if ( bind(sockfd, (struct sockaddr *)&addrlocal, llen) < 0 ) {
		printf("can't bind socket.");
		return -3;
	}

	struct sockaddr_in addrto;
	bzero(&addrto, sizeof(struct sockaddr_in));
	addrto.sin_family = AF_INET;  
	addrto.sin_addr.s_addr = inet_addr("255.255.255.255");
	addrto.sin_port = htons(5246);  
	int nlen = sizeof(addrto); 

	struct sockaddr_in addrfrom;
	bzero(&addrfrom, sizeof(struct sockaddr_in));
	int flen = sizeof(addrfrom);

	while(1) {  
		ret = sendto(sockfd, capwap_discovery, sizeof(capwap_discovery), 0, (struct sockaddr *)&addrto, nlen);
		//printf("send ret= %d.\n", ret);
		//fflush(stdout);
		if ( ret < 0 ) {  
			//printf("send fail, sleep.\n");
			//fflush(stdout);
			sleep(DISCOVERY_INTERVAL_FAIL);
			continue;
	       	} else {         

			retry = 0;
			memset((void *)recvbuff, 0, sizeof(recvbuff));
			while(1) {
				//sleep(1);
				ret = recvfrom(sockfd, recvbuff, sizeof(recvbuff), 0, (struct sockaddr *)&addrfrom, &flen);
				//printf("recv ret= %d. errorno = %d, error=%s\n", ret, errno, strerror(errno));
				//fflush(stdout);
				if ( ret == -1 ) {
					//printf("recv fail, sleep.\n");
					//fflush(stdout);
					if ( retry > 3 ) break;
					retry++;
					sleep(DISCOVERY_ONLY);
					continue;
				}else if ( ret >=0 ) {
					//printf("recv success, sleep.\n");
					//fflush(stdout);
					save_ac_ip_to_ucentral(inet_ntoa(addrfrom.sin_addr));
					sleep(DISCOVERY_INTERVAL);
					break;
				}

			}
		}

		/* discovery timeout, get AC from WiCloud. */
		if ( retry > 3 ) {
			system("/usr/bin/WiCloud.sh");
		}

		cleans++;
		if ( cleans >= MAX_CLEANS ) {
			/* clean config path */
			clean_ucentral_config(UCENTRAL_CONFIG_PATH);
		}
	} 

	return(0);
}
