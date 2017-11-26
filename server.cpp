#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include "cache.h"

using namespace std;

#define MAXDATASIZE 520
#define MAXRETSIZE 70000
#define CACHE_SIZE 10    //maximum capacity in cache
#define BACKLOG 10     //how many pending connections queue will hold

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

//cache path is the 
int send_from_cache(char *cachepath, int sockid){ //int send_from_cache(char *cachepath, char *buf, int sockid){
	//---------Cache Hit!------------
	// the mycache.get will automatically refresh the cache
	// send Reponse to client
	//mycache.PrintCache();
	
	cout << "URL file found: " << cachepath << endl;
	FILE *rfile;
	//char* rbuf;
	char rbuf[MAXRETSIZE];
	int sent;
	if(strstr(cachepath, ".txt")){
		//rbuf = (char *) calloc(MAXRETSIZE, 1);
		strcpy(rbuf, "\r\n\r\n");
		char ch;
		rfile = fopen(cachepath, "r");
		if (rfile == NULL) {
			fprintf(stderr, "Can't open URL file %s!\n",cachepath);
			exit(1);
		}
		while((ch = fgetc(rfile)) != EOF){
			strncat(rbuf, &ch, 1);
		}
		//txtfile = fopen(cachepath, "r");
		fgets(rbuf, MAXRETSIZE + 1, rfile);
		if(rfile != NULL)
			fclose(rfile);
		sent = send(sockid, rbuf, strlen(rbuf), 0);
		cout << sent << " bytes of data sent to client from cache." << endl;
		
	}else if(strstr(cachepath, ".jpg")){
		rfile = fopen(cachepath, "rb");

		// obtain file size:
		long lsize;
		fseek(rfile, 0, SEEK_END);
		lsize = ftell(rfile);
		rewind(rfile);

		//rbuf = (char *) calloc(lsize+4, 1);
		strcpy(rbuf, "\r\n\r\n");
		fread(rbuf+4, 1, lsize, rfile);
		if(rfile != NULL)
			fclose(rfile);
		sent = send(sockid, rbuf, lsize+4, 0);
	}

// if(rbuf != NULL)
// 	free(rbuf);
//memset(buf, 0, MAXDATASIZE);
return 0;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


// compare "exptime" with current time, 1-exptime has been passed, 0-not passed
bool timecmp(char * exptime){

	//current time
	time_t rawtime;
	struct tm * ptm;
	time(&rawtime);
	ptm = gmtime(&rawtime);
	//cout << ptm->tm_wday << ", " << ptm->tm_mday << " " << ptm->tm_mon<< " " << ptm->tm_year << " " << ptm->tm_hour << ":"<< ptm->tm_min << ":" << ptm->tm_sec << " " << endl;
	

	char * wdays;
	wdays = (char*) calloc(21, 1);
	strcpy(wdays, "SunMonTueWedThuFriSat");
	char * months;
	months = (char*) calloc(36, 1);
	strcpy(months, "JanFebMarAprMayJunJulAugSepOctNovDec");

	//check time zone.
	if (!strstr(exptime, "GMT")){
		cout << "Error: Not GMT Time.!\n";
		return false;
	}

	struct tm * exptm;
	exptm = (struct tm *)calloc(1, sizeof(struct tm));
	char *tmptime;
	tmptime = (char *)calloc(4, 1);

	// days since Sunday: 0-6
	strncpy(tmptime+1, exptime, 3);
	exptm->tm_wday = (strstr(wdays, tmptime+1) - wdays)/3.0;

	// day of the month: 1-31
	strncpy(tmptime+2, strstr(exptime, ",") + 2, 2);
	exptm->tm_mday = atoi(tmptime+2);

	// months since January: 0-11
	strncpy(tmptime+1, strstr(exptime, ",") + 5, 3);
	exptm->tm_mon = (strstr(months, tmptime+1) - months)/3.0;

	// years since 1900
	strncpy(tmptime, strstr(exptime, ",") + 9, 4);
	exptm->tm_year = atoi(tmptime) - 1900;

	// hours since midnight: 0-23
	strncpy(tmptime+2, strstr(exptime, ":") - 2, 2);
	exptm->tm_hour = atoi(tmptime+2);

	// minutes after the hour: 0-59
	strncpy(tmptime+2, strstr(exptime, ":") + 1, 2);
	exptm->tm_min = atoi(tmptime+2);

	// seconds after the minute: 0-60
	strncpy(tmptime+2, strstr(exptime, ":") + 4, 2);
	exptm->tm_sec = atoi(tmptime+2);

//	 expire time in cache
	cout << exptm->tm_wday << ", " << exptm->tm_mday << " " << exptm->tm_mon << " " << exptm->tm_year<< " " << exptm->tm_hour << ":"
			<< exptm->tm_min << ":" << exptm->tm_sec << " " << endl;

	if(tmptime != NULL)
		free(tmptime);
	if(wdays != NULL)
		free(wdays);
	if(months != NULL)
		free(months);
	
	cout << "timecmp works.." << endl;
	
	if (exptm->tm_year > ptm->tm_year)
		return false;
	else if((exptm->tm_year < ptm->tm_year))
		return true;
	else
		if(exptm->tm_mon > ptm->tm_mon)
			return false;
		else if(exptm->tm_mon < ptm->tm_mon)
			return true;
		else
			if(exptm->tm_mday > ptm->tm_mday)
				return false;
			else if(exptm->tm_mday < ptm->tm_mday)
				return true;
			else
				if(exptm->tm_hour > ptm->tm_hour)
					return false;
				else if(exptm->tm_hour < ptm->tm_hour)
					return true;
				else
					if(exptm->tm_min > ptm->tm_min)
						return false;
					else if(exptm->tm_min < ptm->tm_min)
						return true;
					else
						if(exptm->tm_sec > ptm->tm_sec)
							return false;// not expired
						else return true;
	return true;  //expired

}




int main(int argc, char *argv[])
{
	if (argc != 3) {
		cout << "Number of input arguments are not equal to 3." <<endl;
		exit(1);
	}

	struct addrinfo hints, *servinfo, *p;
	struct sigaction sa;
	int yes=1;
	int rv;
	char *center;

	int web_socket;
	int server_socket;// listening socket descriptor
	int client_socket;   // New client sockfd
	struct sockaddr_storage their_addr; // client address

	fd_set master;    // master file descriptor list
	fd_set temp_fds_list;  // temp file descriptor list for select()

	socklen_t addrlen;
	int fdmax;        // maximum sockfd number
	int i;

	FD_ZERO(&master);    // clear the master and temp sets
	FD_ZERO(&temp_fds_list);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
	if ((rv = getaddrinfo(NULL, argv[2], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next){
		if ((server_socket = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1){
			perror("server: socket");
			continue;
		}
		
		if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)	{
			perror("setsockopt");
			exit(1);
		}

		if (bind(server_socket, p->ai_addr, p->ai_addrlen) == -1) {
			close(server_socket);
			perror("server: bind");
			continue;
		}
		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}
	cout << "Server socket established.." << endl;
	cout << "Server binded.." << endl;
	
	freeaddrinfo(servinfo); // all done with this structure

	if (listen(server_socket, BACKLOG) == -1)	{
		perror("listen");
		exit(1);
	}
	
	cout << "Listening to clients.." << endl;
	cout << "---------------------------------------------" << endl;
	
	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	// add the listener to the master set
	FD_SET(server_socket, &master);

	// keep track of the biggest file descriptor
	fdmax = server_socket; // so far, it's this one

	Cache mycache(CACHE_SIZE);
	char buf[MAXDATASIZE];
	char GETbuf[MAXDATASIZE];
	char RETbuf[MAXRETSIZE+1];
	char ip4[15];
	char path[PATH_SIZE];
	char exp[MAXTIMESIZE];
	FILE *txtfile;
	bool is_exp = false;
	// loop for handling clients
	while(1)
	{
		temp_fds_list = master; // copy it
		if (select(fdmax+1, &temp_fds_list, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(4);
		}

		// run through the existing connections looking for data to read
		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &temp_fds_list)) { // we got one!!
				if (i == server_socket) { // we have new connections here!
					// handle new connections
					addrlen = sizeof their_addr;
					client_socket = accept(server_socket, (struct sockaddr *)&their_addr, &addrlen);
					if (client_socket == -1) {
						perror("accept");
					} else {
						FD_SET(client_socket, &master); // add to master set
						if (client_socket > fdmax) {    // keep track of the max
							fdmax = client_socket;
						}
					}
				}
				else
				{
					int recv_bytes =0 ;
					memset(buf,0,MAXDATASIZE);
					recv_bytes = recv(i, buf, MAXDATASIZE, 0);

					// handle data from a client
					if (recv_bytes <= 0) {
						// got error or connection closed by client
						if (recv_bytes == 0) {
							// connection closed
							cout << "client disconnected."<<endl;
							cout << "---------------------------------------------" <<endl;
						} else {
							perror("recv");
						}
						close(i); // bye!
						FD_CLR(i, &master); // remove from master set
						break;
					}

					cout << "URL requested by client is: " << buf << endl;

					string URLstr(buf); //buf does not contains .txt!!
					char* cachepath;
					cachepath = (char*) calloc(PATH_SIZE, 1);
					//mycache.PrintCache();
					cachepath = mycache.get(URLstr);
					//cout << "cachepath is: " << cachepath << endl;
					
					// check expire or not
					char * cacheexp;
					
					cacheexp = (char*) calloc(MAXTIMESIZE, 1);

					
					cacheexp = mycache.getexpire(URLstr);

					if (cachepath != NULL){// when file is in the cache

						is_exp = timecmp(cacheexp); 
						
						cout << "cacheexp is: "<< cacheexp <<endl;

						cout << "is exp: "<< is_exp <<endl;
					}
					
					
					//check if url is expired, and do corresponding operations 
					if(cachepath != NULL && is_exp == false){ //when URL is not expired
						send_from_cache(cachepath, i); //buf is 
					}
					else{ //URL is expired or not found
					
						cout << "No such a URL in cache or it is already expired." << endl;
						center = strstr(buf+7, "/");

						if(center){
							int size = center-(buf+7);
							strncpy(ip4, buf+7, size);
							ip4[size] ='\0';
							cout<<"Web URL is:  "<<ip4<<endl;
						}else{
							strcpy(ip4, buf+7);
							ip4[strlen(ip4)] ='\0';
						}

						hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
						hints.ai_socktype = SOCK_STREAM;

						if ((rv = getaddrinfo(ip4, "http", &hints, &servinfo)) != 0) {
							fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
							exit(1);
						}

						// loop through all the results and connect to the first we can
						for(p = servinfo; p != NULL; p = p->ai_next){
							if ((web_socket = socket(p->ai_family, p->ai_socktype,
									p->ai_protocol)) == -1){
								perror("socket");
								continue;
							}
							if (connect(web_socket, p->ai_addr, p->ai_addrlen) == -1) {
								close(web_socket);
								perror("connect");
								continue;
							}
							break; // if we get here, we must have connected successfully
						}

						freeaddrinfo(servinfo);

						if(is_exp == true){ // if URL expired
						
						// concate all message and send Get
							memset(GETbuf, 0, MAXDATASIZE);
							strcpy(GETbuf, "GET ");
							strcat(GETbuf, buf);
							strcat(GETbuf, " HTTP/1.0\n");
							strcat(GETbuf, "If-Modified-Since: ");
							strcat(GETbuf, cacheexp);
							strcat(GETbuf, "\r\n\r\n");
							// cout <<"if it expires then GETbuf should be:" << GETbuf <<endl;
							// cout << "cacheexp should be: " << cacheexp <<endl;

						}
						else{
							memset(GETbuf, 0, MAXDATASIZE);
							strcpy(GETbuf, "GET ");
							strcat(GETbuf, buf);
							strcat(GETbuf, " HTTP/1.0\r\n\r\n");
							// cout << "if not expired the GETbuf should be:" << GETbuf << endl;
							
						}
						
						
						recv_bytes = send(web_socket, GETbuf, sizeof(GETbuf), 0);
						//cout << GETbuf <<endl;
						cout << "Request sent to " << buf << endl;
						
						//recv data from web server until recv == 0
						memset(RETbuf, 0, MAXRETSIZE);
						recv_bytes = MAXRETSIZE; //recv data from web server
						int r_flag = 1; //check is data received from web 
						int file_flag = 0;
						
						
						while(r_flag){ //receive from web
							
							int sent;
							
							recv_bytes = recv(web_socket, RETbuf, MAXRETSIZE*sizeof(char), 0);

							if (strstr(RETbuf, "404 Not Found")){
								cout << "404 Not Found\n";
								cout << "---------------------------------------------" << endl;
								sent = send(i, "404 Not Found", 13, 0);
								close(web_socket);
								r_flag = 0; //no data from web
								break;
							}

							if(is_exp == true && file_flag == 0)
							{
								cout<<"checking if URL expired.." <<endl;
								
								if(strstr(RETbuf, "304 Not Modified"))
								{
									cout <<"= 304 Not Modified"<<endl;

									send_from_cache(cachepath,i);
									
									break;
								}
								if(strstr(RETbuf, "200 OK"))
								{
									cout <<"= 200 OK"<<endl;
								}
							}
							
							if (recv_bytes == 0){
								cout<<"Transmission completed."<<endl;
								cout <<"---------------------------------------------" << endl;
								close(web_socket);
								r_flag = 0; 
								break;
							}
							
							//write the data from web to cache folder, and read from it.
							if(!file_flag){  // URL appears first time 
								
								file_flag = 1;

								struct stat st = {0};

								if (stat("cache", &st) == -1) {
									mkdir("cache", 0700);
								}
								cout<<"Getting data from the web server.." << endl;
								strcpy(path, "cache/");
								strcat(path, strrchr(buf, '/')+1); // initialize the path for recv file
								strcat(path, ".txt");
								strncpy(exp, strstr(RETbuf, "Expires: ") + 9, 29);
								
								cout << "Path****************************************" << endl; 
								cout << path << endl;
								cout << "********************************************" << endl;
								//cout << "WEb content*********************************" << endl; 
								//cout<< RETbuf <<endl;
								
								// add this entry in the cache
								//Entry ent(URLstr, path);
								
								Entry *ent = new Entry();
								ent->url = URLstr;
								strcpy(ent->path, path);
								strcpy(ent->expire, exp);
								mycache.AddEnt(ent);
								mycache.PrintCache();

								if(strstr(path, ".txt")){
									//begin to write into file
									//cout << "RETbuf: \n" << RETbuf <<endl;
									txtfile = fopen(path, "w");
									if (txtfile == NULL){
										fprintf(stderr, "Can't open output file %s!\n",	path);
										exit(1);
									}
									fputs(strstr(RETbuf, "\r\n\r\n")+4, txtfile);
									cout << "Cache saved.." << endl;
								}else if(strstr(path, ".jpg")){
									txtfile = fopen(path, "wb");
									if (txtfile == NULL){
										fprintf(stderr, "Can't open output file %s!\n",	path);
										exit(1);
									}
									fwrite(strstr(RETbuf, "\r\n\r\n")+4, 1, recv_bytes - (strstr(RETbuf, "\r\n\r\n") + 4 - RETbuf), txtfile);
								}
							}else{ //URL is in cache, then read from cache
								if(strstr(path, ".txt")){
									//txtfile = fopen(path, "r");
									//fgets(RETbuf, MAXRETSIZE + 1, txtfile);
									fwrite(RETbuf, 1, recv_bytes, txtfile);
								}else if(strstr(path, ".jpg")){
									fwrite(RETbuf, 1, recv_bytes, txtfile);
								}
								//send_from_cache(cachepath, i);
							}
							// send Reponse to client
							sent = send(i, RETbuf, recv_bytes, 0);
						}
						if(recv_bytes == 0 && txtfile != NULL){
							fclose(txtfile);
						}
						memset(RETbuf, 0, MAXRETSIZE);
						memset(buf, 0, MAXDATASIZE);
						memset(GETbuf, 0, MAXDATASIZE);
						close(i); // bye!
						FD_CLR(i, &master); // remove from master set
						break;
					}
				} // END handle data from client
			} // END got new incoming connection
		} // END looping through file descriptors
	} // END while(1)

	return 0;
}