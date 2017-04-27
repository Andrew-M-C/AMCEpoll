/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	epEventSignal.c
	
	Description: 	
	    This file implements signal event for AMCEpoll.
			
	History:
		2017-04-27: File created as "epEventSignal.c"

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

/********/
#define __HEADERS_AND_MACROS
#ifdef __HEADERS_AND_MACROS

#include "epEventSignal.h"
#include "utilLog.h"

#include <stdlib.h>
#include <string.h>

#define _RETURN_ERR(err)	\
	do{\
		if (err > 0) {\
			errno = err;\
			return (0 - err);\
		} else if (err < 0) {\
			errno = 0 - err;\
			return err;\
		} else {\
			return -1;\
		}\
	}while(0)

#endif


/********/
#define __INTERNAL_NON_EPOLL_FUNCTIONS
#ifdef __INTERNAL_NON_EPOLL_FUNCTIONS

/* --------------------_snprintf_signal_key----------------------- */
static void _snprintf_signal_key(struct AMCEpollEvent *event, char *str, size_t buffLen)
{
	str[buffLen - 1] = '\0';
	snprintf(str, buffLen - 1, "fd_%d", event->fd);
	return;
}


#endif


/********/
#define __PUBLIC_INTERFACES
#ifdef __PUBLIC_INTERFACES

/* --------------------epEventSignal_Create----------------------- */
struct AMCEpollEvent *epEventSignal_Create(int sig, events_t events, int timeout, ev_callback callback, void *userData)
{
	return NULL;
}


/* --------------------epEventFd_AddToBase----------------------- */
int epEventSignal_AddToBase(struct AMCEpoll *base, struct AMCEpollEvent *event)
{
	return -1;
}


/* --------------------epEventSignal_TypeMatch----------------------- */
BOOL epEventSignal_TypeMatch(struct AMCEpollEvent *event)
{
	if (NULL == event) {
		return FALSE;
	} else if (0 == (event->events & EP_EVENT_SIGNAL)) {
		return FALSE;
	} else if (event->fd < 0) {
		return FALSE;
	} else {
		return TRUE;
	}
}


/* --------------------epEventSignal_GenKey----------------------- */
int epEventSignal_GenKey(struct AMCEpollEvent *event, char *keyOut, size_t nBuffLen)
{
	if (event && keyOut && nBuffLen) {
		/* OK */
	} else {
		ERROR("Invalid parameter");
		_RETURN_ERR(EINVAL);
	}

	if (FALSE == epEventSignal_TypeMatch(event)) {
		ERROR("Not signal event");
		_RETURN_ERR(EINVAL);
	}

	_snprintf_signal_key(event, keyOut, nBuffLen);
	return 0;
}


/* --------------------epEventSignal_DetachFromBase----------------------- */
int epEventSignal_DetachFromBase(struct AMCEpoll *base, struct AMCEpollEvent *event)
{
	return -1;
}


/* --------------------epEventSignal_Destroy----------------------- */
int epEventSignal_Destroy(struct AMCEpollEvent *event)
{
	if (FALSE == epEventSignal_TypeMatch(event)) {
		_RETURN_ERR(EINVAL);
	} else {
		epCommon_InvokeCallback(event, event->fd, EP_EVENT_FREE);
		return epCommon_FreeEmptyEvent(event);
	}
}

#endif
/* EOF */

