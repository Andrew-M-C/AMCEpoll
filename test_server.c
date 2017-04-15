/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	test.c
	
	Description: 	
	    This is the test source for AMCEpoll. If you want to use AMCEpoll itself
	only, please link .a or .so file.
			
	History:
		2017-04-08: File created as "test.c"

	------------------------------------------------------------------------

	    This library is free software; you can redistribute it and/or modify it 
	under the terms of the GNU Lesser General Public License as published by the 
	Free Software Foundation, version 2.1 of the License. 
	    This library is distributed in the hope that it will be useful, but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
	details. 
	    You should have received a copy of the GNU Lesser General Public License 
	along with this library; if not, see <http://www.gnu.org/licenses/>.
		
********************************************************************************/

#include "AMCEpoll.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define _CFG_SRV_PORT		5656
#define _LOG(fmt, args...)		printf("[Srv - %04d] "fmt"\n", __LINE__, ##args)

/********/
#define __EVENT_CALLBACKS
#ifdef __EVENT_CALLBACKS

/* ------------------------------------------- */
static void _callback_accept(int fd, uint16_t events, void *arg)
{
	// TODO:
	_LOG("TODO: Accept()");
	return;
}

#endif

/********/
#define __SERVER_PREPARATION
#ifdef __SERVER_PREPARATION

/* ------------------------------------------- */
int _create_local_server(struct AMCEpoll *base)
{
	int retCode = -1;
	int callStat = 0;
	struct sockaddr_in address;
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		goto ERROR;
	}

	callStat = AMCFd_MakeNonBlock(fd);
	if (callStat < 0) {
		goto ERROR;
	}

	callStat = AMCFd_MakeCloseOnExec(fd);
	if (callStat < 0) {
		goto ERROR;
	}

	address.sin_family = AF_INET;
	address.sin_port = htons(_CFG_SRV_PORT);
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	callStat = bind(fd, (struct sockaddr *)&address, sizeof(struct sockaddr_in));
	if (callStat < 0) {
		int err = errno;
		_LOG("Failed to bind for fd %d: %s", fd, strerror(err));
		errno = err;
		goto ERROR;
	}

	callStat = AMCEpoll_AddEvent(base, fd,
					EP_MODE_PERSIST | EP_MODE_EDGE | EP_EVENT_READ | EP_EVENT_ERROR | EP_EVENT_FREE | EP_EVENT_TIMEOUT,
					0, _callback_accept, base, NULL);\
	if (callStat < 0) {
		goto ERROR;
	}

	return 0;
ERROR:
	if (fd > 0) {
		close(fd);
		fd = -1;
	}
	return retCode;
}


#endif


/********/
#define __MAIN_LOOP
#ifdef __MAIN_LOOP

/* ------------------------------------------- */
int main(int argc, char* argv[])
{
	int callStat = 0;
	struct AMCEpoll *base = NULL;
	_LOG("Hello, AMCEpoll!");
	
	base = AMCEpoll_New(1024);
	if (NULL == base) {
		goto END;
	}

	callStat = _create_local_server(base);
	if (callStat < 0) {
		goto END;
	}

	callStat = AMCEpoll_Dispatch(base);
	if (callStat < 0) {
		goto END;
	}
END:
	if (base) {
		AMCEpoll_Free(base);
		base = NULL;
	}
	_LOG("Goodbye, AMCEpoll!");
	return 0;
}

#endif

