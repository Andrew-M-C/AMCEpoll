/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	AMCEpoll.h
	
	Description: 	
	    This file declares main interfaces of AMCEpoll tool.
			
	History:
		2017-04-09: File created as "AMCEpoll.h"

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

#ifndef __AMC_EPOLL_H__
#define __AMC_EPOLL_H__

/********/
/* headers */
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

/********/
/* data types */
struct AMCEpoll;
struct AMCEpollEvent;

#ifndef NULL
#define NULL	((void*)0)
#endif

#ifndef BOOL
#ifndef _DO_NOT_DEFINE_BOOL
#define BOOL	int
#define FALSE	0
#define TRUE	(!(FALSE))
#endif
#endif

/* for uint16_t "events" */
enum {
	EP_MODE_PERSIST  = (1 << 0),	/* only used when adding events */
	EP_MODE_EDGE     = (1 << 1),	/* only used when adding events */

	EP_EVENT_READ    = (1 << 5),
	EP_EVENT_WRITE   = (1 << 6),
	EP_EVENT_ERROR   = (1 << 7),
	EP_EVENT_FREE    = (1 << 8),
	EP_EVENT_TIMEOUT = (1 << 9),
};

/* callback */
typedef void (*ev_callback)(int fd, uint16_t events, void *arg);

/********/
/* functions */
struct AMCEpoll *
	AMCEpoll_New(size_t buffSize);
int
	AMCEpoll_Free(struct AMCEpoll *obj);
int 
	AMCEpoll_AddEvent(struct AMCEpoll *obj, int fd, uint16_t events, int timeout, ev_callback callback, void *userData, struct AMCEpollEvent **eventOut);
int 
	AMCEpoll_DelEvent(struct AMCEpoll *obj, struct AMCEpollEvent *event);
int 
	AMCEpoll_DelEventByFd(struct AMCEpoll *obj, int fd);
int 
	AMCEpoll_Dispatch(struct AMCEpoll *obj);
int 
	AMCEpoll_LoopExit(struct AMCEpoll *obj);


#endif
/* EOF */

