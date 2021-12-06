#include <stdio.h>          
#include <strings.h>
#include <string.h>         
#include <unistd.h>          
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <pthread.h> 
#include <stdlib.h>

#define PORT 6667
#define BACKLOG 1 
#define Max_client_num 10
#define MAXSIZE 1024

struct userinfo{
    char id[100];
    int invite_state;
    int play_state;
};

int fd_table[Max_client_num]={0};
char message[MAXSIZE];
struct userinfo users[100];
int OX_win_situation[8][3] = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}};

int find_fd (char *name){
    for (int i=0;i<100;i++)
        if (strcmp(name,users[i].id)==0)
            return i;
    return -1;
}

void gameover(int sender, int targetfd){

	users[sender].play_state = -1;
	users[targetfd].play_state = -1;
	users[sender].invite_state = -1;
	users[targetfd].invite_state = -1;

}

void message_handler(char *mes, int sender){
    int instruction = 0;
    sscanf (mes,"%d",&instruction);
    switch (instruction) {
        case 1: {               // addchange user name
            char name[100];
            char buf[MAXSIZE];
            sscanf (mes,"1 %s\n",name);
            if(find_fd(name) != -1){
                sprintf(buf, "2 %s 名稱已有人使用,請更改為其他名稱\n", name);
                send(sender, buf, strlen(buf), 0);
            }
            else{
                sscanf(&mes[2],"%s\n",users[sender].id);
                send(sender,"1",1,0);
                printf("1 change user name : %d's id => %s\n",sender,name);
            }
            break;
        }

        case 2:{                // show user table
            char buf[MAXSIZE],now_user_id[100];
            int p = sprintf(buf,"2 ");
            for (int i=0;i<100;i++){
                if (strcmp(users[i].id,"")!=0){
                    sscanf(users[i].id,"%s",now_user_id);
                    if(users[i].play_state != -1)
						p = sprintf(buf+p,"%s is now play with %s\n", now_user_id, users[users[i].play_state].id) + p;
					else
						p = sprintf(buf+p,"%s is idle\n", now_user_id) + p;
                }
			}
            printf("2 show:%s\n",buf);
			send(sender,buf,strlen(buf),0);
            break;
        }

        case 3:{                // invite
            char target_id[100];
            char buf[MAXSIZE];
            sscanf (mes,"3 %s\n",target_id);
            int targetfd = find_fd(target_id);
            if(users[sender].play_state != -1){
				sprintf(buf, "2 Don't invite someone when you are playing!\n");
				send(sender, buf, strlen(buf), 0);
			}
            else if(targetfd == -1){
				sprintf(buf, "2 player is not online\ninput help to get how to use command\n");
				send(sender, buf, strlen(buf), 0);
			}
            else if(targetfd == sender){
				sprintf(buf, "2 please don't duel with yourself\n");
				send(sender, buf, strlen(buf), 0);
			}
            else if(users[targetfd].play_state != -1){
				sprintf(buf, "2 %s is now playing with other, use command 2 to check who is idle\n", users[targetfd].id);
				send(sender, buf, strlen(buf), 0);
			}
            else{
				sprintf(buf, "4 %s want invite you. Do you accept?\n", users[sender].id);
            	users[sender].invite_state = targetfd;
				send(targetfd, buf, strlen(buf), 0);
            	printf("3 invite:%s to %s\n", users[sender].id, target_id);
			}
            break;
        }

        case 5:{                // agree(Y or y) or refuse(N or n)
            char state;
            char target_id[100];
			char buf[MAXSIZE];
            sscanf(mes, "5 %c %s\n", &state, target_id);
           	if(strcmp(target_id,"") == 0){
				sprintf(buf, "2 please enter the opponent's name!!\n");
				send(sender, buf, strlen(buf), 0);
				break;
			}
			int targetfd = find_fd(target_id);
			if(targetfd == -1){
				sprintf(buf, "2 player is not online\n");
				send(sender, buf, strlen(buf), 0);
				break;
			}
			if(users[targetfd].invite_state != sender){
				sprintf(buf, "2 %s is not want duel with you now\n", users[targetfd].id);
				send(sender, buf, strlen(buf), 0);
				break;
			}
			if (state=='Y' || state == 'y'){
				sprintf(buf, "6 0 %s %s\n",target_id ,users[sender].id);
				send(sender, buf, strlen(buf), 0);
				sprintf(buf, "6 1 %s %s\n",target_id ,users[sender].id);
				send(targetfd, buf, strlen(buf), 0);
                users[sender].play_state = targetfd;
                users[targetfd].play_state = sender;
				users[targetfd].invite_state = -1;
				users[sender].invite_state = -1;
				sprintf(buf, "2 remember! you are %s\n", users[sender].id);
				send(sender, buf, strlen(buf), 0);
				sprintf(buf, "2 remember! you are %s\n", users[targetfd].id);
                send(targetfd, buf, strlen(buf), 0);
				printf("6:\n");
			}
            else if(state == 'N' || state == 'n'){
				sprintf(buf, "2 You rejected %s 's inviation\n", users[targetfd].id);
				send(sender, buf, strlen(buf), 0);
				sprintf(buf, "2 %s is rejected yours inviation\n", users[sender].id);
				send(targetfd, buf, strlen(buf), 0);
				users[targetfd].invite_state = -1;
				users[sender].invite_state = -1;
			}
            break;
        }
        case 7:{   
			char board[9];
            char game_info[100];
            char buf[MAXSIZE];
            sscanf(mes, "7  %c %c %c %c %c %c %c %c %c",&board[0],&board[1],&board[2],&board[3],&board[4],&board[5],&board[6],&board[7],&board[8]); //read board in client

            for (int i=0;i<100;i++)
                game_info[i] = '\0';
            memset(buf,'\0',MAXSIZE);
            memset(game_info,'\0',sizeof(game_info));
            strcat(game_info, users[sender].id);

            /*--------------check win-----------------------------*/
            for (int i = 0; i < 8;i++)  {
                if (board[OX_win_situation[i][0]]==board[OX_win_situation[i][1]] && board[OX_win_situation[i][1]]==board[OX_win_situation[i][2]]) {     //one line has the same sign
                    if (board[OX_win_situation[i][0]]!='*') {    // sign is not '*'
                        strcat(game_info, " is winner!!!\n");
                        sprintf (buf,"8 2 %c %c %c %c %c %c %c %c %c %s\n",board[0],board[1],board[2],board[3],board[4],board[5],board[6],board[7],board[8],game_info);
                        printf ("7:%s",buf);
                        send(sender,buf,sizeof(buf),0);
                        send(users[sender].play_state,buf,sizeof(buf),0);
                        gameover(sender, users[sender].play_state);
                        return;
                    }
                }
            }

            /*--------------check draw-----------------------------*/
            memset(buf,'\0',MAXSIZE);
            memset(game_info,'\0',sizeof(game_info));
            for (int i = 0; i < 9;i++) {
                if (board[i]== '*')
                    break;
                if (i==8) {
                    strcat(game_info, "----------draw!----------\n");
                    sprintf (buf,"8 2 %c %c %c %c %c %c %c %c %c %s\n",board[0],board[1],board[2],board[3],board[4],board[5],board[6],board[7],board[8],game_info);
                    printf ("7:%s",buf);
                    send(sender,buf,sizeof(buf),0);
                    send(users[sender].play_state,buf,sizeof(buf),0);
                    gameover(sender, users[sender].play_state);
                    return;
                }
            }

            /*------------------no draw and no win, play continue--------------*/
            memset(buf,'\0',MAXSIZE);
            memset(game_info,'\0',sizeof(game_info));
            strcat(game_info, users[users[sender].play_state].id);
            strcat(game_info, "'s turn!\n");
            sprintf (buf,"8 1 %c %c %c %c %c %c %c %c %c %s\n",board[0],board[1],board[2],board[3],board[4],board[5],board[6],board[7],board[8],game_info);
            printf ("7:%s",buf);
            send(users[sender].play_state,buf,sizeof(buf),0);
            sprintf (buf,"8 0 %c %c %c %c %c %c %c %c %c %s\n",board[0],board[1],board[2],board[3],board[4],board[5],board[6],board[7],board[8],game_info);
			send(sender,buf,sizeof(buf),0);
            break;
        }
    }


}


void *pthread_service(void* sfd){
    int fd = *(int *)sfd;
    while(1)
    {
        int numbytes;
        int i;
        numbytes=recv(fd,message,MAXSIZE,0);
        printf ("\n\n%s\n\n",message);

        /*---close socket-------*/
        if(numbytes<=0){
            for(i=0;i<Max_client_num;i++){
                if(fd==fd_table[i]){
                    fd_table[i]=0;               
                }
            }
            memset(users[fd].id,'\0',sizeof(users[fd].id));
            users[fd].play_state = -1;
            break;
        }

        message_handler(message,fd);
        bzero(message,MAXSIZE);
    }
    close(fd);

}



int  main()  
{ 
    int listenfd, connectfd;    
    struct sockaddr_in server; 
    struct sockaddr_in client;      
    int sin_size; 
    sin_size=sizeof(struct sockaddr_in); 
    int client_num = 0;
    int fd;
    
    /*---------struct initialize---------*/
    for (int i=0;i<100;i++) { 
        for (int j=0;j<100;j++)
            users[i].id[j] ='\0';
		users[i].invite_state = -1;
        users[i].play_state = -1;
    }

    /*-------create socket server------------*/
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {   
        perror("Creating socket failed.");
        exit(1);
    }

    int opt = SO_REUSEADDR;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bzero(&server,sizeof(server));  

    server.sin_family=AF_INET; 
    server.sin_port=htons(PORT); 
    server.sin_addr.s_addr = inet_addr("127.0.0.1"); 

    if (bind(listenfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1){ 
        perror("Bind error.");
        exit(1); 
    }   
    if(listen(listenfd,BACKLOG) == -1){  
        perror("listen() error\n"); 
        exit(1); 
    } 
    printf("Waiting for client....\n");

    /*------------listen client---------------*/
    while(1){
        if ((fd = accept(listenfd,(struct sockaddr *)&client,&sin_size))==-1) {
            perror("accept() error\n"); 
            exit(1); 
        }
        if(client_num >= Max_client_num){
            printf("Client number is full, no more client is allowed\n");
            close(fd);
        }

        for(int i=0; i<Max_client_num; i++){
            if(fd_table[i]==0){
                fd_table[i]=fd;
                break;
            }

        }
        pthread_t tid;
        pthread_create(&tid,NULL,(void*)pthread_service,&fd);
        client_num += 1;
    }
    close(listenfd);            
}

