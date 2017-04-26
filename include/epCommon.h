/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	epCommon.h
	
	Description: 	
	    This file declares internal data structures for all events.
			
	History:
		2017-04-26: File created as "epCommon.h"

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

#ifndef __EP_COMMON_H__
#define __EP_COMMON_H__

#include <stdint.h>
#include <unistd.h>
#include <sys/epoll.h>

#include "cAssocArray.h"
#include "AMCEpoll.h"

typedef struct epoll_event epoll_event_st;

struct AMCEpollEvent {
	int            fd;
	ev_callback    callback;
	void          *user_data;
	void          *inter_data;
	int            epoll_events;
	events_t       events;
};


struct AMCEpoll {
	int             epoll_fd;
	uint32_t        base_status;
	cAssocArray    *all_events;
	size_t          epoll_buff_size;
	epoll_event_st  epoll_buff[0];
};

#define EVENT_KEY_LEN_MAX	(32)


struct AMCEpollEvent *
	epCommon_NewEmptyEvent(void);
int 
	epCommon_FreeEmptyEvent(struct AMCEpollEvent *event);
struct AMCEpollEvent *
	epCommon_GetEvent(struct AMCEpoll *base, const char *key);
int 
	epCommon_AddEvent(struct AMCEpoll *base, struct AMCEpollEvent *event, const char *key);
struct AMCEpollEvent * 
	epCommon_DetachEvent(struct AMCEpoll *base, const char *key);
int 
	epCommon_InvokeCallback(struct AMCEpollEvent *event, int fdOrSig, events_t eventCodes);

#endif
/* EOF */

