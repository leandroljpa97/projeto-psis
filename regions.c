/**********************************************************************
Authors: Diogo Rodrigues, Nº 84030
		 Leandro Almeida, Nº 84112

Last Updated on XXXXX

File name: region.c

COMMENTS
	Functions that do copy, paste, wait and send up

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
        printf("Alocation error in send_top\n");
        return (-1);
    }

    if(read(user_fd, data, message.length) != message.length)
    {
        free(data);
        return (-1);
    }

    //if the clipboard isn't dad, he sends the information received from the app to dad
    if(sock_main_server != SINGLE_MODE)
    {
        if(pthread_mutex_lock(&mux_sendUP)!=0)
            error_confirmation("error in mutex_ in function send_top: in function send_top");

        if(write(sock_main_server, &message, sizeof(_message)) != sizeof(_message))
        {
            if(pthread_mutex_unlock(&mux_sendUP)!=0)
                error_confirmation("error in mutex_ in function send_top: in function send_top");
            free(data);
            return 0;
        }

        if(write(sock_main_server, data, message.length) != message.length)
        {

           if(pthread_mutex_unlock(&mux_sendUP)!=0)
               error_confirmation("Error unlock mutex mux_sendUp: in function sendTop");
            free(data);
            return 0;
        }

       if(pthread_mutex_unlock(&mux_sendUP)!=0)
           error_confirmation("Error unlock mutex mux_sendUp: in function sendTop");
    }
    // If who receives is dad, he has to send the information across pipe
    else
    {
        if(pthread_mutex_lock(&mux_sendUP)!=0)
            error_confirmation("Error lock mutex mux_sendUp: in function send_Top");

        if(write(pipefd[WRITE], &message, sizeof(_message)) != sizeof(_message))
        {
            if(pthread_mutex_unlock(&mux_sendUP)!=0)
                error_confirmation("Error unlock mux_sendUp: in function send_top");
            free(data);
            error_confirmation("error writing to pipe: in function send_top");
        }
        if(write(pipefd[WRITE], data, message.length) != message.length)
        {
            if(pthread_mutex_unlock(&mux_sendUP)!=0)
                error_confirmation("Error unlock mux_sendUp: in function send_top");
            free(data);
            error_confirmation("error in copy in function send_top:");
        }

        if(pthread_mutex_unlock(&mux_sendUP)!=0)
            error_confirmation("Error unlock mux_sendUp in function send_top:");    }

    free(data);

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

    //read lock to dont do copy and paste at the same time
    if(pthread_rwlock_rdlock(&rwlock[message.region])!=0)
            error_confirmation("Error read_lock- function paste:");

    if(clipboard.matrix[message.region] != NULL)
        message.length = clipboard.size[message.region];
    else
        message.length = 0;

    buf = malloc(message.length);
    if(buf == NULL)
    {
        if(pthread_rwlock_unlock(&rwlock[message.region])!=0)
            error_confirmation("Error unlock rwlock: function paste");

        return(-1);
    }
    //in order to put our critical region more fast, do memcpy and write out of the critical region
    memcpy(buf,clipboard.matrix[message.region],message.length);
    if(pthread_rwlock_unlock(&rwlock[message.region])!=0)
        error_confirmation("Error unlock rwlock: function paste");

    if(write(user_fd, &message, sizeof(_message)) != sizeof(_message))
    {
        return(-1);
    }

    if(message.length != 0)
    {
        if(write(user_fd, buf, message.length) != message.length)
        {
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

    if(pthread_mutex_lock(&mutex_wait[message.region])!=0)
        error_confirmation("Error in lock mutex_wait in function wait");

    //waiting for the new signal
    if(pthread_cond_wait(&data_cond[message.region], &mutex_wait[message.region])!=0)
        error_confirmation("Error in cond_wait of mutex_wait in function wait");

    //go out of the critical region and do paste
    if(pthread_mutex_unlock(&mutex_wait[message.region])!=0)
        error_confirmation("Error in unlock mutex_wait in function wait");

    if(paste(message, user_fd) == -1)
    {
        return (-1);
    }

    return 0;
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
    void * buffer = NULL;
    int receive_bytes;
    buffer = malloc(message_aux.length);
    if(buffer == NULL)
        error_confirmation("buffer alocation in function copy_to_clipboard");

    if((receive_bytes = read(fd, buffer, message_aux.length)) != message_aux.length)
    {
        printf("error in communication between clipboards \n- function copy_to_clipboard");
        free(buffer);
        return -1;
    }

    //create a write lock in order to prevent copy's and pastes at the same time
    if(pthread_rwlock_wrlock(&rwlock[message_aux.region])!=0)
        error_confirmation("ERROR in wrlock in function copy_to_clipboard");

    if(clipboard.matrix[message_aux.region] != NULL)
        free(clipboard.matrix[message_aux.region]);

    clipboard.size[message_aux.region] = message_aux.length;

    clipboard.matrix[message_aux.region] = buffer;

    if(pthread_rwlock_unlock(&rwlock[message_aux.region])!=0)
        error_confirmation("ERROR in readLock in function copy_to_clipboard");

    return receive_bytes;
}


