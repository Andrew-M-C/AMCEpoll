/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	epCommon.h
	
	Description: 	
	    This file declares common data structures for all events.
			
	History:
		2017-05-02: Remove functions. Those functions is replaced by epEvent.
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

/* headers */
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>

#include "cAssocArray.h"
#include "AMCEpoll.h"

/* constants */
#define EVENT_KEY_LEN_MAX	(32)
#define SIGNAL_NUM_MAX		(64)
#define INTERNAL_DATA_LEN	(64)

/* data structures */
struct AMCEpollEvent;
typedef struct epoll_event epoll_event_st;
typedef int (*free_func)(struct AMCEpollEvent *event);
typedef int (*genkey_func)(struct AMCEpollEvent *event, char *keyBuff, size_t nBuffLen);
typedef int (*attach_func)(struct AMCEpoll *base, struct AMCEpollEvent *event);
typedef int (*detach_func)(struct AMCEpoll *base, struct AMCEpollEvent *event);
typedef int (*invoke_func)(struct AMCEpoll *base, struct AMCEpollEvent *event, int epollEvent);

struct AMCEpollEvent {
	int            fd;
	ev_callback    callback;
	void          *user_data;
	char           key[EVENT_KEY_LEN_MAX];
	uint8_t        inter_data[INTERNAL_DATA_LEN];		/* internal data, reserved for different types of events */
	int            epoll_events;
	events_t       events;
	free_func      free_func;
	genkey_func    genkey_func;
	attach_func    attach_func;
	detach_func    detach_func;
	invoke_func    invoke_func;
};


struct AMCEpoll {
	int             epoll_fd;
	uint32_t        base_status;
	cAssocArray    *all_events;
	size_t          epoll_buff_size;
	epoll_event_st  epoll_buff[0];
};

/* tools */
#define BITS_ANY_SET(val, bits)		(0 != ((val) & (bits)))
#define BITS_ALL_SET(val, bits)		((bits) == ((val) & (bits)))
#define BITS_HAVE_INTRSET(bitA, bitB)	((bitA) != ((bitA) & (~(bitB))))		/* The two bits have intersetion */

/* function */
int ep_err(int err);


#endif
/* EOF */

