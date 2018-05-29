/**********************************************************************

PROGRAMAÇÃO DE SISTEMAS 2017/2018 - 2º Semestre

Authors: Diogo Rodrigues, Nº 84030
		 Leandro Almeida, Nº 84112

Last Updated on XXXXX

File name: clipboard_library.c

COMMENTS
	Threads of clipboard

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

// Definition of global variables

struct sockaddr_un local_addr;
struct sockaddr_un user_addr;
struct sockaddr_in server_addr;
struct sockaddr_in client_addr;

int pipefd[2];

_clipboard clipboard;
_clipboards_list * clipboards_list;

int sock_main_server;

pthread_mutex_t mux;
pthread_rwlock_t rwlock[MAX_REGIONS];
pthread_mutex_t mutex_child;
int flag_wait[MAX_REGIONS];
int waiting_list[MAX_REGIONS];
pthread_mutex_t mutex_wait[MAX_REGIONS];
pthread_cond_t data_cond[MAX_REGIONS] = PTHREAD_COND_INITIALIZER;


/**********************************************************************

new_thread_clipboard()

Arguments: user_fd - value returned by the unix_connection

Return: (void *)

Effects: communication with a user

Description: 
	This function do all the communication with a user

**********************************************************************/

void * new_thread_clipboard(void * _user_fd)
{
	int user_fd = *((int*)_user_fd);
	_message message;

	while(recv(user_fd, &message, sizeof(_message), 0) > 0)
	{
		if(message.action == COPY)
		{
			printf("Vou fazer copy!!!!\n");
			if(send_top(message, user_fd) == -1)
			{
				perror("Error in copy\n");
			}
		}
		else if(message.action == PASTE)
		{
			printf("Vou fazer paste!!!!\n");
			if(paste(message, user_fd) == -1)
			{
				perror("Error in paste\n");
			}
		}
		else if(message.action == WAIT)
		{
			printf("Vou fazer wait!!!!\n");
			if(wait(message, user_fd) == -1)
			{
				perror("Error in wait\n");
			}
		}
	}

	return 0;
}





/**********************************************************************

accept_remote_connections()

Arguments: none

Return: (void *)

Effects: clipboard is ready to inet connection

Description: 
	This function is called by the remote clipboard to accept inet connection 
	with news clipboards

**********************************************************************/

void * accept_remote_connections()
{
	socklen_t size_addr;
    pthread_t thread_id;

	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd == -1){
    	perror("socket: ");
    	exit(-1);
  	}

	server_addr.sin_family = AF_INET;
  	server_addr.sin_addr.s_addr= INADDR_ANY;

  	generate_random_port_and_bind(sock_fd);

  	listen(sock_fd, 2);

	while(1)
	{
    	size_addr = sizeof(struct sockaddr);

		int sock_fd_aux = accept(sock_fd, (struct sockaddr *) &client_addr, &size_addr);
		if(sock_fd_aux == -1) {
			perror("Accept:");
			exit(EXIT_FAILURE);
		}

        pthread_create(&thread_id, NULL, receiveDOWN_sendUP, &sock_fd_aux);
    }
}

void * receiveDOWN_sendUP(void * _socket_fd_son)
{
    int socket_fd_son= *((int*)_socket_fd_son);
    _message message;
    void * buffer_aux=NULL;
    int j;
    //---------------initialize my son------------------------
	//nao preciso de meter aqui agora um readlock????

    pthread_mutex_lock(&mutex_child);

    clipboards_list = create_new_son (clipboards_list, socket_fd_son);

    for(j=0; j<10; j++)
    {
        sleep(2);
        printf("região %d \n",j);
        message.length=clipboard.size[j];
        printf("A região %d tem size de %lu \n", j, clipboard.size[j]);
        message.region=j;
        printf("CHEGUEI AQUI!!\n");

       // write(socket_fd_son, &message, sizeof(_message));
        //aqui nao pode entretanto mudar????
        if(message.length != 0)
        {
        	printf("SOU O PAI E MANDEI ATUALIZAÃO PARA O FILHO\n");

            printf("o message.length abtes é %lu!!!!!\n", message.length);

            //read(socket_fd_son, &message, sizeof(_message));
            printf("o message.length depois é %lu!!!!!\n", message.length);

            paste(message, socket_fd_son);
       	}

    }
    pthread_mutex_unlock(&mutex_child);



    printf("O sock_fd_son é %d\n", socket_fd_son);
 
    //-----------------------------------------------------------
    //i'll receive information from my son, and when it happens i'll send to my dad
    while((read(socket_fd_son, &message, sizeof(_message)))>0)
    {
        printf("mal ini: message.region= %d , message.length=%lu \n",message.region,message.length);
        if(buffer_aux!=NULL)
        {
            free(buffer_aux);
        }
        buffer_aux= malloc(message.length);
        if(buffer_aux==NULL)
        {
            printf("mal inicializado \n");
            exit(-1);
        }
        read(socket_fd_son, buffer_aux,message.length);
        if(sock_main_server!=-1)
        {
            pthread_mutex_lock(&mux);
            write(sock_main_server,&message,sizeof(_message));
            write(sock_main_server,buffer_aux,message.length);
            pthread_mutex_unlock(&mux);

        }
            //if who receives is dad, he has to send the information across pipe
        else
        {
            pthread_mutex_lock(&mux);
            write(pipefd[WRITE],&message,sizeof(_message));
            write(pipefd[WRITE],buffer_aux,message.length);
            pthread_mutex_unlock(&mux);

        }



    }

    _clipboards_list * aux1= clipboards_list;
    while(aux1 !=NULL)
    {
        printf("xau--fd: %d \n",aux1->sock_fd);

        aux1=aux1->next;
    }


    clipboards_list=delete_son(clipboards_list, socket_fd_son);

_clipboards_list * aux = clipboards_list;
while(aux !=NULL)
{
    printf("oii fd: %d \n", aux->sock_fd);

    aux=aux->next;
}


		return 0;


}




void * receiveUP_sendDOWN(void * _fd)
{
    int fd = *((int *) _fd);
    _message message_aux;
    _clipboards_list * aux;


    while(read(fd, &message_aux, sizeof(_message)) > 0)
    {
        printf("recebi do pipe ou do socket normal \n");
        //MAIS UMA VEZ ISTO É A FUNCAO DE COPY - A NOSSA 'NOVA ' FUNCAO (QUE NAO EXISTE)
        //isto é que é o 'novo' copy- criar uma funcao para isto maybe! quando o pai escreve po filho
        if(message_aux.length==0)
        {
            continue;
        }

        //nao sei o que é suposto retornar ou nao pah...
        //se ele vier aqui é pq houve merda a copiar no read!! sinal que nao leu o read no copy
        if(!copy_to_clipboard(fd,message_aux))
            return 0;

		pthread_mutex_lock(&mutex_wait[message_aux.region]);

		if(waiting_list[message_aux.region]>0)
		{
			flag_wait[message_aux.region]=1;
			//for(int j=0; j<waiting_list[message_aux.region];j++)
			//pthread_cond_signal(&data_cond[message_aux.region]);
			pthread_cond_broadcast(&data_cond[message_aux.region]);
		}

		printf("eu vou bazaar do mutex 1 \n");
		pthread_mutex_unlock(&mutex_wait[message_aux.region]);
		printf("eu vou bazar do mutex 2 \n");

		aux = clipboards_list;

		pthread_mutex_lock(&mutex_child);
        while(aux!=NULL)
        {
            printf("tou a mandar o copy para os meus filhos \n");
            write(aux->sock_fd, &message_aux, sizeof(_message));
            write(aux->sock_fd, clipboard.matrix[message_aux.region], message_aux.length);
            aux=aux->next;
        }
        pthread_mutex_unlock(&mutex_child);

        for(int i = 0; i < 10; i++)
		{
			printf("Conteudo da região %d é %s\n", i, clipboard.matrix[i]);
		}
    }

    //only arrives here when dad dies

    sock_main_server=-1;
    receiveUP_sendDOWN(pipefd);

    		return 0;

}