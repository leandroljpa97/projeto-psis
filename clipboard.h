/**********************************************************************

PROGRAMAÇÃO DE SISTEMAS 2017/2018 - 2º Semestre

Authors: Diogo Rodrigues, Nº 84030
		 Leandro Almeida, Nº 84112

Last Updated on XXXXX

File name: clipboard.h

COMMENTS
	Functions of the API

**********************************************************************/

#ifndef _CLIPBOARD_H
#define _CLIPBOARD_H

#include <sys/types.h>

#include "defines.h"
#include "clipboard_library.h"

/**********************************************************************

clipboard_connect()

Arguments: clipboard_dir - directory where the local clipboard was launched

Return: int

Effects: connect a app with a local clipboard

Description:
	This function is called by the application before interacting with
	the distributed clipboard

**********************************************************************/
int clipboard_connect(char * clipboard_dir);

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
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);

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
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);

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
int clipboard_wait(int clipboard_id, int region, void *buf, size_t count);

/**********************************************************************

clipboard_close()

Arguments: clipboard_id - value returned by the clipboard_connect

Return: void

Effects: close connection

Description:
	This function closes the connection between the application and the
	local clipboard

**********************************************************************/
void clipboard_close(int clipboard_id);

#endif
