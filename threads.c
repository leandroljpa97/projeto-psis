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

pthread_mutex_t mux_sendUP;
pthread_rwlock_t rwlock[MAX_REGIONS];
pthread_mutex_t mutex_child;
pthread_mutex_t mutex_wait[MAX_REGIONS];
pthread_cond_t data_cond[MAX_REGIONS] = PTHREAD_COND_INITIALIZER;


/**********************************************************************

user_communication
Arguments: user_fd - value returned by the unix_connection

Return: (void *)

Effects: communication with a user

Description: 
	This function do all the communication with a user

**********************************************************************/

void * user_communication(void * _user_fd)
{
	int user_fd = *((int*)_user_fd);
	_message message;

	while(recv(user_fd, &message, sizeof(_message), 0) > 0)
	{
	    if(!verification_library(message))
            break;


		if(message.action == COPY)
		{
			if(send_top(message, user_fd) == -1)
			{
				printf("Error in copy- communication with app\n");
                break;
			}
		}
		else if(message.action == PASTE)
		{
			if(paste(message, user_fd) == -1)
			{
				printf("Error in paste- communication with app\n");
                break;
			}
		}
		else if(message.action == WAIT)
		{
			if(wait(message, user_fd) == -1)
			{
				printf("Error in wait- comunication with app\n");
                break;
			}
		}
		else break;
	}

    clipboard_close(user_fd);
    return NULL;
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
	if (sock_fd == -1)
        error_confirmation("Error in socket_inet:");

    server_addr.sin_family = AF_INET;
  	server_addr.sin_addr.s_addr= INADDR_ANY;

  	generate_random_port_and_bind(sock_fd);

  	listen(sock_fd, 2);

	while(1)
	{
    	size_addr = sizeof(struct sockaddr);

		int sock_fd_aux = accept(sock_fd, (struct sockaddr *) &client_addr, &size_addr);
		if(sock_fd_aux == -1)
            error_confirmation("Error in Accept:");


        pthread_create(&thread_id, NULL, receiveDOWN_sendUP, &sock_fd_aux);
        pthread_detach(thread_id);

    }
    return NULL;
}

/**********************************************************************

receiveDown_sendUP()

Arguments: _socket_fd_son

Return: (void *)


Description:
	This thread is always ready to read from one son and send the information to the parent
    there are one thread "receiveDOWN_send_UP per son
**********************************************************************/

void * receiveDOWN_sendUP(void * _socket_fd_son)
{
    int socket_fd_son= *((int*)_socket_fd_son);
    _message message;
    void * buffer_aux=NULL;
    int j;

    // Initialize my son
    if(pthread_mutex_lock(&mutex_child)!=0)
        error_confirmation("Error in lock mutex child in function receiveDown_sendUp");

    if((clipboards_list = create_new_son (clipboards_list, socket_fd_son)) == NULL)
    {
        if(pthread_mutex_unlock(&mutex_child)!=0)
            error_confirmation("Error in unlock mutex child in function receiveDown_sendUp");

        clipboard_close(socket_fd_son);
        return NULL;
    }

    //send all my content to my son
    for(j = 0; j < MAX_REGIONS; j++)
    {
        message.length = clipboard.size[j];
        message.region = j;

        if(message.length != 0)
        {
            if(paste(message, socket_fd_son) == -1)
            {
                clipboards_list = delete_son(clipboards_list, socket_fd_son);
                if(pthread_mutex_unlock(&mutex_child)!=0)
                    error_confirmation("Error in unlock mutex child in function receiveDown_sendUp");

                return NULL;
            }
       	}
    }

    if(pthread_mutex_unlock(&mutex_child)!=0)
        error_confirmation("Error in unlock mutex child in function receiveDown_sendUp");


    //-----------------------------------------------------------
    //i'll receive information from my son, and when it happens i'll send to my dad
    while((read(socket_fd_son, &message, sizeof(_message)))>0)
    {
        buffer_aux = malloc(message.length);
        if(buffer_aux == NULL)
            error_confirmation("Alocation error:");

        if(read(socket_fd_son, buffer_aux, message.length) != message.length)
        {
            free(buffer_aux);
            break;
        }

        if(sock_main_server != SINGLE_MODE)
        {
            if(pthread_mutex_lock(&mux_sendUP)!=0)
                error_confirmation("Error in lock mutex_sendUP in function receiveDown_sendUp");

            if(write(sock_main_server, &message, sizeof(_message)) != sizeof(_message))
            {
                if(pthread_mutex_unlock(&mux_sendUP)!=0)
                    error_confirmation("Error in unlock mutex_sendUP in function receiveDown_sendUp");

                free(buffer_aux);
                continue;
            }
            if(write(sock_main_server, buffer_aux, message.length) != message.length)
            {
                if(pthread_mutex_unlock(&mux_sendUP)!=0)
                    error_confirmation("Error in unlock mutex child in function receiveDown_sendUp");

                free(buffer_aux);
                continue;
            }
            if(pthread_mutex_unlock(&mux_sendUP)!=0)
                error_confirmation("Error in unlock mutex_sendUP in function receiveDown_sendUp");


        }
            //if who receives is dad, he has to send the information across pipe
        else
        {
            if(pthread_mutex_lock(&mux_sendUP)!=0)
                error_confirmation("Error in lock mutex_sendUP in function receiveDown_sendUp");

            if(write(pipefd[WRITE], &message, sizeof(_message)) != sizeof(_message))
            {
                free(buffer_aux);
                error_confirmation("Error in Pipe:");
            }
            if(write(pipefd[WRITE], buffer_aux, message.length) != message.length)
            {
                free(buffer_aux);
                error_confirmation("Error in Pipe:");
            }
            if(pthread_mutex_unlock(&mux_sendUP)!=0)
                error_confirmation("Error in unlock mutex child in function receiveDown_sendUp");

        }

        free(buffer_aux);
    }


    if(pthread_mutex_lock(&mutex_child)!=0)
        error_confirmation("Error in lock mutex child in function receiveDown_sendUp");

    clipboards_list = delete_son(clipboards_list, socket_fd_son);
    if(pthread_mutex_unlock(&mutex_child)!=0)
        error_confirmation("Error in unlock mutex child in function receiveDown_sendUp");

    return NULL;
}

/**********************************************************************

receiveUP_sendDown()

Arguments: _socket_fd_son

Return: (void *)


Description:
	This thread is always read the information from the parent and send to all sons
    there are one thread "receiveDOWN_send_UP per son
**********************************************************************/

void * receiveUP_sendDOWN(void * _fd)
{
    int fd = *((int *) _fd);
    _message message_aux;
    _clipboards_list * aux;

    while(read(fd, &message_aux, sizeof(_message)) > 0)
    {
        //when the length is zero, don't read the content to alocate
        if(message_aux.length == 0)
            continue;

        if(copy_to_clipboard(fd, message_aux) == -1)
        	break;

		if(pthread_mutex_lock(&mutex_wait[message_aux.region])!=0)
		    error_confirmation("Error mutex_wait: function receiveUP_sendDown");

		if(pthread_cond_broadcast(&data_cond[message_aux.region])!=0)
            error_confirmation("Error in broadcast in function receiveUp_sendDown");

        if(pthread_mutex_unlock(&mutex_wait[message_aux.region])!=0)
            error_confirmation("Error in unlock mutex wait in function receiveUp_sendDown");


		if(pthread_mutex_lock(&mutex_child)!=0)
            error_confirmation("Error in lock mutex child in function receiveUp_sendDown");

        aux = clipboards_list;

        //send all information to all sons
        while(aux != NULL)
        {
            if(write(aux->sock_fd, &message_aux, sizeof(_message))!=sizeof(_message))
            {
                delete_son(clipboards_list,aux->sock_fd);
                error_confirmation("error communication between clipboards on function receiveUp_sendDown");
            }
            if(write(aux->sock_fd, clipboard.matrix[message_aux.region], message_aux.length)!=message_aux.length)
            {
                delete_son(clipboards_list,aux->sock_fd);
                error_confirmation("error communication between clipboards on function receiveUp_sendDown");
            }

            aux = aux->next;
        }
        if(pthread_mutex_unlock(&mutex_child)!=0)
            error_confirmation("Error in unlock mutex child in function receiveUp_sendDown");


    }


    //only arrives here when dad dies and in this case this is clipboard work in singleMode
    close(sock_main_server);
    sock_main_server = SINGLE_MODE;
    receiveUP_sendDOWN(pipefd);

    return NULL;
}