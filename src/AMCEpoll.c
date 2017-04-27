/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	AMCEpoll.c
	
	Description: 	
	    This file contains mail logic of AMCEpoll.h.
			
	History:
		2017-04-08: File created as "AMCEpoll.c"

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
#define __HEADERS
#ifdef __HEADERS

#include "epCommon.h"
#include "epEventFd.h"
#include "utilLog.h"
#include "AMCEpoll.h"
#include "cAssocArray.h"

#include <sys/epoll.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>

#endif


/********/
#define __DATA_DEFINITIONS
#ifdef __DATA_DEFINITIONS

/* for uint32_t "base_status" in AMCEpoll */
enum {
	EP_STAT_SHOULD_EXIT = (1 << 0),
	EP_STAT_EPOLL_ERROR = (1 << 1),
	EP_STAT_RUNNING =     (1 << 2),
	/* for future use */
};

#define MAX_FD_STR_LEN		(16)
#define EP_EVENT_ALL_MASK	(\
	EP_MODE_PERSIST | EP_MODE_EDGE | EP_EVENT_READ | \
	EP_EVENT_WRITE | EP_EVENT_ERROR | EP_EVENT_FREE | \
	EP_EVENT_TIMEOUT\
	)

#define _RETURN_ERRNO()	\
	do{\
		int err = errno;\
		if ((err) > 0) {\
			return (0 - (err));\
		} else {\
			return -1;\
		}\
	}while(0)

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
#define __AMC_EPOLL_MAIN_LOOP
#ifdef __AMC_EPOLL_MAIN_LOOP

/* --------------------_dispatch_main_loop----------------------- */
static int _dispatch_main_loop(struct AMCEpoll *base)
{
	struct epoll_event *evBuff = base->epoll_buff;
	int evFd = base->epoll_fd;
	int evSize = base->epoll_buff_size;
	int nTotal = 0;
	int nIndex = 0;
	int errCpy = 0;
	BOOL shouldExit = FALSE;

	base->base_status = 0;
	base->base_status |= EP_STAT_RUNNING;

	/* This is actually a thread-like process */
	do {
		nTotal = epoll_wait(evFd, evBuff, evSize, 1000);		// TODO: implement timeout
		errCpy = errno;
		if (nTotal < 0) {
			if (EINTR == errCpy) {
				// TODO: Support signal events
			}
			else {
				ERROR("Failed in epoll_wait(): %s", strerror(errCpy));
				base->base_status |= EP_STAT_EPOLL_ERROR;
				shouldExit = TRUE;
			}
		}
		else if (0 == nTotal) {
			// TODO: Add support
			DEBUG("Enjoy your peace...");
		}
		else {
			DEBUG("%d event(s) active", nTotal);
			int epollWhat = 0;
			struct AMCEpollEvent *amcEvent = NULL;
			for (nIndex = 0; nIndex < nTotal; nIndex ++)
			{
				epollWhat = evBuff[nIndex].events;
				amcEvent = (struct AMCEpollEvent *)(evBuff[nIndex].data.ptr);

				if (amcEvent)
				{
					events_t amcWhat = epCommon_EventCodeEpollToAmc(epollWhat);
					epCommon_InvokeCallback(amcEvent, amcEvent->fd, amcWhat);

					if (0 == (amcEvent->events & EP_MODE_PERSIST)) {
						epEventFd_DetachFromBase(base, amcEvent);
						epEventFd_Destroy(amcEvent);
						amcEvent = NULL;
					}
				}
			}
		}
		// end of "else (nTotal < 0) {..."

	} while (FALSE == shouldExit);
	// end of "do - while (FALSE == shouldExit)"

	/* clean status */
	base->base_status &= ~(EP_STAT_SHOULD_EXIT | EP_STAT_RUNNING);

	/* return */
	if (base->base_status & EP_STAT_EPOLL_ERROR) {
		_RETURN_ERR(errCpy);
	}
	else {
		return 0;
	}
}


#endif


/********/
#define __PUBLIC_FUNCTIONS
#ifdef __PUBLIC_FUNCTIONS

/* --------------------AMCEpoll_New----------------------- */
struct AMCEpoll *AMCEpoll_New(size_t buffSize)
{
	if (buffSize <= 0) {
		ERROR("Invalid size %d", buffSize);
		errno = EINVAL;
		return NULL;
	}
	else {
		struct AMCEpoll *ret;
		size_t objLen = sizeof(*ret) + sizeof(epoll_event_st) * buffSize;
		BOOL isOK = TRUE;

		/* malloc */
		ret = malloc(objLen);
		if (NULL == ret) {
			isOK = FALSE;
		} else {
			memset(ret, 0, objLen);
			ret->epoll_buff_size = buffSize;
		}

		/* epoll_create */
		if (isOK) {
			ret->epoll_fd = epoll_create(buffSize);
			if (ret->epoll_fd < 0) {
				isOK = FALSE;
			}
		}

		/* aAssocArray */
		if (isOK) {
			ret->all_events = cAssocArray_Create(FALSE);
			if (NULL == ret->all_events) {
				isOK = FALSE;
			}
		}

		/* return */
		if (FALSE == isOK) {
			if (ret) {
				AMCEpoll_Free(ret);
				ret = NULL;
			}
		}
		return ret;
	}
}


/* --------------------AMCEpoll_Free----------------------- */
int AMCEpoll_Free(struct AMCEpoll *base)
{
	if (NULL == base) {
		ERROR("Nil parameter");
		_RETURN_ERR(EINVAL);
	}
	else
	{
		if (base->all_events)
		{
			cArrayKeys *allKeys = cAssocArray_GetKeys(base->all_events);
			cArrayKeys *eachKey = allKeys;
			struct AMCEpollEvent *eachEvent = NULL;
			int callStat = 0;

			while (eachKey)
			{
				eachEvent = epCommon_GetEvent(base, eachKey->key);
				if (eachEvent) {
					if (NULL == eachEvent->detach_func) {
						CRIT("No detach function defined for event %p", eachEvent);
					} else {
						callStat = (eachEvent->detach_func)(base, eachEvent);
						if (callStat < 0) {
							CRIT("Failed to detach event %p: %s", eachEvent, strerror(errno));
						}
						callStat = epCommon_FreeEmptyEvent(eachEvent);
						if (callStat < 0) {
							CRIT("Failed to free event %p: %s", eachEvent, strerror(errno));
						}
					}
				}

				eachKey = eachKey->next;
			}

			cAssocArray_Delete(base->all_events, TRUE);
			base->all_events = NULL;

			if (allKeys) {
				cArrayKeys_Free(allKeys);
				allKeys = NULL;
			}
		}

		if (base->epoll_fd > 0) {
			close(base->epoll_fd);
			base->epoll_fd = -1;
		}

		free(base);
		base = NULL;

		return 0;
	}
}


/* --------------------AMCEpoll_NewFileEvent----------------------- */
struct AMCEpollEvent *AMCEpoll_NewFileEvent(int fd, events_t events, int timeout, ev_callback callback, void *userData)
{
	struct AMCEpollEvent *newEvent = NULL;

	if (epCommon_IsSignalEvent(events)) {
		// TODO:
		return NULL;
	}
	else if (epCommon_IsTimeoutEvent(events)) {
		// TODO:
		return NULL;
	}
	else if (epCommon_IsFileEvent(events)) {
		newEvent = epEventFd_Create(fd, events, timeout, callback, userData);
		return newEvent;
	}
	else {
		ERROR("Illegal event request: 0x%04lx", (long)events);
		return NULL;
	}
}


/* --------------------AMCEpoll_FreeEvent----------------------- */
int AMCEpoll_FreeEvent(struct AMCEpollEvent *event)
{
	if (NULL == event) {
		_RETURN_ERR(EINVAL);
	}
	else if (epEventFd_TypeMatch(event)) {
		return epEventFd_Destroy(event);
	}
	else {
		ERROR("Unknown event type: 0x%04lx", event->events);
		_RETURN_ERR(EINVAL);
	}
}


/* --------------------AMCEpoll_AddEvent----------------------- */
int AMCEpoll_AddEvent(struct AMCEpoll * base, struct AMCEpollEvent * event)
{
	if ((NULL == base) || (NULL == event)) {
		ERROR("Invalid parameter in AMCEpoll_AddEvent()");
		_RETURN_ERR(EINVAL);
	}
	else if (epEventFd_TypeMatch(event)) {
		return epEventFd_AddToBase(base, event);
	}
	else {
		// TODO:
		ERROR("Unknown event type 0x%04lx", event->events);
		_RETURN_ERR(EINVAL);
	}
}


/* --------------------AMCEpoll_DelEvent----------------------- */
int AMCEpoll_DelEvent(struct AMCEpoll * base, struct AMCEpollEvent * event)
{
	int callStat = 0;

	if ((NULL == base) || (NULL == event)) {
		ERROR("Invalid parameter in AMCEpoll_DelEvent()");
		_RETURN_ERR(EINVAL);
	}
	else if (event->detach_func) {
		callStat = (event->detach_func)(base, event);
		return callStat;
	}
	else {
		// TODO:
		ERROR("Unknown event type 0x%04lx", event->events);
		_RETURN_ERR(EINVAL);
	}
}


/* --------------------AMCEpoll_DelAndFreeEvent----------------------- */
int AMCEpoll_DelAndFreeEvent(struct AMCEpoll *base, struct AMCEpollEvent *event)
{
	int callStat = 0;

	callStat = AMCEpoll_DelEvent(base, event);
	if (0 == callStat) {
		callStat = AMCEpoll_FreeEvent(event);
	}

	return callStat;
}


/* --------------------AMCEpoll_DelEvent----------------------- */
int AMCEpoll_Dispatch(struct AMCEpoll *obj)
{
	if (NULL == obj) {
		ERROR("Nil parameter");
		_RETURN_ERR(EINVAL);
	} 
	else if (0 == cAssocArray_Size(obj->all_events)) {
		return 0;
	}
	else {
		return _dispatch_main_loop(obj);
	}
}


/* --------------------AMCEpoll_LoopExit----------------------- */
int AMCEpoll_LoopExit(struct AMCEpoll *obj)
{
	if (NULL == obj) {
		ERROR("Nil parameter");
		_RETURN_ERR(EINVAL);
	} else {
		obj->base_status |= EP_STAT_SHOULD_EXIT;
		return 0;
	}
}


/* --------------------AMCFd_MakeNonBlock----------------------- */
int AMCFd_MakeNonBlock(int fd)
{
	if (fd < 0) {
		ERROR("Invalid file descriptor");
		_RETURN_ERR(EINVAL);
	}
	else {
		int flags = fcntl(fd, F_GETFL, NULL);
		flags = fcntl(fd, F_SETFL, (flags | O_NONBLOCK));
		if (0 == flags) {
			return 0;
		} else {
			int err = errno;
			ERROR("Failed to set O_NONBLOCK for fd %d: %s", fd, strerror(err));
			_RETURN_ERR(err);
		}
	}
}


/* --------------------AMCFd_MakeCloseOnExec----------------------- */
int AMCFd_MakeCloseOnExec(int fd)
{
	if (fd < 0) {
		ERROR("Invalid file descriptor");
		_RETURN_ERR(EINVAL);
	}
	else {
		int flags = fcntl(fd, F_GETFD, NULL);
		flags = fcntl(fd, F_SETFD, (flags | FD_CLOEXEC));
		if (0 == flags) {
			return 0;
		} else {
			int err = errno;
			ERROR("Failed to set FD_CLOEXEC for fd %d: %s", fd, strerror(err));
			_RETURN_ERR(err);
		}
	}
}


/* --------------------AMCFd_Read----------------------- */
ssize_t AMCFd_Read(int fd, void *rawBuf, size_t nbyte)
{
	int err = 0;
	ssize_t callStat = 0;
	ssize_t ret = 0;
	uint8_t *buff = (uint8_t *)rawBuf;
	BOOL isDone = FALSE;

	if (fd < 0) {
		_RETURN_ERR(EBADF);
	}
	if (NULL == buff) {
		_RETURN_ERR(EINVAL);
	}
	if (0 == nbyte) {
		return 0;
	}

	/* loop read */
	do {
		callStat = read(fd, buff + ret, nbyte - ret);
		err = errno;

		if (0 == callStat) {
			/* EOF */
			DEBUG("Fd %d EOF", fd);
			//ret = 0;
			isDone = TRUE;
		}
		else if (callStat < 0)
		{
			if (EINTR == err) {
				DEBUG("Fd %d EINTR", fd);
				/* continue */
			} else if (EAGAIN == err) {
				DEBUG("Fd %d EAGAIN", fd);
				isDone = TRUE;
			} else {
				DEBUG("Fd %d error in read(): %s", strerror(err));
				ret = -1;
				isDone = TRUE;
			}
		}
		else
		{
			ret += callStat;

			if (ret >= nbyte) {
				isDone = TRUE;
			}
		}
	} while (FALSE == isDone);
	// end of "while (FALSE == isDone)"

	return ret;
}


/* --------------------AMCFd_Write----------------------- */
ssize_t AMCFd_Write(int fd, const void *buff, size_t nbyte)
{
	int err = 0;
	ssize_t ret = 0;
	ssize_t callStat = 0;
	BOOL isDone = FALSE;

	if (fd < 0) {
		_RETURN_ERR(EBADF);
	}
	if (NULL == buff) {
		_RETURN_ERR(EINVAL);
	}
	if (0 == nbyte) {
		return 0;
	}

	do {
		callStat = write(fd, buff + ret, nbyte - ret);
		if (callStat < 0) {
			err = errno;
			if (EINTR == errno) {
				/* continue */
			} else if (EAGAIN == errno) {
				isDone = TRUE;
			} else {
				DEBUG("Fd %d error in write(): %s", strerror(err));
				ret = (ret > 0) ? ret : -1;
				isDone = TRUE;
			}
		}
		else {
			ret += callStat;
			if (ret >= nbyte) {
				isDone = TRUE;
			}
		}
	}
	while (FALSE == isDone);

	return ret; 
}



#endif


/* EOF */

