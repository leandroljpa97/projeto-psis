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
pthread_rwlock_t rw_child;
int flag_wait[MAX_REGIONS];
int waiting_list[MAX_REGIONS];
pthread_mutex_t mutex_wait[MAX_REGIONS];
pthread_cond_t data_cond[MAX_REGIONS] = PTHREAD_COND_INITIALIZER;

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
	int i;

	printf("Caught signal Ctr-C\n");

	for(i = 0; i < 10; i++)
	{
		printf("Conteudo da região %d é %s\n", i, clipboard.matrix[i]);
	}

	for(i = 0; i < 10; i++)
	{
		if(clipboard.matrix[i]!=NULL)
			free(clipboard.matrix[i]);

		pthread_rwlock_destroy(&rwlock[i]);
		pthread_mutex_destroy(&mutex_wait[i]);
	}

    pthread_rwlock_destroy(&rw_child);
	pthread_mutex_destroy(&mux);

	unlink(SOCK_ADDRESS);

	exit(0);
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

	signal(SIGINT, ctrl_c_callback_handler);

	//Initialize mutex!
    if(pthread_mutex_init(&mux, NULL) != 0)
    {
        perror("Mutex_init:");
        exit(EXIT_FAILURE);
    }

    if (pipe(pipefd) == -1)
    {
    	perror("Pipe:");
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
        	perror("Mutex_init:");
        	exit(EXIT_FAILURE);
		}

		if(pthread_rwlock_init(&rwlock[i], NULL) != 0) 
		{
			perror("rwlock_init:");
        	exit(EXIT_FAILURE);
		}
	}
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
			if(copy(message, user_fd) == -1)
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

copy()

Arguments: message - message struct 
		   user_fd - value returned by the unix_connection

Return: (void)

Effects: spread information to the top of the tree

Description: 
	This function sends the copy (the new data) to the top of the tree 

**********************************************************************/

int copy(_message message, int user_fd)
{
	char * data;

    data = (char*) malloc(message.length);
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

/**********************************************************************

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
	char * buf = NULL;

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

	buf=malloc(message.length*sizeof(char));
	if(buf==NULL)
	{
		printf("Error in buf alocation \n");
		return(-1);
	}
		
	memcpy(buf,clipboard.matrix[message.region],message.length*sizeof(char));
	pthread_rwlock_unlock(&rwlock[message.region]);

	if(write(user_fd, &message, sizeof(_message)) != sizeof(_message))
	{
		printf("Error in buf alocation \n");
		return(-1);
    }

	if(write(user_fd, buf, message.length) != message.length)
	{
		perror("Error in paste\n");
       	return (-1);
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
	waiting_list[message.region] = waiting_list[message.region] + 1;

	pthread_mutex_lock(&mutex_wait[message.region]);
	while(flag_wait[message.region] != 1)
	{
		pthread_cond_wait(&data_cond[message.region], &mutex_wait[message.region]);
	}
	
	pthread_mutex_unlock(&mutex_wait[message.region]);

	if(paste(message, user_fd) == -1)
	{
		printf("Error in paste of the wait\n");
		return (-1);
	}

	waiting_list[message.region]=waiting_list[message.region] - 1;
	flag_wait[message.region] = 0;
	
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
            perror("Bind:");
            exit(EXIT_FAILURE);
        }
        
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
	if (sock_fd_remote == -1){
		perror("socket: ");
		exit(-1);
	}

	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(port);
	inet_aton(adress, &client_addr.sin_addr);

	if(connect(sock_fd_remote, (const struct sockaddr *) &client_addr, sizeof(struct sockaddr_in)) == -1){
		perror("Error connecting with remote clipboard\n");
		exit(EXIT_FAILURE);
	}

	return sock_fd_remote;
}


void * receiveDOWN_sendUP(void * _socket_fd_son)
{
    int socket_fd_son= *((int*)_socket_fd_son);
    _message message;
    char * buffer_aux=NULL;
    int j;
    //---------------initialize my son------------------------
	//nao preciso de meter aqui agora um readlock????

    pthread_rwlock_rdlock(&rw_child);

    clipboards_list = create_new_son (clipboards_list, socket_fd_son);

    for(j=0; j<10; j++)
    {
        sleep(2);
        printf("região %d \n",j);
        //atencao isto de baixo é para sair pah o sizeof char e tal
        message.length=clipboard.size[j];
        printf("A região %d tem size de %lu \n", j, clipboard.size[j]);
        message.region=j;
        printf("CHEGUEI AQUI!!\n");

        write(socket_fd_son, &message, sizeof(_message));

        //aqui nao pode entretanto mudar????

        printf("VAI DAR BOSTA\n");

        if(message.length != 0)
        {
        	printf("SOU O PAI E MANDEI ATUALIZAÃO PARA O FILHO\n");

            printf("o message.length abtes é %lu!!!!!\n", message.length);

            read(socket_fd_son, &message, sizeof(_message));
            printf("o message.length depois é %lu!!!!!\n", message.length);

            paste(message, socket_fd_son);
       	}



    }
    pthread_rwlock_unlock(&rw_child);



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
        buffer_aux= malloc(message.length*sizeof(char));
        if(buffer_aux==NULL)
        {
            printf("mal inicializado \n");
            exit(-1);
        }
        read(socket_fd_son, buffer_aux,message.length*sizeof(char));
        if(sock_main_server!=-1)
        {
            pthread_mutex_lock(&mux);
            write(sock_main_server,&message,sizeof(_message));
            write(sock_main_server,buffer_aux,message.length*sizeof(char));
            pthread_mutex_unlock(&mux);

        }
            //if who receives is dad, he has to send the information across pipe
        else
        {
            pthread_mutex_lock(&mux);
            write(pipefd[WRITE],&message,sizeof(_message));
            write(pipefd[WRITE],buffer_aux,message.length*sizeof(char));
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

void * initialize_receiveUP_sendDOWN()
{
    _message message_aux;
    int j;

    //receive information by parent to initialize
    for(j = 0; j < 10; j++)
    {
        int err_rcv = recv(sock_main_server, &message_aux, sizeof(_message), 0);
        if(err_rcv != sizeof(_message))
            return 0;

        printf("Li a %d mensagem\n", j);


        clipboard.size[j] = message_aux.length;

        printf("clipboard.size[j]= %lu \n",clipboard.size[j]);
        if(clipboard.size[j] != 0)
        {
        	printf("SOU O FILHO E VOU RECEBER UMA ATUALIZAÃO\n");

            printf("Vou atualizar a região %d\n", j);
            clipboard.matrix[j] = (char *) malloc(message_aux.length*sizeof(char));

            if((clipboard.matrix[j] == NULL) || (clipboard_paste(sock_main_server, message_aux.region, clipboard.matrix[j], clipboard.size[j]) == 0))
            {
                printf("a posicao que dá erro é %d \n",j);
                printf("ERROR IN PASTE\n");

                printf("clipboard.matrix[j] dá : %s de size %lu \n",clipboard.matrix[j],clipboard.size[j]);
                //é suposto sair??????????????
            }
            else
                printf("DATA RECEIVED dentro do clipboards: %s\n",clipboard.matrix[j]);
        }

    }

    receiveUP_sendDOWN(&sock_main_server);

    		return 0;

}

void * receiveUP_sendDOWN(void * _fd)
{
    int fd = *((int *) _fd);
    _message message_aux;
    char * buffer = NULL;
    _clipboards_list * aux;


    while(read(fd, &message_aux, sizeof(_message)) >0)
    {
        printf("recebi do pipe ou do socket normal \n");
        //MAIS UMA VEZ ISTO É A FUNCAO DE COPY - A NOSSA 'NOVA ' FUNCAO (QUE NAO EXISTE)
        //isto é que é o 'novo' copy- criar uma funcao para isto maybe! quando o pai escreve po filho

        buffer=malloc(sizeof(char)*message_aux.length);
        if(read(fd, buffer, message_aux.length*sizeof(char))!=message_aux.length*sizeof(char))
        {
        	printf("merdaaa \n");
        	return 0;
        }

		pthread_mutex_lock(&mutex_wait[message_aux.region]);
		pthread_rwlock_wrlock(&rwlock[message_aux.region]);

        if(clipboard.matrix[message_aux.region] != NULL)
        	free(clipboard.matrix[message_aux.region]);

        clipboard.size[message_aux.region]=message_aux.length;
        printf("recebi o buffer do pipe ou do outro : %s \n",buffer);

        clipboard.matrix[message_aux.region] = buffer;
        printf("o clipboard correspondente é :%s da posicao : %d \n",clipboard.matrix[message_aux.region],message_aux.region);

		pthread_rwlock_unlock(&rwlock[message_aux.region]);
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

		pthread_rwlock_wrlock(&rw_child);
        while(aux!=NULL)
        {
            printf("tou a mandar o copy para os meus filhos \n");
            write(aux->sock_fd, &message_aux, sizeof(_message));
            write(aux->sock_fd, buffer, message_aux.length*sizeof(char));
            aux=aux->next;
        }
        pthread_rwlock_unlock(&rw_child);

        for(int i = 0; i < 10; i++)
		{
			printf("Conteudo da região %d é %s\n", i, clipboard.matrix[i]);
		}
    }

    sock_main_server=-1;
    receiveUP_sendDOWN(pipefd);

    		return 0;

}