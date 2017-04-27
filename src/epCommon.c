/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	epCommon.c
	
	Description: 	
	    This file implements common functions for all event types.
			
	History:
		2017-04-26: File created as "epCommon.c"

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

#include "epCommon.h"
#include "utilLog.h"
#include "cAssocArray.h"

#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>

#endif


/********/
#define __PUBLIC_INTERFACES
#ifdef __PUBLIC_INTERFACES

/* --------------------epCommon_NewEmptyEvent----------------------- */
struct AMCEpollEvent *epCommon_NewEmptyEvent()
{
	struct AMCEpollEvent *newEvent = NULL;
	
	newEvent= malloc(sizeof(*newEvent));
	if (NULL == newEvent) {
		CRIT("Failed to allocate a new event: %s", strerror(errno));
	} else {
		newEvent->fd = -1;
		newEvent->callback = NULL;
		newEvent->user_data = NULL;
		newEvent->inter_data = NULL;
		newEvent->epoll_events = 0;
		newEvent->events = 0;
		newEvent->detach_func = NULL;
	}
	
	return newEvent;
}


/* --------------------epCommon_FreeEmptyEvent----------------------- */
int epCommon_FreeEmptyEvent(struct AMCEpollEvent *event)
{
	if (NULL == event) {
		errno = EINVAL;
		return -EINVAL;
	}
	else {
		free(event);
		return 0;
	}
}


/* --------------------epCommon_GetEvent----------------------- */
struct AMCEpollEvent *epCommon_GetEvent(struct AMCEpoll *base, const char *key)
{
	if (base && key) {
		return cAssocArray_GetValue(base->all_events, key);
	}
	else {
		errno = EINVAL;
		return NULL;
	}
}


/* --------------------epCommon_AddEvent----------------------- */
int epCommon_AddEvent(struct AMCEpoll *base, struct AMCEpollEvent *event, const char *key)
{
	if (base && event && key) {
		return cAssocArray_AddValue(base->all_events, key, event);
	}
	else {
		errno = EINVAL;
		return -EINVAL;
	}
}


/* --------------------epCommon_DetachEvent----------------------- */
struct AMCEpollEvent *epCommon_DetachEvent(struct AMCEpoll *base, const char *key)
{
	if (base && key)
	{
		struct AMCEpollEvent *event = cAssocArray_GetValue(base->all_events, key);
		if (event) {
			cAssocArray_RemoveValue(base->all_events, key, FALSE);
		}
		return event;
	}
	else {
		errno = EINVAL;
		return NULL;
	}
}


/* --------------------epCommon_InvokeCallback----------------------- */
int epCommon_InvokeCallback(struct AMCEpollEvent *event, int fdOrSig, events_t eventCodes)
{
	if (NULL == event) {
		errno = EINVAL;
		return -EINVAL;
	}
	if (0 == eventCodes) {
		errno = EINVAL;
		return -EINVAL;
	}

	/* Do any bit mask set? */
	if (BITS_HAVE_INTRSET(event->events, eventCodes)) {
		(event->callback)(event, fdOrSig, eventCodes, event->user_data);
	}
	return 0;
}


/* --------------------epCommon_IsFileEvent----------------------- */
BOOL epCommon_IsFileEvent(events_t events)
{
	return BITS_ANY_SET(events, (EP_EVENT_READ | EP_EVENT_WRITE));
}


/* --------------------epCommon_IsTimeoutEvent----------------------- */
BOOL epCommon_IsTimeoutEvent(events_t events)
{
	if (epCommon_IsFileEvent(events)) {
		return FALSE;
	} else {
		return BITS_ANY_SET(events, EP_EVENT_TIMEOUT);
	}
}


/* --------------------epCommon_IsSignalEvent----------------------- */
BOOL epCommon_IsSignalEvent(events_t events)
{
	// TODO:
	return FALSE;
}


/* --------------------epCommon_IsSignalEvent----------------------- */
events_t epCommon_EventCodeEpollToAmc(int epollEvents)
{
	events_t amcEvents = 0;

	if (epollEvents & EPOLLERR) {
		amcEvents |= EP_EVENT_ERROR;
	}
	if (epollEvents & EPOLLHUP) {
		amcEvents |= EP_EVENT_ERROR;
	}
	if ((epollEvents & EPOLLIN) || (epollEvents & EPOLLPRI)) {
		amcEvents |= EP_EVENT_READ;
	}
	if (epollEvents & EPOLLOUT) {
		amcEvents |= EP_EVENT_WRITE;
	}

	return amcEvents;
}


#endif
/* EOF */

