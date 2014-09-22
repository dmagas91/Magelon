
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <sys/stat.h>
#include <errno.h>
#define PORT 1234
#define MAXLEN 1400


int Socket(int family, int type, int protocol){
	int n;
	char *error;
	if ((n=socket(family,type,protocol))==-1){
		error = strerror(errno);
		printf("%s\n",error);
		exit(4);
	} else {
		return n;
	}
}

int Getaddrinfo(const char *hostname, const char *service, const struct addrinfo *hints, struct addrinfo **result) {
	int err;
	char *error;
	err = getaddrinfo(hostname, service, hints, result);
	if (err){
		error = strerror(errno);
		printf("%s\n",error);
		exit(5);
	} else {
		return 0;
	}
}

int Connect (int sockfd, const struct sockaddr *server, socklen_t addrlen){
	int error;
	error=connect(sockfd, server, addrlen);
	if (error==-1){
		err(1,"connect\n");
		exit (-1);
	} else {
		return 0;
	}
}

ssize_t Send(int s, const void *msg, size_t len, int flags){
	int error;
	error=send(s, msg, len, flags);
	if (error==-1){
		err(1,"send\n");
		exit (-1);
	} else {
		return 0;
	}
}

ssize_t readn(int fd, void *vptr, size_t n){
	
	size_t nleft;
	size_t nread;
	char *ptr;
	
	ptr = vptr;
	nleft = n;
	
	while (nleft >0){
		if( (nread = read(fd, ptr, nleft))<0){
			if (errno == EINTR)
				nread = 0;
			else
				return (-1);
		} else if (nread == 0)
					break;
		nleft -= nread;
		ptr += nread;
	}
	return (n-nleft); 			
} 


int main(int argc, char *argv[]){
	int pflag = 0;
	int cflag = 0;
	// int newflag=0;
	int mysocket;
	// struct sockaddr_in servaddr;
	void *msgbuff;
	char msgbuff_rec[MAXLEN];
	struct addrinfo hints, *res;
	int options;
	int error;
	int i=0;
	FILE *f;
	uint32_t pomak = 0;
	uint32_t vrijeme = 0;
	uint8_t status;
	ssize_t recvlen;
	struct stat fileStat;

	
	if (argc!=3 && argc !=5 && argc!=4 && argc!=6){
		err(3,"Usage: ./tcpklijent [-p port] [-c] server_IP file_name\n");
	}
	while ((options = getopt(argc, argv, "cp:")) != -1){
		switch (options){
			case 'p':
				// memset(&servaddr,0,sizeof(servaddr));
				memset(&hints, 0 , sizeof (hints));
				hints.ai_family = AF_INET;
				hints.ai_socktype = SOCK_DGRAM;
				hints.ai_flags =  AI_PASSIVE;
				Getaddrinfo(argv[argc-2], optarg, &hints, &res);
				
				// servaddr.sin_port = ((struct sockaddr_in *)res->ai_addr)->sin_port;
				// servaddr.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
				pflag=1;
				break;
			case 'c':
				cflag=1;
				break;
			default:
				err(3,"Usage: ./tcpsklijent [-p port] [-c] server_IP file_name\n");
				break;
		}
	}
	if (pflag==0){
		
		memset(&hints, 0 , sizeof (hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags|=AI_CANONNAME;
		// int port;
		// port = htons(PORT);
		char str[15];
		sprintf(str, "%d", PORT);
		Getaddrinfo(argv[argc-2], str, &hints, &res);
		
		// servaddr.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
	}

	stat(argv[argc-1], &fileStat);
	int ftime = (int) fileStat.st_mtime;
	// if (newflag==0){
	vrijeme = (uint32_t) ftime;
	// }
	if (cflag){
		pomak = fileStat.st_size;
	} 
	
	f = fopen(argv[argc-1], "a");
		if (f == NULL){
			// file ne postoji
			f = fopen(argv[argc-1], "w+");
			if (f == NULL){
				err(11, "file could not be opened \n");
			}
			// newflag=1;
		}
	
	mysocket = Socket(res->ai_family, res->ai_socktype, 0);
	
	
	// servaddr.sin_family = AF_INET;
	// memset(servaddr.sin_zero, '\0', sizeof (servaddr.sin_zero));
	Connect (mysocket, res->ai_addr, res->ai_addrlen);
	

	pomak = htonl(pomak);
	vrijeme = htonl(vrijeme);
	msgbuff = (void*)malloc(MAXLEN);
	memset(msgbuff, 0, MAXLEN);
	memcpy(msgbuff, (const void*) &pomak, sizeof (uint32_t));
	memcpy(msgbuff+sizeof(uint32_t), (const void*) &vrijeme, sizeof (uint32_t));
	memcpy(msgbuff+sizeof(uint32_t)*2, (const void*) argv[argc-1], strlen (argv[argc-1]));
	Send(mysocket, msgbuff, 8+strlen(argv[argc-1])+1, 0);
	error = readn(mysocket, &status, 1);
	if (error == -1){
		err(22, "readn\n");
	}
	if (status>=1 && status<=99){
		printf("file na serveru ne postoji\n");
		return 0;
	}
	if (status>=100 && status<=199){
		printf("file up to date\n");
		return 0;
	} 
	if (status>=200 && status<=254){
		fseek(f, pomak,SEEK_SET);
		
		while (1){
			memset(msgbuff_rec, 0, sizeof(msgbuff_rec));
			recvlen = recv(mysocket, msgbuff_rec, sizeof(msgbuff_rec), 0);
			if (recvlen == -1){
				err(9, "receve\n");
			}
			if (recvlen == 0) {
				break;
			}
			error = fputs(msgbuff_rec, f);
			if (error == -1){
				err(3,"fputs\n");
			} 
			fseek(f, pomak+(MAXLEN*i) ,SEEK_SET);
			i++;
		}
		
	}
	return 0;
}
