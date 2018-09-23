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
#include "clipboard.h"

// Message struct for comunication
typedef struct _message_struct  {
	int action; 
	int region;
	size_t length;
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
	int sock_fd;
	struct _clipboards_list_struct  * next;
} _clipboards_list;

//all the sockets adress used
extern struct sockaddr_un local_addr;
extern struct sockaddr_un user_addr;
extern struct sockaddr_in server_addr;
extern struct sockaddr_in client_addr;

//pipe that is used to communication in dad to receive in one thread the information from apps and son's and send to sons
extern int pipefd[2];


extern _clipboard clipboard;
extern _clipboards_list * clipboards_list;

//socket dad
extern int sock_main_server;

//mutex and read/write locks used to syncronize the program
extern pthread_mutex_t mux_sendUP;
extern pthread_rwlock_t rwlock[MAX_REGIONS];
extern pthread_mutex_t mutex_child;
extern pthread_mutex_t mutex_wait[MAX_REGIONS];
extern pthread_cond_t data_cond[MAX_REGIONS];

/**********************************************************************

ctrl_c_callback_handler()

Arguments: signum - signal number corresponding to the signal that needs to be handled

Return: void

Effects: close the program

Description:
	This function is called to close the program

**********************************************************************/
void ctrl_c_callback_handler(int signum);

/**********************************************************************

error_confirmation()

 message error and exit of the program

**********************************************************************/
void error_confirmation();

/**********************************************************************

initialize_clipboard()

Arguments: none

Return: (void)

Effects: initialize clipboard

Description:
	This function initializes all variables needed for the clipboard to work

**********************************************************************/
void initialize_clipboard();

/**********************************************************************

verification_library()

Arguments: none

Return: int


Description:
    This function check if the data insered by library is valid
**********************************************************************/
int verification_library(_message message);

/**********************************************************************

unix_connection()

Arguments: none

Return: int

Effects: clipboard is ready to unix connection

Description:
	This function is called by the clipboard to do unix connection with the users

**********************************************************************/
int unix_connection();

/**********************************************************************

user_communication
Arguments: user_fd - value returned by the unix_connection

Return: (void *)

Effects: communication with a user

Description:
	This function do all the communication with a user

**********************************************************************/
void * user_communication(void * _user_fd);
int send_top(_message message, int user_fd);
int copy_to_clipboard(int fd, _message message_aux);
int paste(_message message, int user_fd);
int wait(_message message, int user_fd);

/**********************************************************************

accept_remote_connections()

Arguments: none

Return: (void *)

Effects: clipboard is ready to inet connection

Description:
	This function is called by the remote clipboard to accept inet connection
	with news clipboards

**********************************************************************/

void * accept_remote_connections();
void generate_random_port_and_bind(int _sock_fd);
int inet_connection_client(char adress[], int port);
/**********************************************************************

receiveDown_sendUP()

Arguments: _socket_fd_son

Return: (void *)


Description:
	This thread is always ready to read from one son and send the information to the parent
    there are one thread "receiveDOWN_send_UP per son
**********************************************************************/
void * receiveDOWN_sendUP(void * _socket_fd_son);

/**********************************************************************

create_new_son()

Arguments: clipboards_list * - _clipboards_list
            sock_fd_aux - int

Return: _clipboards_list

Effects: create new son and set in list

Description: create new son and save sock_fd


**********************************************************************/
_clipboards_list * create_new_son (_clipboards_list * clipboards_list, int sock_fd_aux);

/**********************************************************************

delete_son()

Arguments: clipboards_list * - _clipboards_list
            sock_fd_son - int

Return: _clipboards_list

Description: delete new son of the list


**********************************************************************/
_clipboards_list * delete_son(_clipboards_list *clipboards_list, int socket_fd_son);


/**********************************************************************

receiveUP_sendDown()

Arguments: _socket_fd_son

Return: (void *)


Description:
	This thread is always read the information from the parent and send to all sons
    there are one thread "receiveDOWN_send_UP per son
**********************************************************************/
void * receiveUP_sendDOWN(void * _fd);

#endif