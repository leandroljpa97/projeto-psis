/**********************************************************************

PROGRAMAÇÃO DE SISTEMAS 2017/2018 - 2º Semestre

Authors: Diogo Rodrigues, Nº 84030
		 Leandro Almeida, Nº 84112

Last Updated on XXXXX

File name: utils.c

COMMENTS
	Functions of the clipboard

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

ctrl_c_callback_handler()

Arguments: signum - signal number corresponding to the signal that needs to be handled

Return: void

Effects: close the program

Description:
	This function is called to close the program

**********************************************************************/

void ctrl_c_callback_handler(int signum)
{
    unlink(SOCK_ADDRESS);
    exit(0);

}


/**********************************************************************

error_confirmation()

 message error and exit of the program

**********************************************************************/
void error_confirmation(char*s)
{
        printf("%s \n",s);
        exit(EXIT_FAILURE);
}

/**********************************************************************

initialize_clipboard()

Arguments: none

Return: (void)

Effects: initialize clipboard

Description:
	This function initializes all variables needed for the clipboard to work

**********************************************************************/

void initialize_clipboard()
{
    int i;

    clipboards_list = NULL;

    // If a clipboard starts in single mode (-1)
    sock_main_server = SINGLE_MODE;
    
    void (*ctrl_c)(int);
    ctrl_c = signal(SIGINT, ctrl_c_callback_handler);

    void (*close_socket)(int);
    close_socket = signal(SIGPIPE, SIG_IGN);

    if(ctrl_c == SIG_ERR || close_socket == SIG_ERR)
        error_confirmation("Could not handle SIGINT or SIGPIPE");

    if (pipe(pipefd) == -1)
        error_confirmation("Pipe:");

    //Initialize mutex!
    if(pthread_mutex_init(&mux_sendUP, NULL) != 0 || pthread_mutex_init(&mutex_child,NULL) != 0)
        error_confirmation("Mutex_Init");

    for(i = 0; i < MAX_REGIONS; i++)
    {
        clipboard.size[i] = 0;
        clipboard.matrix[i] = NULL;

        if(pthread_mutex_init(&mutex_wait[i], NULL) != 0)
            error_confirmation("Mutex_init");

        if(pthread_rwlock_init(&rwlock[i], NULL) != 0)
            error_confirmation("Mutex_init:");
    }
}


/**********************************************************************

generate_random_port()

Arguments: none

Return: int

Effects: generate a random port

Description:
	This function is called by the function that accept remote connections
	and an int that will be the port

**********************************************************************/

void generate_random_port_and_bind(int _sock_fd)
{
    int err = -1, counter = 0, port = 0;

    srand(getpid());

    //until 100, if the port already exist, generate new number
    while(err == -1)
    {
        port = rand() % (64738 - 1024) + 1024;
        server_addr.sin_port = htons(port);

        if(counter > 100 )
            error_confirmation("error in bind");

        err = bind(_sock_fd, (struct sockaddr* ) &server_addr, sizeof(struct sockaddr_in));

        counter += 1;
    }

    printf("The port to connect with me is %d\n", port);
}

/**********************************************************************

inet_connection_client()

Arguments: adress - adress of the host where the remote clipboard is running
		   port - port of the host where the remote clipboard is running

Return: int

Effects: inet connection with a remote clipboard

Description:
	This function is called by the new clipboard to do inet connection
	with a remote clipboard

**********************************************************************/

int inet_connection_client(char adress[], int port) 
{
    int sock_fd_remote = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd_remote == -1)
        error_confirmation("socket in inet_connection_client:");

    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(port);
    inet_aton(adress, &client_addr.sin_addr);

    if (connect(sock_fd_remote, (const struct sockaddr *) &client_addr, sizeof(struct sockaddr_in)) == -1)
        error_confirmation("error connecting with client:");

    return sock_fd_remote;
}
/**********************************************************************

verification_library()

Arguments: none

Return: int


Description:
    This function check if the data insered by library is valid
**********************************************************************/
int verification_library(_message message)
{
    if(message.region>=MAX_REGIONS|| message.region<0 || (message.action!=WAIT && message.action!=PASTE && message.action!=COPY))
    {
        return 0;
    }

    return 1;
}

/**********************************************************************

unix_connection()

Arguments: none

Return: int

Effects: clipboard is ready to unix connection

Description:
	This function is called by the clipboard to do unix connection with the users

**********************************************************************/

int unix_connection()
{
    int CLIPBOARD_SOCKET = socket(AF_UNIX, SOCK_STREAM, 0);
    if (CLIPBOARD_SOCKET == -1)
    error_confirmation("error in socket unix:");
    local_addr.sun_family = AF_UNIX;
    strcpy(local_addr.sun_path, SOCK_ADDRESS);

    int err = bind(CLIPBOARD_SOCKET, (struct sockaddr *) &local_addr, sizeof(struct sockaddr_un));
    if(err == -1)
    error_confirmation("BIND:");

    if(listen(CLIPBOARD_SOCKET, 2) == -1)
    error_confirmation("LISTEN:");

    return CLIPBOARD_SOCKET;
}


/**********************************************************************

create_new_son()

Arguments: clipboards_list * - _clipboards_list
            sock_fd_aux - int

Return: _clipboards_list

Effects: create new son and set in list

Description: create new son and save sock_fd


**********************************************************************/


_clipboards_list * create_new_son (_clipboards_list * clipboards_list, int sock_fd_aux)
{
    _clipboards_list * aux = (_clipboards_list *) malloc(sizeof(_clipboards_list));
    if(aux == NULL)
        return NULL;

    aux->sock_fd = sock_fd_aux;

    aux->next = clipboards_list;

    return aux;
}

/**********************************************************************

delete_son()

Arguments: clipboards_list * - _clipboards_list
            sock_fd_son - int

Return: _clipboards_list

Description: delete new son of the list


**********************************************************************/

_clipboards_list * delete_son(_clipboards_list *clipboards_list, int socket_fd_son)
{

    _clipboards_list * aux ;
    _clipboards_list *current;

    aux=clipboards_list;
    if(clipboards_list->sock_fd==socket_fd_son)
    {
        clipboards_list=clipboards_list->next;
        clipboard_close(aux->sock_fd);
        free(aux);
        return clipboards_list;

    }
    current=clipboards_list;
    aux=aux->next;
    while(aux!=NULL && aux->sock_fd !=socket_fd_son)
    {
        current=aux;
        aux=aux->next;

    }
    current->next=aux->next;
    clipboard_close(aux->sock_fd);
    free(aux);

    return clipboards_list;
}


