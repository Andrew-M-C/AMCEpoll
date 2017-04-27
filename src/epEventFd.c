/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	epEventFd.c
	
	Description: 	
	    This file implements file descriptor event interfaces for AMCEpoll.
			
	History:
		2017-04-26: File created as "epEventFd.c"

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
#define __INTERNAL_EPOLL_FUNCTIONS
#ifdef __INTERNAL_EPOLL_FUNCTIONS

/* --------------------_epoll_code_from_amc_code----------------------- */
static int _epoll_code_from_amc_code(events_t amcEv)
{
	int ret = 0;
	if (0 == (amcEv & EP_MODE_PERSIST)) {
		ret |= EPOLLONESHOT;
	}
	if (amcEv & EP_MODE_EDGE) {
		ret |= EPOLLET;
	}
	if (amcEv & EP_EVENT_READ) {
		ret |= EPOLLIN | EPOLLPRI;
	}
	if (amcEv & EP_EVENT_WRITE) {
		ret |= EPOLLOUT;
	}
	if (amcEv & EP_EVENT_ERROR) {
		ret |= EPOLLERR | EPOLLHUP;
	}
#if 0
	if (amcEv & EP_EVENT_FREE) {
		/* nothing */
	}
	if (amcEv & EP_EVENT_TIMEOUT) {
		/* nothing */
	}
#endif
	return ret;
}


/* --------------------_epoll_add----------------------- */
static int _epoll_add(struct AMCEpoll *base, struct AMCEpollEvent *amcEvent)
{
	int callStat = 0;
	struct epoll_event epollEvent;
	
	epollEvent.events = _epoll_code_from_amc_code(amcEvent->events);
	epollEvent.data.ptr = amcEvent;

	callStat = epoll_ctl(base->epoll_fd, EPOLL_CTL_ADD, amcEvent->fd, &epollEvent);
	if (0 == callStat) {
		return 0;
	} else {
		int err = errno;
		ERROR("Failed in epoll_add(): %s", strerror(err));
		_RETURN_ERR(err);
	}
}


/* --------------------_epoll_mod----------------------- */
static int _epoll_mod(struct AMCEpoll *base, struct AMCEpollEvent *amcEvent)
{
	int callStat = 0;
	struct epoll_event epollEvent;

	epollEvent.events = _epoll_code_from_amc_code(amcEvent->events);
	epollEvent.data.ptr = amcEvent;

	callStat = epoll_ctl(base->epoll_fd, EPOLL_CTL_MOD, amcEvent->fd, &epollEvent);
	if (0 == callStat) {
		return 0;
	} else {
		int err = errno;
		ERROR("Failed in epoll_mod(): %s", strerror(err));
		_RETURN_ERR(err);
	}
}


/* --------------------_epoll_del----------------------- */
static int _epoll_del(struct AMCEpoll *base, struct AMCEpollEvent *amcEvent)
{
	int callStat = epoll_ctl(base->epoll_fd, EPOLL_CTL_DEL, amcEvent->fd, NULL);
	if (0 == callStat) {
		return 0;
	} else {
		int err = errno;
		ERROR("Failed in epoll_del(): %s", strerror(err));
		_RETURN_ERR(err);
	}
}


#endif


/********/
#define __INTERNAL_NON_EPOLL_FUNCTIONS
#ifdef __INTERNAL_NON_EPOLL_FUNCTIONS

/* --------------------_check_events----------------------- */
static BOOL _check_events(events_t events)
{
	if (0 == events) {
		return FALSE;
	}
	if (0 == (events & (EP_EVENT_READ | EP_EVENT_WRITE))) {
		return FALSE;
	}

	return TRUE;
}


/* --------------------_snprintf_fd_key----------------------- */
static void _snprintf_fd_key(struct AMCEpollEvent *event, char *str, size_t buffLen)
{
	str[buffLen - 1] = '\0';
	snprintf(str, buffLen - 1, "fd_%d", event->fd);
	return;
}


/* --------------------_add_fd_event----------------------- */
static int _add_fd_event(struct AMCEpoll *base, struct AMCEpollEvent *event)
{
	int callStat = 0;
	char key[EVENT_KEY_LEN_MAX];

	_snprintf_fd_key(event, key, sizeof(key));
	DEBUG("Add new event: %s - %p", key, event);

	callStat = epCommon_AddEvent(base, event, key);
	if (callStat < 0) {
		int err = errno;
		ERROR("Failed to add new event: %s", strerror(err));
		_RETURN_ERR(err);
	}

	callStat = _epoll_add(base, event);
	if (0 == callStat) {
		return 0;
	}
	else {
		int err = errno;
		epCommon_DetachEvent(base, key);
		_RETURN_ERR(err);
	}
}


/* --------------------_mod_fd_event----------------------- */
static int _mod_fd_event(struct AMCEpoll *base, struct AMCEpollEvent *event)
{
	return _epoll_mod(base, event);
}


/* --------------------_del_fd_event----------------------- */
static int _del_fd_event(struct AMCEpoll *base, struct AMCEpollEvent *event)
{
	return _epoll_del(base, event);
}


#endif

/********/
#define __PUBLIC_INTERFACES
#ifdef __PUBLIC_INTERFACES

/* --------------------epEventFd_Create----------------------- */
struct AMCEpollEvent *epEventFd_Create(int fd, events_t events, int timeout, ev_callback callback, void *userData)
{
	struct AMCEpollEvent *newEvent = NULL;
	if (fd < 0) {
		ERROR("Invalid file descriptor: %d", fd);
		errno = EINVAL;
		goto ENDS;
	}
	if (NULL == callback) {
		ERROR("No callback specified for file descriptor %d", fd);
		errno = EINVAL;
		goto ENDS;
	}
	if (FALSE == _check_events(events)) {
		ERROR("Invalid events for fd %d: 0x%04lx", fd, (long)events);
		errno = EINVAL;
		goto ENDS;
	}

	newEvent = epCommon_NewEmptyEvent();
	if (NULL == newEvent) {
		goto ENDS;
	}

	newEvent->fd = fd;
	newEvent->callback = callback;
	newEvent->user_data = userData;
	newEvent->inter_data = NULL;
	newEvent->epoll_events = _epoll_code_from_amc_code(events);
	newEvent->events = events;
	newEvent->detach_func = epEventFd_DetachFromBase;

ENDS:
	return newEvent;
}


/* --------------------epEventFd_AddToBase----------------------- */
int epEventFd_AddToBase(struct AMCEpoll *base, struct AMCEpollEvent *event)
{
	if ((NULL == base) || (NULL == event)) {
		ERROR("Invalid parameter");
		_RETURN_ERR(EINVAL);
	}
	else if (FALSE == epEventFd_TypeMatch(event)) {
		ERROR("Not file event");
		_RETURN_ERR(EINVAL);
	}
	else {
		char key[EVENT_KEY_LEN_MAX] = "";
		struct AMCEpollEvent *oldEvent = NULL;

		_snprintf_fd_key(event, key, sizeof(key));
		oldEvent = epCommon_GetEvent(base, key);

		/* add a new event */
		if (NULL == oldEvent) {
			return _add_fd_event(base, event);
		}
		/* update event */
		else if (oldEvent == event) {
			return _mod_fd_event(base, event);
		}
		/* event duplicated */
		else {
			ERROR("Event for %d already existed", event->fd);
			_RETURN_ERR(EEXIST);
		}
	}
	/* ends */
}


/* --------------------epEvendFd_TypeMatch----------------------- */
BOOL epEventFd_TypeMatch(struct AMCEpollEvent *event)
{
	if (NULL == event) {
		return FALSE;
	} else if (0 == (event->events & (EP_EVENT_READ | EP_EVENT_WRITE))) {
		return FALSE;
	} else if (event->fd < 0) {
		return FALSE;
	} else {
		return TRUE;
	}
}


/* --------------------epEventFd_GenKey----------------------- */
int epEventFd_GenKey(struct AMCEpollEvent *event, char *keyOut, size_t nBuffLen)
{
	if (event && keyOut && nBuffLen) {
		/* OK */
	} else {
		ERROR("Invalid parameter");
		_RETURN_ERR(EINVAL);
	}

	if (FALSE == epEventFd_TypeMatch(event)) {
		ERROR("Not file event");
		_RETURN_ERR(EINVAL);
	}

	_snprintf_fd_key(event, keyOut, nBuffLen);
	return 0;
}


/* --------------------epEventFd_DetachFromBase----------------------- */
int epEventFd_DetachFromBase(struct AMCEpoll *base, struct AMCEpollEvent *event)
{
	if (base && event) {
		/* OK */
	} else {
		ERROR("Invalid parameter");
		_RETURN_ERR(EINVAL);
	}

	if (FALSE == epEventFd_TypeMatch(event)) {
		ERROR("Not file event");
		_RETURN_ERR(EINVAL);
	}
	else {
		int callStat = 0;
		char key[EVENT_KEY_LEN_MAX];
		struct AMCEpollEvent *eventInBase = NULL;

		_snprintf_fd_key(event, key, sizeof(key));

		eventInBase = epCommon_GetEvent(base, key);
		if (eventInBase != event) {
			ERROR("Event %p is not menber of Base %p", event, base);
			_RETURN_ERR(ENOENT);
		}

		callStat = _del_fd_event(base, event);
		if (callStat < 0) {
			int err = errno;
			ERROR("Failed to del event in epoll_ctl(): %s", strerror(errno));
			_RETURN_ERR(err);
		}

		if (epCommon_DetachEvent(base, key)) {
			return 0;
		} else {
			_RETURN_ERR(errno);
		}
	}
}


/* --------------------epEventFd_Destroy----------------------- */
int epEventFd_Destroy(struct AMCEpollEvent *event)
{
	if (FALSE == epEventFd_TypeMatch(event)) {
		_RETURN_ERR(EINVAL);
	} else {
		epCommon_InvokeCallback(event, event->fd, EP_EVENT_FREE);
		return epCommon_FreeEmptyEvent(event);
	}
}

#endif
/* EOF */

