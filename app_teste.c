#include "clipboard.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h> 
#include <unistd.h>
#include <string.h>
#include <signal.h>

int main()
{
	char data[100];
	int region=0;
	int aux = 0;
	int action = 3;
	char aux_text[50],aux_sent[100];


	int flag_copy, flag_paste,flag_wait;

	int sock_fd = clipboard_connect("./");
		
	if(sock_fd == -1)
	{
		printf("Error opening in fifo\n");
		exit(-1);
	}

	while(1)
	{
		printf("cheguei aqui!!! aux=%d\n", aux);
		//ASK IF THE USER WANT TO COPY OR PAST 
		while((aux != 1) || ((action != COPY) && (action != PASTE) && action!=WAIT))
		{
			printf("Do you wanna COPY (0) or PASTE (1) or WAIT(2)? \n");
			fgets(aux_text, 50, stdin);
			aux = sscanf(aux_text, "%d", &action);
		}

		//COPY STUFF
		if(action == COPY)
		{
			printf("Write what you want\n");
			fgets(aux_sent, 50, stdin);

			aux = 3;
			aux_sent[strlen(aux_sent)-1]='\0';
			
			printf("a palavra é (mal começa) %s \n",aux_sent);

			while((region > 9|| region < 0) || aux != 1)
			{
				printf("Chose a region \n");
				fgets(aux_text, 50, stdin);
				aux = sscanf(aux_text, "%d", &region);
			}

			aux = 3;

			//não percebo o ultimo parâmetro pois não sei o que é o count no 'clipboard_copy'
			//if it is returned 0 bytes
			if((flag_copy = clipboard_copy(sock_fd, region, aux_sent, sizeof(char)*(strlen(aux_sent)+1)))==0)
			{
				printf("ERROR IN COPY\n");	
				//é suposto sair??????????????
				exit(-1);
			}
						printf("a palavra é (dps do copy) %s \n",aux_sent);
		}
		//PASTE STUFF
		else if(action == PASTE)
		{
			aux = 3;

			while(region > 9 ||region < 0 || aux != 1)
			{
				printf("Chose a region that you wanna paste \n");
				fgets(aux_text, 50, stdin);
				aux = sscanf(aux_text, "%d", &region);
			}

			aux = 3;

			//parametros aldrabados, temos d saber o tamanho da 'data' e alocar 'data' dinamicamente??
		// é só estupido aqui mandar o strlen(data) como parametro pois o 'data' que esta e o anterior que fizémos paste
			if((flag_paste = clipboard_paste(sock_fd, region, data, 5*sizeof(char))) == 0)
			{
					printf("ERROR IN PASTE\n");	
					//é suposto sair??????????????
			}
			else
				printf("DATA RECEIVED: %s\n",data);

		}
		else if(action==WAIT)
        {
			aux = 3;

			while(region > 9 ||region < 0 || aux != 1)
			{
				printf("Chose a region that you wanna wait \n");
				fgets(aux_text, 50, stdin);
				aux = sscanf(aux_text, "%d", &region);
			}

			aux = 3;

			if((flag_wait = clipboard_wait(sock_fd, region, data, 100*sizeof(char))) == 0)
			{
				printf("ERROR IN PASTE\n");
				//é suposto sair??????????????
			}
			else
				printf("DATA RECEIVED: %s\n",data);


        }
		
		

	}
		
	exit(0);
}
