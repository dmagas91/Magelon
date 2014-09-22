

#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <sys/stat.h>
#include <errno.h>
#include <syslog.h>
#include <stdarg.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>

#define MAXBUFF 512

int Socket(int family, int type, int protocol, int dflag){
	int n;
	if ((n=socket(family,type,protocol))==-1){
		if (dflag){
			syslog(LOG_ALERT, "TFTP ERROR %d fb45558 socket\n",errno);
			return -1;
		}else{
			fprintf(stderr,"TFTP ERROR %d fb45558 socket\n",errno);
			return -1;
		}
		
		
	} else {
		return n;
	}
}

ssize_t Sendto(int sockfd, void *buff, size_t nbytes, int flags, const struct sockaddr* to, socklen_t addrlen, int dflag){
	int erro;
	erro= sendto(sockfd, buff, nbytes, flags, to, addrlen);
	if (erro<0){
		if (dflag){
			syslog(LOG_ALERT, "TFTP ERROR %d fb45558 send\n",errno);
			return -1;
		}else{
			fprintf(stderr,"TFTP ERROR %d fb45558 send\n",errno);
			return -1;
		}
	}else{
		return 0;
	} 
}

int Getaddrinfo(const char *hostname, const char *service, const struct addrinfo *hints, struct addrinfo **result, int dflag) {
	int erro;
	
	erro = getaddrinfo(hostname, service, hints, result);
	if (erro<0){
		if (dflag){
			syslog(LOG_ALERT, "TFTP ERROR %d fb45558 getaddrinfo\n",errno);
			return -1;
		}else{
			fprintf(stderr,"TFTP ERROR %d fb45558 getaddrinfo\n",errno);
			return -1;
		}
	} else {
		return 0;
	}
}

ssize_t Recvfrom(int sockfd, void *buff, size_t nbytes, int flags, struct sockaddr* from, socklen_t *fromaddrlen, int dflag){
	
	int n;
	n=recvfrom(sockfd, buff, nbytes, flags, from, fromaddrlen);
	if (n<0){
		if (dflag){
			syslog(LOG_ALERT, "TFTP ERROR %d fb45558 recvfrom\n",errno);
			return -1;
		}else{
			fprintf(stderr,"TFTP ERROR %d fb45558 recvfrom\n",errno);
			return -1;
		}
	}else{
		return n;
	}
}

int Bind (int sockfd, const struct sockaddr *myaddr, int addrlen, int dflag){
	int erro;
	erro=bind(sockfd, myaddr, addrlen);
	if (erro<0){
		if (dflag){
			syslog(LOG_ALERT, "TFTP ERROR %d fb45558 bind\n",errno);
			return -1;
		}else{
			fprintf(stderr,"TFTP ERROR %d fb45558 bind\n",errno);
			return -1;
		}
	}else{
		return 0;
	}
}

int deamon_init(const char *pname, int facility){
	int i;
	pid_t pid;
	
	if ((pid = fork()) < 0)
		return (-1);
	else if (pid)
		_exit(0);
	
	if (setsid() < 0)
		return (-1);
	
	signal (SIGHUP, SIG_IGN);
	
	if ((pid = fork())< 0 )
		return (-1);	
	else if (pid)
		_exit(0);
	//chdir ("/tftpboot");
	
	for (i=0; i<64; i++)
		close (i);
	
	open("/dev/null", O_RDONLY);
	open("/dev/null", O_RDWR);
	open("/dev/null", O_RDWR);	
	
	openlog("MrePro tcpserver", LOG_PID, LOG_FTP);
	return 0;
}

int netasciiread(char *buffer, FILE *f, int *flag, int *newline){
	int i=0;
	int broj=0;
	char znak;
	if (*flag == 1 ){
		buffer[i++]='\n';
	}
	while(broj++ <= 512){
		if (feof(f) !=0) {
			return broj;
		}
		znak = fgetc(f);
		if(znak == '\n'){
			*newline=*newline+1;
			buffer[i++] = '\r';
			if(++broj >= 512){
				*flag = 1;
				return 512;
			}
		}
		//printf("%x",znak);
		if (znak != EOF && znak!=0){
			
			buffer[i++] = znak;
		}
	}
	*flag=0;
	return 512;
}


int main (int argc, char *argv[]){
	int dflag = 0;
	int mysocket;
	void *msgbuff;
	struct addrinfo hints, *res;
	int options;
	int error;
	pid_t pid;
	struct sockaddr_in cliaddr;
	socklen_t clilen;
	
	
	
	if (argc!=2 && argc!=3){
		perror("Usage: ./tftpserver [-d] port\n");
		exit (-1);
	}
	if (strstr(argv[argc-1],"-d")){
		perror("Usage: ./tftpserver [-d] port\n");
		exit (-1);
	}
	if (strstr(argv[argc-1],"tftpserver")){
		perror("Usage: ./tftpserver [-d] port\n");
		exit (-1);
	}
	if (argc == 1){
		perror("Usage: ./tftpserver [-d] port\n");
		exit (-1);
	}
	while ((options = getopt(argc, argv, "d")) != -1){
		switch (options){
			case 'd':
				dflag = 1;
				deamon_init(argv[0], LOG_FTP);
				break;
			default:
				err(3,"Usage: ./tftpserver [-d] port\n");
				break;
		}
	}
	
	memset(&hints, 0 , sizeof (hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags|=AI_CANONNAME;
	Getaddrinfo(NULL, argv[argc-1], &hints, &res, dflag);
	
	mysocket = Socket(res->ai_family, res->ai_socktype, 0, dflag);
	
	// bind na port koji je zadan
	Bind(mysocket, res->ai_addr, res->ai_addrlen, dflag);
	
	//petlja
	while (1) {
		
		msgbuff = (void *) malloc(255);
		memset(&cliaddr, 0, sizeof(cliaddr));
		clilen=sizeof(cliaddr);
		Recvfrom(mysocket, msgbuff, MAXBUFF, 0, (struct sockaddr *) &cliaddr, &clilen, dflag);
		
		
		//počinje dijete
		if ((pid = fork()) == 0) {
			int newline=0;
			int newsocket;
			int netasciiflag = 1;
			FILE *f;
			//dijete zatvara socket roditelja
			close(mysocket);
			char *mode;
			char *filename;
			uint16_t *msgkod;
			// provjera je li kod valjan
			msgkod=msgbuff;
			*msgkod = ntohs(*msgkod);
			uint16_t kod;
			uint16_t koderr;
			void *msgbuff_reply;
			if (*msgkod != 1 ) {
				msgbuff_reply = (void*) malloc(50);
				memset(msgbuff_reply, 0, 50);
				kod =  5;
				koderr = 4;
				kod = ntohs(kod);
				memcpy(msgbuff_reply, (const void*) &kod, 2);
				memcpy(msgbuff_reply+2, (const void*) &koderr, 2);
				char *poruka = "illegal operation";
				memcpy(msgbuff_reply+4, (const void*) poruka, strlen(poruka));
				Sendto(newsocket, msgbuff_reply, 4+strlen(poruka)+1, 0, (struct sockaddr *)&cliaddr, clilen, dflag);
				
				if (dflag){
					syslog(LOG_ALERT, "TFTP ERROR %d fb45558 send\n",koderr);
					continue;
				}else{
					fprintf(stderr,"TFTP ERROR %d fb45558 send\n",koderr);
					continue;
				}
			}
			
			//pomicem pointer na pocetak filename-a
			filename = (char*) (msgbuff + 2);
			//pomicem pointer mode-a na kraj filename-a
			mode = (char * ) (msgbuff + 2 + strlen(filename) + 1);
			if (strstr(mode,"octet")){
				netasciiflag=0;
			}
			char adresa[255];
			
			inet_ntop(AF_INET, &(cliaddr.sin_addr), adresa, INET_ADDRSTRLEN);
			if(dflag){
				syslog(LOG_INFO, "%s->%s\n", adresa, filename);
			}else{
				fprintf(stdout,"%s->%s\n", adresa, filename);
			}
			//ako je zadano /tftpboot/filename
			if (strchr(filename, '/') ){
				filename = filename + 10 ;
			}
			// pozicioniraj se u traženi folder
			error = chdir("/tftpboot");
			if (error==-1){
				perror("could not positio into directory\n");
				exit(-1);
			}
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_DGRAM;
			hints.ai_flags|=AI_CANONNAME;
			//dobijem proizvoljan port ?
			Getaddrinfo(NULL, NULL, &hints, &res, dflag);
			
			newsocket = Socket(AF_INET, SOCK_DGRAM, 0, dflag);
			
			//error = bind(newsocket, res->ai_addr, res->ai_addrlen);
			//if (error<0){
			//	if (dflag){
			//		syslog(LOG_ALERT, "TFTP ERROR %d fb45558 bind\n",errno);
			//		return -1;
			//	}else{
			//		err(-1,"TFTP ERROR %d fb45558 bind\n",errno);
					
			//	}
			//}
			
			if (netasciiflag == 0 ) {
				// binary mode
				f = fopen(filename, "rb");
				
				
			}else{
				// netascii mode
				f = fopen(filename, "r");
			}
			if (f == NULL ) {
				msgbuff_reply = (void*) malloc(50);
				memset(msgbuff_reply, 0, 50);
				kod =  5;
				koderr = 1;
				kod = ntohs(kod);
				koderr = ntohs(koderr);
				memcpy(msgbuff_reply, (const void*) &kod, 2);
				memcpy(msgbuff_reply+2, (const void*) &koderr, 2);
				char *poruka = "file not found";
				memcpy(msgbuff_reply+4, (const void*) poruka, strlen(poruka));
				Sendto(newsocket, msgbuff_reply, 4+strlen(poruka)+1, 0, (struct sockaddr *)&cliaddr, clilen, dflag);
				
				if (dflag){
					syslog(LOG_ALERT, "TFTP ERROR %d fb45558 file not found\n",koderr);
					continue;
				}else{
					fprintf(stderr,"TFTP ERROR %d fb45558 file not found\n",koderr);
					continue;
				}
			}
			// file uspješno otvoren
			char buffer[512];
			uint16_t packno=0;
			int flag=0;
			int readlen;
			void *msgbuff_recv;
			struct timeval tv;
			tv.tv_sec=3;
			tv.tv_usec=0;
			if (setsockopt(newsocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))<0){
				if (dflag){
					syslog(LOG_ALERT, "TFTP ERROR %d fb45558 setsockopt\n",errno);
					continue;
				}else{
					fprintf(stderr,"TFTP ERROR %d fb45558 setsockopt\n",errno);
					continue;
				}
			}
			msgbuff_reply =(void*) malloc(516);
			while (1) {
				memset(msgbuff_reply, 0, 516);
				memset(buffer, 0, 512);
				if (netasciiflag){
					
					readlen = netasciiread(buffer, f, &flag, &newline);
				}else{
					readlen = fread(buffer, 1, 512, f);
					
				}
				
				kod = 3;
				packno++;
				int retransmit=0;
				kod = htons(kod);
				packno = htons(packno);
				memcpy(msgbuff_reply, (const void*) &kod, 2);
				memcpy(msgbuff_reply+2, (const void*) &packno, 2);
				memcpy(msgbuff_reply+4, (const void*) buffer, readlen);
				packno=ntohs(packno);
				if (netasciiflag && readlen<512){
					readlen=readlen-2;
				}
				while(retransmit <3){
					Sendto(newsocket, msgbuff_reply, 4+readlen, 0, (struct sockaddr *)&cliaddr, clilen, dflag);
					
					error = recvfrom(newsocket, msgbuff_recv, MAXBUFF, 0, (struct sockaddr *) &cliaddr, &clilen);
					if (error <0){
						//timeout
						retransmit++;
						continue;
					}else{
						break;
					}
				}
				if (retransmit>3){
					if (dflag){
						syslog(LOG_ALERT, "TFTP ERROR %d fb45558 no ack\n",errno);
						return -1;
					}else{
						err(-1,"TFTP ERROR %d fb45558 no ack\n",errno);
					}
				}
				// provjera da li je poslan ack
				uint16_t *kodpointer; 
				kodpointer = (uint16_t *)msgbuff_recv;
				*kodpointer = ntohs(*kodpointer);
				if (*kodpointer !=4){
					memset(msgbuff_reply, 0, 50);
					kod =  5;
					koderr = 4;
					kod = ntohs(kod);
					memcpy(msgbuff_reply, (const void*) &kod, 2);
					memcpy(msgbuff_reply+2, (const void*) &koderr, 2);
					char *poruka = "ack not receved";
					memcpy(msgbuff_reply+4, (const void*) poruka, strlen(poruka));
					Sendto(newsocket, msgbuff_reply, 4+strlen(poruka)+1, 0, (struct sockaddr *)&cliaddr, clilen, dflag);
					
					if (dflag){
						syslog(LOG_ALERT, "TFTP ERROR %d fb45558 ack not receved\n",koderr);
						continue;
					}else{
						fprintf(stderr,"TFTP ERROR %d fb45558 ack not receved\n",koderr);
						continue;
					}
				}
				
				/*
				uint32_t *packno_recv_temp;
				uint16_t *packno_recv;
				packno_recv_temp = (uint32_t *) msgbuff_recv;
				*packno_recv_temp = htonl(*packno_recv_temp);
				*packno_recv = ntohs(*packno_recv_temp);
				printf("%d\n", packno);
				printf("%d\n", *packno_recv);
				
				if (*packno_recv != packno){
					memset(msgbuff_reply, 0, 50);
					kod =  5;
					koderr = 4;
					kod = ntohs(kod);
					memcpy(msgbuff_reply, (const void*) &kod, 2);
					memcpy(msgbuff_reply+2, (const void*) &koderr, 2);
					char *poruka = "ack not correct";
					memcpy(msgbuff_reply+4, (const void*) poruka, strlen(poruka));
					error = sendto(newsocket, msgbuff_reply, 4+strlen(poruka)+1, 0, (struct sockaddr *)&cliaddr, clilen);
					if (error<0){
						if (dflag){
							syslog(LOG_ALERT, "TFTP ERROR %d fb45558 send\n",errno);
							return -1;
						}else{
							err(-1,"TFTP ERROR %d fb45558 send\n",errno);
						
						}
					}
					if (dflag){
						syslog(LOG_ALERT, "TFTP ERROR %d fb45558 ack not correct\n",koderr);
						continue;
					}else{
						fprintf(stderr,"TFTP ERROR %d fb45558 ack not correct\n",koderr);
						continue;
					}
				} */
				// ack uspješno primljen
				if (readlen <512){
					//zadnji paket poslan
					return 0;
				}
				fseek(f, packno*512-newline, SEEK_SET);
				retransmit =0;
				
				
			}
		}
		//roditelj
		
	}
	return 0;
}
