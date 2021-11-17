#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define SERV_PORT 80
#define BUFSIZE 16384
			
char webpage[] =
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"﻿<!DOCTYPE html>"
"<html><head><meta charset=UTF-8><title>cys107u</title></head>\r\n"
"<body><form method=\"post\" enctype=\"multipart/form-data\">\r\n"
"<input type=\"file\" name=\"select_file\" size=\'10\'><input type=\"submit\" name=\"Upload\" style=\"\"></form>\r\n"
"<img src=\"https://www.naguide.com/wp-content/uploads/2021/07/Goose-Goose-Duck-Nexus-Colony-Kills-13.jpg\"></body></html>\r\n";

void upload_file(char *buffer,int fd,int taken){

	int buflen;
	//check upload file
	char *pos; 
	pos = strstr(buffer,"filename=\"");
	if (pos == 0)
		return;//no upload

	
	char filename[BUFSIZE+1],path[BUFSIZE+1];	
	/*get file name*/
	pos+=10; //  "filename=\"" has 10 words
	sprintf(path,"./upload/");
	long i=9; // "./upload/" has 9 words
	char *content_end=strstr(pos,"\""),*j;
	
	//cat filename behind path
	for(j=pos;j!=content_end;i++,j++){
		path[i] = *j;
	}
	path[i]=0;

	/*locate position to start */
	pos = strstr(pos,"\n");
	pos = strstr(pos+1,"\n");
	pos = strstr(pos+1,"\n");
	pos++;
	
	/*locate content_end position*/
	content_end = pos;
	content_end = strstr(content_end, "\r\n----");
	buflen = strlen(buffer);
	if(content_end != NULL)	
		*content_end = '\0';
	else
		content_end = &buffer[buflen-1];
	
	/*write flie to path*/
	int download_fd = open(path,O_CREAT|O_WRONLY|O_TRUNC|O_SYNC,S_IRWXO|S_IRWXU|S_IRWXG);
	if(download_fd == -1)
		write(fd,"Faild to download file.\n",19);
	write(download_fd,pos,(content_end-pos));
	while((taken = read(fd, buffer, BUFSIZE)) > 0){
		printf("In while\n");
		buflen = strlen(buffer);
		content_end = strstr(content_end, "-----");
		if(content_end != NULL){
			*content_end = '\0';
			write(download_fd, buffer, (content_end - buffer));
		}else{
			write(download_fd, buffer, taken);
		}
		if(content_end != NULL)
			break;
	}
	close(download_fd);
	printf("////close fd////\n");
	return;
}

int main(int argc, char*argv[])
{
	struct sockaddr_in server_addr, client_addr;
	socklen_t sin_len = sizeof(client_addr);
	int fd_server, fd_client;
	char buf[BUFSIZE];
	int fdimg;
	int on = 1;
	long ret;

	fd_server = socket(AF_INET,SOCK_STREAM,0);
	if(fd_server<0)
	{
		perror("socket");
		exit(1);
	} 

	setsockopt(fd_server,SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(SERV_PORT);

	if(bind(fd_server, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1)
	{
		perror("bind");
		close(fd_server);
		exit(1);
	}


	if(listen(fd_server,10)==-1)
	{
		perror("listen");
		close(fd_server);
		exit(1);
	}

	while(1)
	{
		fd_client = accept(fd_server, (struct sockaddr *) &client_addr, &sin_len);

		if(fd_client == -1)
		{
			perror("Connection failed!\n");
			continue;
		}

		printf("Got client connection!\n");

		if(!fork())
		{
			/*child*/
			close(fd_server);
			memset(buf,0,BUFSIZE);
			read(fd_client,buf,BUFSIZE-1);

			printf("%s\n",buf);


			if(strncmp(buf,"GET ",4)==0||strncmp(buf,"get ",4)==0)
			{
				write(fd_client, webpage, sizeof(webpage)-1);
			}
			else if(strncmp(buf,"POST ",5)==0||strncmp(buf,"post ",5)==0){
				
				write(fd_client, webpage, sizeof(webpage)-1);
				ret = read(fd_client,buf,BUFSIZE);
				printf("77777777777777777777777777777777777777777\n");
				printf("%s\n",buf);
				printf("77777777777777777777777777777777777777777\n");
				upload_file(buf,fd_client,ret);
       				write(fd_client, webpage, sizeof(webpage)-1);

				while ((ret=read(webpage, buf, BUFSIZE))>0){
           				write(fd_client,buf,ret);
        			}
				
			}
			else
				exit(3);
				
		
			close(fd_client);
			printf("closing~\n");
			exit(0);
		}
		/*parent*/
		close(fd_client);
	
	}
	return 0;


}
