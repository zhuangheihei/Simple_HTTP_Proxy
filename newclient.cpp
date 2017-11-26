//system header
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <sstream>
#include <stdlib.h>

//network header
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>

using namespace std;

#define buffersize 70000

void err_sys(const char* x) 
{ 
    perror(x); 
    exit(1); 
}

int rec(int s, char *buf, int len, int timeout)
{
	fd_set fds;
	int res;
	struct timeval tv;
	

	// set up the file descriptor set
	FD_ZERO(&fds);
	FD_SET(s, &fds);

	// set up the struct timeval for the timeout
	tv.tv_sec = timeout;
	tv.tv_usec = 0;

	// wait until timeout or data received
	res = select(s+1, &fds, NULL, NULL, &tv);
	if (res == 0) return -2; // timeout!
	if (res == -1) return -1; // error

	// data must be here, so do a normal recv()
	return recv(s, buf, len, 0);
}


// // get sockaddr, IPv4 or IPv6:
// void *get_in_addr(struct sockaddr *sa)
// {
// 	if (sa->sa_family == AF_INET) {
// 		return &(((struct sockaddr_in*)sa)->sin_addr);
// 	}

// 	return &(((struct sockaddr_in6*)sa)->sin6_addr);
// }
// int c_web(string host){
//     int pyFd;
//     struct addrinfo hints, *res, *p;
//     int status;
    
//     memset(&hints, 0, sizeof(hints));
//     hints.ai_family = AF_INET; // AF_INET or AF_INET6 to force version
//     hints.ai_socktype = SOCK_STREAM;
    
//     if ((status = getaddrinfo(host.c_str(), "http", &hints, &res)) != 0) {
//         char error[100];
//         snprintf(error,sizeof(error),"getaddrinfo: %s\n", gai_strerror(status));
//         err_sys(error);
//     }
    
//     //printf("IP addresses for %s:\n\n", m.url->host.c_str());

//     for(p = res;p != NULL; p = p->ai_next) {
//         if(((pyFd= socket(p->ai_family, p->ai_socktype, p->ai_protocol))>=0 )&&
//                 (connect(pyFd, p->ai_addr, p->ai_addrlen)>=0))
//             break;
//     }
//     if(p==NULL) pyFd=-1;
//     freeaddrinfo(res);
//     return pyFd;
    
// }
int main(int argc, char *argv[])
{
	int pyFd;  
	struct addrinfo hints, *res, *p;
	int status;
	char * URL = NULL;
	char buffer[buffersize+1];
	char filename[500];
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	if (argc != 4) {
// 		fprintf(stderr,"usage: client hostname\n");
// 		exit(1);
        char error[100];
        snprintf(error,sizeof(error),"Please input host+port+url\n");
        err_sys(error);
	}
	
	if ((status = getaddrinfo(argv[1], argv[2], &hints, &res)) != 0) {
// 		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
// 		return 1;
        char error[100];
        snprintf(error,sizeof(error),"getaddrinfo: %s\n", gai_strerror(status));
        err_sys(error);
	}
	
		// loop through all the results and connect to the first we can
	for(p = res; p != NULL; p = p->ai_next) {
// 		if ((pyFd = socket(p->ai_family, p->ai_socktype,
// 				p->ai_protocol)) == -1) {
// 			perror("client: socket");
// 			continue;
// 		}
// 		if (connect(pyFd, p->ai_addr, p->ai_addrlen) == -1) {
// 			close(pyFd);
// 			perror("client: connect");
// 			continue;
// 		}
// 		break;
        if(((pyFd= socket(p->ai_family, p->ai_socktype, p->ai_protocol))>=0 )&&
                 (connect(pyFd, p->ai_addr, p->ai_addrlen)>=0))
            break;
	}

	if (p == NULL) {
// 		fprintf(stderr, "client: failed to connect\n");
// 		return 2;
        char error[100];
        snprintf(error,sizeof(error),"client: failed to connect\n");
        err_sys(error);
	}

	freeaddrinfo(res); // all done with this structure
    
    int length=strlen(argv[3]);
	if(strstr(argv[3], "http://") != NULL){
		URL = (char *)calloc(length,1);
		strcpy(URL, argv[3]);
		cout << endl << URL<<endl;
	}else{
		URL = (char *)calloc(length+7,1);
		strcpy(URL, "http://");
		strcat(URL, argv[3]);
	}
	

	send(pyFd, URL, strlen(URL), 0);

	
	int ff = 0;
	int recstatus = 0;
	FILE *newfile;

	int mark = 1;
	while(mark)
	{
		(recstatus = rec(pyFd, buffer, buffersize, 2));
		cout <<"Recieved Status: " << recstatus <<endl;
		if(recstatus<=0){
			mark = 0;
			cout<<"End of a transmission"<<endl;
			break;
		}
        

		if (strstr(buffer, "404 Not Found")){
			cout << "404 Not Found!\n";
			break;
		}

		if (!ff){
			cout<<buffer<<endl;

			ff =1;
			strcat(filename, strrchr(URL, '/')+1);
			if (strstr(URL, ".txt")){
				newfile = fopen(filename, "w");
				if (newfile == NULL){
				    char error[100];
                    snprintf(error,sizeof(error),"Failed to open %s!\n",
							filename);
                    err_sys(error);
				// 	fprintf(stderr, "Open file failed %s!\n",
				// 			filename);
				// 	exit(1);
				}
				char *pointer = strstr(buffer, "\r\n\r\n") ;
				if(pointer){
					fputs(pointer+4,newfile);
				}else{
					cout<<"Empty message"<<endl;
				}
			}else if(strstr(URL, ".jpg")){
				newfile = fopen(filename, "wb");
				if (newfile == NULL){
				// 	fprintf(stderr, "Can't open output file %s!\n",
				// 			filename);
				// 	exit(1);
				    char error[100];
                    snprintf(error,sizeof(error),"Failed to open %s!\n",
							filename);
                    err_sys(error);
				}
				fwrite(strstr(buffer, "\r\n\r\n")+4, 1, recstatus - (strstr(buffer, "\r\n\r\n") + 4 - buffer), newfile);
			}
		}else{
			if(strstr(filename, ".txt")){
				fputs(buffer,newfile);
			}else if(strstr(filename, ".jpg")){
				fwrite(buffer, 1, recstatus, newfile);
			}
		}
		memset(buffer,0,buffersize);
	}
	cout<<"Close file."<<endl;
	if (recstatus == 0 && newfile != NULL){
		fclose(newfile);
	}

	free(URL);
	close(pyFd);

	return 0;
}