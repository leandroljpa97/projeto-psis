/**********************************************************************

PROGRAMAÇÃO DE SISTEMAS 2017/2018 - 2º Semestre

Authors: Diogo Rodrigues, Nº 84030
		 Leandro Almeida, Nº 84112

Last Updated on XXXXX

File name: clipboard.h

COMMENTS
	Functions of the clipboard

**********************************************************************/
#ifndef _CLIPBOARD_LIBRARY_H
#define _CLIPBOARD_LIBRARY_H

#include "defines.h"
#include "sock_stream.h"
#include "clipboard.h"

// Message struct for comunication

typedef struct _message_struct  {
	int action; 
	int region;
	size_t length;
	int flag;
} _message;

// Clipboard struct with the information in the regions

typedef struct clipboard_struct
{
	size_t size[MAX_REGIONS];
	void * matrix[MAX_REGIONS];
} _clipboard;

/*
Clipboard_connections struct is the struct that receives all the connections 
from news clipboards with this remote clipboard
*/

typedef struct _clipboards_list_struct 
{
	pthread_t thread_id;
	int sock_fd;
	struct _clipboards_list_struct  * next;
} _clipboards_list;

extern struct sockaddr_un local_addr;
extern struct sockaddr_un user_addr;
extern struct sockaddr_in server_addr;
extern struct sockaddr_in client_addr;

extern int pipefd[2];

extern _clipboard clipboard;
extern _clipboards_list * clipboards_list;

extern int sock_main_server;

extern pthread_mutex_t mux;
extern pthread_rwlock_t rwlock[MAX_REGIONS];
extern pthread_mutex_t mutex_child;
extern int flag_wait[MAX_REGIONS];
extern int waiting_list[MAX_REGIONS];
extern pthread_mutex_t mutex_wait[MAX_REGIONS];
extern pthread_cond_t data_cond[MAX_REGIONS];


void ctrl_c_callback_handler(int signum);
void initialize_clipboard();
int unix_connection();
void * new_thread_clipboard(void * _user_fd);
int send_top(_message message, int user_fd);
int copy_to_clipboard(int fd, _message message_aux);
int paste(_message message, int user_fd);
int wait(_message message, int user_fd);
void * accept_remote_connections();
void generate_random_port_and_bind(int _sock_fd);
int inet_connection_client(char adress[], int port);
void * receiveDOWN_sendUP(void * _socket_fd_son);
_clipboards_list * create_new_son (_clipboards_list * clipboards_list, int sock_fd_aux);
_clipboards_list * delete_son(_clipboards_list *clipboards_list, int socket_fd_son);
void * initialize_receiveUP_sendDOWN();
void * receiveUP_sendDOWN(void * _fd);

#endif