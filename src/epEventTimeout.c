/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	epEventTimeout.c
	
	Description: 	
	    This file implements pure timeout event interfaces for AMCEpoll.
			
	History:
		2017-06-14: File created as "epEventFd.c"

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

#include "epEvent.h"
#include "epEventTimeout.h"
#include "utilLog.h"

#include <stdlib.h>
#include <string.h>


int epEventTimeout_AddToBase(struct AMCEpoll *base, struct AMCEpollEvent *event);
int epEventTimeout_GenKey(struct AMCEpollEvent *event, char *keyOut, size_t nBuffLen);
int epEventTimeout_DetachFromBase(struct AMCEpoll *base, struct AMCEpollEvent *event);
int epEventTimeout_Destroy(struct AMCEpollEvent *event);
int epEventTimeout_InvokeCallback(struct AMCEpoll *base, struct AMCEpollEvent *event, int epollEvent, BOOL timeout);

#endif


/********/
#define __INTERNAL_FUNCTIONS
#if 1

/* --------------------_check_timeout_events----------------------- */
static BOOL _check_timeout_events(events_t events)
{
	if (0 == events) {
		return FALSE;
	}
	if (BITS_ANY_SET(events, (EP_EVENT_READ | EP_EVENT_WRITE | EP_EVENT_SIGNAL))) {
		return FALSE;
	}
	if (FALSE == BITS_ALL_SET(events, EP_EVENT_TIMEOUT)) {
		return FALSE;
	}

	return TRUE;
}


/* --------------------_snprintf_timeout_key----------------------- */
static void _snprintf_timeout_key(struct AMCEpollEvent *event, char *str, size_t buffLen)
{
	str[buffLen - 1] = '\0';
	snprintf(str, buffLen - 1, "tm_%ld", (unsigned long)(uintptr_t)event);
	return;
}


#endif

/********/
#define __CLASS_PUBLIC_FUNCTIONS
#if 1

/* --------------------epEventTimeout_Create----------------------- */
struct AMCEpollEvent *epEventTimeout_Create(int fd, events_t events, long timeout, ev_callback callback, void *userData)
{
	struct AMCEpollEvent *newEvent = NULL;
	if (fd >= 0) {
		ERROR("Invalid file descriptor: %d", fd);
		errno = EINVAL;
		goto ENDS;
	}
	if (NULL == callback) {
		ERROR("No callback specified for timeout event");
		errno = EINVAL;
		goto ENDS;
	}
	if (FALSE == _check_timeout_events(events)) {
		ERROR("Invalid events for timeout: 0x%04lx", (long)events);
		errno = EINVAL;
		goto ENDS;
	}

	newEvent = epEventIntnl_NewEmptyEvent();
	if (NULL == newEvent) {
		goto ENDS;
	}

	snprintf(newEvent->description, sizeof(newEvent->description), EVENT_TIMEOUT_DESCRIPTION" %ld", (unsigned long)(uintptr_t)newEvent);
	newEvent->fd = -1;
	newEvent->callback = callback;
	newEvent->user_data = userData;
	newEvent->epoll_events = 0;
	newEvent->timeout = timeout;
	newEvent->events = events;
	newEvent->free_func = epEventTimeout_Destroy;
	newEvent->genkey_func = epEventTimeout_GenKey;
	newEvent->attach_func = epEventTimeout_AddToBase;
	newEvent->detach_func = epEventTimeout_DetachFromBase;
	newEvent->invoke_func = epEventTimeout_InvokeCallback;
	_snprintf_timeout_key(newEvent, newEvent->key, sizeof(newEvent->key));

ENDS:
	return newEvent;
}


/* --------------------epEventTimeout_AddToBase----------------------- */
int epEventTimeout_AddToBase(struct AMCEpoll *base, struct AMCEpollEvent *event)
{
	if ((NULL == base) || (NULL == event)) {
		ERROR("Invalid parameter");
		return ep_err(EINVAL);
	}
	else {
		struct AMCEpollEvent *oldEvent = epEventIntnl_GetEvent(base, event->key);

		/* add a new event */
		if (NULL == oldEvent) {
			int callStat = 0;
			callStat = epEventIntnl_AttachToBase(base, event);
			if (callStat < 0) {
				return ep_err(callStat);
			}
			
			callStat = epEventIntnl_AttachToTimeoutChain(base, event);
			return callStat;
		}
		/* event duplicated */
		else {
			DEBUG("Event %s already existed", event->description);
			return ep_err(AMC_EP_ERR_OBJ_EXISTS);
		}
	}
	/* ends */
}


/* --------------------epEventTimeout_IsTimeoutEvent----------------------- */
BOOL epEventTimeout_IsTimeoutEvent(events_t what)
{
	return _check_timeout_events(what);
}


/* --------------------epEventTimeout_GenKey----------------------- */
int epEventTimeout_GenKey(struct AMCEpollEvent *event, char *keyOut, size_t nBuffLen)
{
	if (event && keyOut && nBuffLen) {
		/* OK */
	} else {
		ERROR("Invalid parameter");
		return ep_err(EINVAL);
	}

	_snprintf_timeout_key(event, keyOut, nBuffLen);
	return 0;
}


/* --------------------epEventTimeout_DetachFromBase----------------------- */
int epEventTimeout_DetachFromBase(struct AMCEpoll *base, struct AMCEpollEvent *event)
{
	if ((NULL == base) || (NULL == event))
	{
		ERROR("Invalid parameter");
		return ep_err(EINVAL);
	}
	else
	{
		int callStat = 0;
		struct AMCEpollEvent *eventInBase = NULL;
		eventInBase = epEventIntnl_GetEvent(base, event->key);
		if (eventInBase != event) {
			ERROR("Event %p is not member of Base %p", event, base);
			return ep_err(AMC_EP_ERR_OBJ_NOT_FOUND);
		}

		callStat = epEventIntnl_DetachFromBase(base, event);
		if (callStat < 0) {
			ERROR("Failed to detach event %s from base", event->description);
			return callStat;
		}

		callStat = epEventIntnl_DetachFromTimeoutChain(base, event);
		return callStat;
	}
}


/* --------------------epEventTimeout_Destroy----------------------- */
int epEventTimeout_Destroy(struct AMCEpollEvent *event)
{
	if (NULL == event) {
		return ep_err(EINVAL);
	}
	else {
		epEventIntnl_InvokeUserFreeCallback(event, event->fd);
		return epEventIntnl_FreeEmptyEvent(event);
	}
}


/* --------------------epEventTimeout_InvokeCallback----------------------- */
int epEventTimeout_InvokeCallback(struct AMCEpoll *base, struct AMCEpollEvent *event, int epollEvent, BOOL timeout)
{
	if (base && event) {
		epEventIntnl_InvokeUserCallback(event, event->fd, EP_EVENT_TIMEOUT);
		return 0;
	}
	else {
		return ep_err(EINVAL);
	}
}


#endif
/* EOF */

