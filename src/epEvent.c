/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	epEvent.c
	
	Description: 	
	    This file implements class "epEvent".
			
	History:
		2017-04-30: File created as "epEvent.c"

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

#include "epEventFd.h"
#include "epEventSignal.h"

#include "epCommon.h"
#include "epEvent.h"
#include "utilLog.h"
#include "cAssocArray.h"

#include <string.h>
#include <stdlib.h>

#endif


/********/
#define __INTERNAL_CLASS_FUNCTIONS
#ifdef __INTERNAL_CLASS_FUNCTIONS

/* --------------------epEventIntnl_NewEmptyEvent----------------------- */
struct AMCEpollEvent *epEventIntnl_NewEmptyEvent()
{
	struct AMCEpollEvent *newEvent = NULL;
	
	newEvent= malloc(sizeof(*newEvent));
	if (NULL == newEvent) {
		CRIT("Failed to allocate a new event: %s", strerror(errno));
	} else {
		memset(newEvent, 0, sizeof(*newEvent));
		newEvent->fd = -1;
	}
	
	return newEvent;
}


/* --------------------epEventIntnl_FreeEmptyEvent----------------------- */
int epEventIntnl_FreeEmptyEvent(struct AMCEpollEvent *event)
{
	free(event);
	return 0;
}


/* --------------------epEventIntnl_AttachToBase----------------------- */
int epEventIntnl_AttachToBase(struct AMCEpoll *base, struct AMCEpollEvent *event)
{
	if (NULL == base) {
		RETURN_ERR(EINVAL);
	} else if (NULL == event) {
		RETURN_ERR(EINVAL);
	} else if ('\0' == event->key[0]) {
		RETURN_ERR(EBADF);
	} else {
		int callStat = cAssocArray_AddValue(base->all_events, event->key, event);
		if (callStat < 0) {
			return (0 - errno);
		} else {
			return 0;
		}
	}
}


/* --------------------epEventIntnl_DetachFromBase----------------------- */
int epEventIntnl_DetachFromBase(struct AMCEpoll *base, struct AMCEpollEvent *event)
{
	if (NULL == base) {
		RETURN_ERR(EINVAL);
	} else if (NULL == event) {
		RETURN_ERR(EINVAL);
	} else if ('\0' == event->key[0]) {
		RETURN_ERR(EBADF);
	} 
	else {
		struct AMCEpollEvent *eventInBase = cAssocArray_GetValue(base->all_events, event->key);

		if (eventInBase == event) {
			int callStat = cAssocArray_RemoveValue(base->all_events, event->key, FALSE);
			if (callStat < 0) {
				return (0 - errno);
			} else {
				return 0;
			}
		}
		else {
			RETURN_ERR(ENOENT);
		}
	}
}


/* --------------------epEventIntnl_InvokeUserCallback----------------------- */
int epEventIntnl_InvokeUserCallback(struct AMCEpollEvent *event, int handler, events_t what)
{
	if (NULL == event) {
		RETURN_ERR(EINVAL);
	} else if (NULL == event->callback) {
		RETURN_ERR(EBADF);
	} else {
		(event->callback)(event, handler, what, event->user_data);
		return 0;
	}
}


/* --------------------epEventIntnl_InvokeUserCallback----------------------- */
int epEventIntnl_InvokeUserFreeCallback(struct AMCEpollEvent *event, int handler)
{
	if (event && event->callback) {
		if (BITS_HAVE_INTRSET(event->events, EP_EVENT_FREE)) {
			(event->callback)(event, handler, EP_EVENT_FREE, event->user_data);
		}
		return 0;
	}
	else {
		RETURN_ERR(EINVAL);
	}
}


/* --------------------epEventIntnl_GetEvent----------------------- */
struct AMCEpollEvent *epEventIntnl_GetEvent(struct AMCEpoll *base, const char *key)
{
	if (base && key) {
		return cAssocArray_GetValue(base->all_events, key);
	} else {
		return NULL;
	}
}


#endif


/********/
#define __PUBLIC_INTERFACES
#ifdef __PUBLIC_INTERFACES

/* --------------------epEvent_New----------------------- */
struct AMCEpollEvent *epEvent_New(int fd, events_t what, long timeout, ev_callback callback, void *userData)
{
	if (epEventFd_IsFileEvent(what)) {
		return epEventFd_Create(fd, what, timeout, callback, userData);
	}
	if (epEventSignal_IsSignalEvent(what)) {
		return epEventSignal_Create(fd, what, timeout, callback, userData);
	}
	// TODO:
	else {
		errno = EINVAL;
		return NULL;
	}
}


/* --------------------epEvent_Free----------------------- */
int epEvent_Free(struct AMCEpollEvent *event)
{
	if (NULL == event) {
		RETURN_ERR(EINVAL);
	}
	else if (NULL == event->free_func) {
		CRIT("Event %p not init correctly", event);
		RETURN_ERR(EBADF);
	}
	else {
		return (event->free_func)(event);
	}
}


/* --------------------epEvent_GetKey----------------------- */
const char * epEvent_GetKey(struct AMCEpollEvent *event)
{
	if (NULL == event) {
		errno = EINVAL;
		return NULL;
	}
	else if ('\0' == event->key) {
		errno = EBADF;
		return NULL;
	}
	else {
		return event->key;
	}
}


/* --------------------epEvent_AddToBase----------------------- */
int epEvent_AddToBase(struct AMCEpoll *base, struct AMCEpollEvent *event)
{
	if (NULL == base) {
		RETURN_ERR(EINVAL);
	} else if (NULL == event) {
		RETURN_ERR(EINVAL);
	} else if ('\0' == event->attach_func) {
		RETURN_ERR(EBADF);
	} else {
		return (event->attach_func)(base, event);
	}
}


/* --------------------epEvent_DelFromBase----------------------- */
int epEvent_DelFromBase(struct AMCEpoll *base, struct AMCEpollEvent *event)
{
	if (NULL == base) {
		RETURN_ERR(EINVAL);
	} else if (NULL == event) {
		RETURN_ERR(EINVAL);
	} else if ('\0' == event->detach_func) {
		RETURN_ERR(EBADF);
	} else {
		return (event->detach_func)(base, event);
	}
}


/* --------------------epEvent_DelFromBaseAndFree----------------------- */
int epEvent_DelFromBaseAndFree(struct AMCEpoll *base, struct AMCEpollEvent *event)
{
	if (NULL == base) {
		RETURN_ERR(EINVAL);
	} else if (NULL == event) {
		RETURN_ERR(EINVAL);
	}
	else {
		int callStat = 0;
		struct AMCEpollEvent *oldEvent = epEvent_GetEvent(base, event->key);
		if (oldEvent == event) {
			callStat = epEvent_DelFromBase(base, event);
		}

		if (0 == callStat) {
			callStat = epEvent_Free(event);
		}

		return callStat;
	}
}


/* --------------------epEvent_InvokeCallback----------------------- */
int epEvent_InvokeCallback(struct AMCEpoll *base, struct AMCEpollEvent *event, int epollEvents)
{
	if (0 == epollEvents) {
		ERROR("Empty event for %p", event);
		RETURN_ERR(EINVAL);
	} else if (NULL == base) {
		RETURN_ERR(EINVAL);
	} else if (NULL == event) {
		RETURN_ERR(EINVAL);
	} else if ('\0' == event->invoke_func) {
		RETURN_ERR(EBADF);
	} else {
		return (event->invoke_func)(base, event, epollEvents);
	}
}


#endif
/* EOF */

