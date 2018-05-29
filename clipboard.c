/**********************************************************************

PROGRAMAÇÃO DE SISTEMAS 2017/2018 - 2º Semestre

Authors: Diogo Rodrigues, Nº 84030
		 Leandro Almeida, Nº 84112

Last Updated on XXXXX

File name: clipboard.c

COMMENTS
	Main program that create a new clipboard

**********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "clipboard_library.h"


/**********************************************************************

main()

Arguments: argc - number of arguments
		   argv - table of pointers to the arguments string

Return: int

Effects: none

Description: 
	Main program of a clipboard

**********************************************************************/

int main(int argc, char *argv[]){

	pthread_t thread_id[3];
	socklen_t size_addr;
	char adress[100];

	initialize_clipboard();

	if(argc != 1 && argc != 4 && (strcmp(argv[1], "-c") != 0))
	{
		printf("invalid input parameters\n");
		exit(EXIT_FAILURE);
	}


	// If the clipboard run in connected mode
	if (argc==4)
	{
		strcpy(adress,argv[2]);
		sock_main_server = inet_connection_client(adress , atoi(argv[3]));

        pthread_create(&thread_id[0], NULL, receiveUP_sendDOWN, &sock_main_server);
    }
    else
    {
        pthread_create(&thread_id[0], NULL, receiveUP_sendDOWN, &pipefd[READ]);
    }

	pthread_create(&thread_id[1], NULL, accept_remote_connections, 0);

	int CLIPBOARD_SOCKET = unix_connection();

	while(1)
	{
		size_addr = sizeof(struct sockaddr);

		int user_fd = accept(CLIPBOARD_SOCKET, (struct sockaddr *) &user_addr, &size_addr);
		if(user_fd == -1) {
			perror("Accept:");
			exit(-1);
		}

		pthread_create(&thread_id[2], NULL, new_thread_clipboard, &user_fd);
	}	
}
