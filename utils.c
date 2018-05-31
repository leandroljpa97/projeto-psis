/**********************************************************************

PROGRAMAÇÃO DE SISTEMAS 2017/2018 - 2º Semestre

Authors: Diogo Rodrigues, Nº 84030
		 Leandro Almeida, Nº 84112

Last Updated on XXXXX

File name: clipboard_library.c

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
	/*
    int i;

    printf("Caught signal Ctr-C\n");

    _clipboards_list * aux = clipboards_list;
    _clipboards_list * aux2 = clipboards_list;

    while(aux != NULL)
    {
    	if(aux->next == NULL)
    	{
    		aux = aux->next;
    		continue;
    	}

    	aux2 = aux->next;

    	close(aux->sock_fd);

    	free(aux);

    	aux = aux2;
    }

    for(i = 0; i < 10; i++)
    {
    	printf("FINAL: Conteudo da região %d é %s\n", i, (char *) clipboard.matrix[i]);

        if(clipboard.matrix[i]!=NULL)
            free(clipboard.matrix[i]);

        pthread_rwlock_destroy(&rwlock[i]);
        pthread_mutex_destroy(&mutex_wait[i]);
    }

    pthread_mutex_destroy(&mutex_child);
    pthread_mutex_destroy(&mux);
    */

    unlink(SOCK_ADDRESS);

    exit(EXIT_SUCCESS);
}

int system_error(void *node)
{

    if(node == NULL)
    {
        return -1;
    }

    return 0;
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
    sock_main_server = -1;
    
    void (*ctrl_c)(int);
    ctrl_c = signal(SIGINT, ctrl_c_callback_handler);

    void (*close_socket)(int);
    close_socket = signal(SIGPIPE, SIG_IGN);

    if(ctrl_c == SIG_ERR || close_socket == SIG_ERR)
    {
        printf("Could not handle SIGINT or SIGPIPE");
        exit(EXIT_FAILURE);
    }

    if (pipe(pipefd) == -1)
    {
        perror("Pipe");
        exit(EXIT_FAILURE);
    }

    //Initialize mutex!
    if(pthread_mutex_init(&mux, NULL) != 0 || pthread_mutex_init(&mutex_child,NULL) != 0)
    {
        perror("Mutex_init");
        exit(EXIT_FAILURE);
    }

    for(i = 0; i < 10; i++)
    {
        clipboard.size[i] = 0;
        clipboard.matrix[i] = NULL;

        flag_wait[i] = 0;
        waiting_list[i] = 0;

        if(pthread_mutex_init(&mutex_wait[i], NULL) != 0)
        {
            perror("Mutex_init");
            exit(EXIT_FAILURE);
        }

        if(pthread_rwlock_init(&rwlock[i], NULL) != 0)
        {
            perror("rwlock_init");
            exit(EXIT_FAILURE);
        }
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

    while(err == -1)
    {
        port = rand() % (64738 - 1024) + 1024;
        server_addr.sin_port = htons(port);

        if(counter > 50 ) {
            perror("Bind");
            exit(EXIT_FAILURE);
        }

        err = bind(_sock_fd, (struct sockaddr* ) &server_addr, sizeof(struct sockaddr_in));

        counter += 1;
        printf("O counter chegou a %d\n", counter);
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

int inet_connection_client(char adress[], int port) {
    int sock_fd_remote = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd_remote == -1) {
        perror("socket: ");
        exit(-1);
    }

    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(port);
    inet_aton(adress, &client_addr.sin_addr);

    if (connect(sock_fd_remote, (const struct sockaddr *) &client_addr, sizeof(struct sockaddr_in)) == -1) {
        perror("Error connecting with remote clipboard\n");
        exit(EXIT_FAILURE);
    }

    return sock_fd_remote;
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
    {
        perror("Socket:");
        exit(EXIT_FAILURE);
    }

    local_addr.sun_family = AF_UNIX;
    strcpy(local_addr.sun_path, SOCK_ADDRESS);

    int err = bind(CLIPBOARD_SOCKET, (struct sockaddr *) &local_addr, sizeof(struct sockaddr_un));
    if(err == -1)
    {
        printf("CHEGUEI AQUI\n");

        perror("Bind:");
        exit(EXIT_FAILURE);
    }

    if(listen(CLIPBOARD_SOCKET, 2) == -1)
    {
        perror("Listen:");
        exit(EXIT_FAILURE);
    }

    return CLIPBOARD_SOCKET;
}

/**********************************************************************

send_top()

Arguments: message - message struct
		   user_fd - value returned by the unix_connection

Return: (void)

Effects: spread information to the top of the tree

Description:
	This function sends the copy (the new data) to the top of the tree

**********************************************************************/

int send_top(_message message, int user_fd)
{
    void * data;

    data = malloc(message.length);
    if(data == NULL)
    {
        printf("Alocation error\n");
        return (-1);

    }

    if(write(user_fd, &(message.flag), sizeof(int)) != sizeof(int))
    {
        printf("Error in copy\n");
        return (-1);
    }

    if(read(user_fd, data, message.length) != message.length)
    {
        printf("Error in copy\n");
        return (-1);
    }

    if(sock_main_server!=-1)
    {
        pthread_mutex_lock(&mux);
        if(write(sock_main_server, &message, sizeof(_message)) != sizeof(_message))
        {
            printf("Error in copy\n");
            return (-1);
        }
        if(write(sock_main_server, data, message.length) != message.length)
        {
            printf("Error in copy\n");
            return (-1);
        }
        pthread_mutex_unlock(&mux);

    }
    // If who receives is dad, he has to send the information across pipe
    else
    {
        pthread_mutex_lock(&mux);
        if(write(pipefd[WRITE], &message, sizeof(_message)) != sizeof(_message))
        {
            perror("Error in copy\n");
        }
        if(write(pipefd[WRITE],data,message.length) != message.length)
        {
            printf("Error in copy\n");
            return (-1);
        }
        pthread_mutex_unlock(&mux);

    }

    return 0;
}

/*********************************************************************

paste()

Arguments: message - message struct
		   user_fd - value returned by the unix_connection

Return: (void)

Effects: sends the information present in the region requested by the user

Description:
	This function sends the information present in the region requested by the user

**********************************************************************/

int paste(_message message, int user_fd)
{
    void * buf = NULL;

    pthread_rwlock_rdlock(&rwlock[message.region]);
    if(clipboard.matrix[message.region] != NULL )
    {
        message.length=clipboard.size[message.region];
        message.flag=SUCCESS;
    }
    else
    {
        message.length=0;
        message.flag=ERROR;
    }

    buf=malloc(message.length);
    if(buf==NULL)
    {
        printf("Error in buf alocation \n");
        return(-1);
    }

    memcpy(buf,clipboard.matrix[message.region],message.length);
    pthread_rwlock_unlock(&rwlock[message.region]);

    if(write(user_fd, &message, sizeof(_message)) != sizeof(_message))
    {
        printf("o write vai retornar -1 sigpipe!! \n");
        return(-1);
    }

    if(message.length!=0)
    {
        if(write(user_fd, buf, message.length) != message.length)
        {
            printf("o write vai retornar -1 sigpipe OUTRA VEZ!! \n");

            perror("Error in paste\n");
            return (-1);
        }

    }
    return 0;



}

/**********************************************************************

wait()

Arguments: message - message struct
		   user_fd - value returned by the unix_connection

Return: (void)

Effects: waits for a change on a certain region (new copy)

Description:
	This function waits for a change on a certain region (new copy) and
	when it happens make a paste

**********************************************************************/

int wait(_message message, int user_fd)
{
    pthread_mutex_lock(&mutex_wait[message.region]);

    pthread_cond_wait(&data_cond[message.region], &mutex_wait[message.region]);

    pthread_mutex_unlock(&mutex_wait[message.region]);

    if(paste(message, user_fd) == -1)
    {
        printf("Error in paste of the wait\n");
        return (-1);
    }

    return 0;
}

_clipboards_list * create_new_son (_clipboards_list * clipboards_list, int sock_fd_aux){

    _clipboards_list * aux = (_clipboards_list *) malloc(sizeof(_clipboards_list));
    aux->sock_fd = sock_fd_aux;

    aux->next = clipboards_list;

    return aux;
}

_clipboards_list * delete_son(_clipboards_list *clipboards_list, int socket_fd_son)
{
    _clipboards_list * aux ;
    _clipboards_list *current;


    aux=clipboards_list;
    if(clipboards_list->sock_fd==socket_fd_son)
    {
        clipboards_list=clipboards_list->next;
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
    free(aux);
    return clipboards_list;
}

/**********************************************************************

copy_to_clipboard()

Arguments: message_aux - message struct
		   fd - value returned by the unix_connection

Return: (void)

Description: save the information into clipboard

**********************************************************************/

int copy_to_clipboard(int fd, _message message_aux)
{
    void * buffer=NULL;
    int erc;
    buffer= malloc(message_aux.length);
    if((erc=read(fd, buffer, message_aux.length))!=message_aux.length)
    {
        printf("merdaaa \n");
        return 0;
    }
    pthread_rwlock_wrlock(&rwlock[message_aux.region]);

    if(clipboard.matrix[message_aux.region] != NULL)
        free(clipboard.matrix[message_aux.region]);

    clipboard.size[message_aux.region]=message_aux.length;
    printf("recebi o buffer do pipe ou do outro : %s \n", (char *)buffer);

    clipboard.matrix[message_aux.region] = buffer;
    printf("o clipboard correspondente é :%s da posicao : %d \n", (char *) clipboard.matrix[message_aux.region], message_aux.region);

    pthread_rwlock_unlock(&rwlock[message_aux.region]);

    return erc;
}