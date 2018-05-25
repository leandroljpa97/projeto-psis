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
#include "sock_stream.h"
#include "clipboard_library.h"

int clipboard_connect(char * clipboard_dir);
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);
int clipboard_wait(int clipboard_id, int region, void *buf, size_t count);
void clipboard_close(int clipboard_id);

#endif