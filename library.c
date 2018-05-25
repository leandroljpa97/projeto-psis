/**********************************************************************

PROGRAMAÇÃO DE SISTEMAS 2017/2018 - 2º Semestre

Authors: Diogo Rodrigues, Nº 84030
		 Leandro Almeida, Nº 84112

Last Updated on XXXXX

File name: library.c

COMMENTS
	Functions of the API

**********************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>

#include "clipboard.h"

/**********************************************************************

clipboard_connect()

Arguments: clipboard_dir - directory where the local clipboard was launched

Return: int

Effects: connect a app with a local clipboard

Description: 
	This function is called by the application before interacting with 
	the distributed clipboard

**********************************************************************/

int clipboard_connect(char * clipboard_dir) 
{
	struct sockaddr_un server_addr;
	
	int clipboard_id = socket(AF_UNIX, SOCK_STREAM, 0);
	if (clipboard_id == -1)
	{
		perror("Socket:");
		return(-1);
	}

	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, SOCK_ADDRESS);

	int err_c = connect(clipboard_id, (const struct sockaddr *) &server_addr, sizeof(struct sockaddr_un));
	if(err_c == -1)
	{
		perror("Connect:");
		return(-1);
	}

	return clipboard_id;
}

/**********************************************************************

clipboard_copy()

Arguments: clipboard_id - value returned by the clipboard_connect
		   region - identification of the region the user wants to copy the data to
		   buf - pointer to the data that is to be copied to the local clipboard
		   count - the length of the data pointed by the buf

Return: int

Effects: copies a new data to a region on the local clipboard

Description: 
	This function copies the data pointed by buf to a region on the local 
	clipboard

**********************************************************************/

int clipboard_copy(int clipboard_id, int region, void *buf, size_t count)
{
	_message message;
	int send_bytes, err_rcv, flag_check = ERROR;

	if(region < 0 || region >= MAX_REGIONS || clipboard_id < 0)
	{
		printf("Error - Invalid Information (Region or clipboard_id)\n");
		return 0;
	}

	message.action = COPY;
	message.region = region;
	message.length = count;
	message.flag = SUCCESS;

	if((send_bytes = write(clipboard_id, &message, sizeof(_message))) != sizeof(_message))
	{
		printf("Error on clipboard_copy\n");
		return 0;
	}
			
	err_rcv = read(clipboard_id, &flag_check, sizeof(int));
	message.flag = flag_check; 

	if(err_rcv != sizeof(int) || message.flag == ERROR)
	{
		printf("Error on clipboard_copy\n");
		return 0;
	}

	if((send_bytes = write(clipboard_id, buf, message.length)) != message.length)
	{		
		printf("Error on clipboard_copy\n");
		return 0;
	}
	else 
		return send_bytes; 
}

/**********************************************************************

clipboard_paste()

Arguments: clipboard_id - value returned by the clipboard_connect
		   region - region the user wants to paste data from
		   buf - pointer to the data where the data is to be copied to
		   count - the length of memory region pointed by buf

Return: int

Effects: copies from the system the data in a certain region to the application 

Description: 
	This function copies from the system to the application the data in a certain 
	region. The copied data is stored in the memory pointed by buf up to a length 
	of count

**********************************************************************/

int clipboard_paste(int clipboard_id, int region, void *buf, size_t count)
{
	_message message;
	int send_bytes, err_rcv, receive_bytes;

	if(region < 0 || region >= MAX_REGIONS || clipboard_id < 0)
	{
		printf("Error - Invalid Information (Region or clipboard_id)\n");
		return 0;
	}

	message.action = PASTE;
	message.region = region;

	if((send_bytes = write(clipboard_id, &message, sizeof(_message))) != sizeof(_message))
	{
		printf("Error on clipboard_paste\n");
		return 0;
	}
	
	err_rcv = read(clipboard_id, &message, sizeof(_message));
	if(err_rcv != sizeof(_message) || message.flag == ERROR)
	{
		printf("Error on clipboard_paste\n");
		return 0;
	}

	if((receive_bytes = read(clipboard_id, buf, message.length)) != message.length)
	{		
		printf("Error on clipboard_paste\n");
		return 0;
	}
	else 
		return receive_bytes;
}

/**********************************************************************

clipboard_wait()

Arguments: clipboard_id - value returned by the clipboard_connect
		   region - region the user wants to wait for
		   buf - pointer to the data where the data is to be copied to
		   count - the length of memory region pointed by buf

Return: int

Effects: waits for a change on a certain region and when it happens the 
		 new data in that region is copied to memory 

Description: 
	This function waits for a change on a certain region ( new copy), 
	and when it happens the new data in that region is copied to memory 
	pointed by buf. The copied data is stored in the memory pointed by 
	buf up to a length of count

**********************************************************************/

int clipboard_wait(int clipboard_id, int region, void *buf, size_t count)
{
	_message message;
	int send_bytes, receive_bytes, err_rcv;

	if(region < 0 || region >= MAX_REGIONS || clipboard_id < 0)
	{
		printf("Error - Invalid Information (Region or clipboard_id)\n");
		return 0;
	}

	message.action = WAIT;
	message.region = region;

	if((send_bytes = write(clipboard_id, &message, sizeof(_message))) != sizeof(_message))
	{
		printf("Error on clipboard_wait\n");
		return 0;
	}

	err_rcv = read(clipboard_id, &message, sizeof(_message));
	if(err_rcv != sizeof(_message) || message.flag == ERROR)
	{
		printf("Error on clipboard_wait\n");
		return 0;
	}

	if((receive_bytes = read(clipboard_id, buf, message.length)) != message.length)
	{		
		printf("Error on clipboard_paste\n");
		return 0;
	}
	else 
		return receive_bytes;
}

/**********************************************************************

clipboard_close()

Arguments: clipboard_id - value returned by the clipboard_connect
		   
Return: void

Effects: close connection

Description: 
	This function closes the connection between the application and the 
	local clipboard

**********************************************************************/

void clipboard_close(int clipboard_id)
{
	close(clipboard_id);
}