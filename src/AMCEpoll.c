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

#include "epEvent.h"
#include "utilLog.h"
#include "AMCEpoll.h"
#include "cAssocArray.h"
#include "utilTimeout.h"

#include <sys/epoll.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

#endif


/********/
#define __AMC_EPOLL_MAIN_LOOP
#ifdef __AMC_EPOLL_MAIN_LOOP

/* --------------------_loop_get_timeout_msec----------------------- */
static long _loop_get_timeout_msec(struct AMCEpoll *base)
{
	long ret = 0;
	ret = utilTimeout_MinimumSleepMilisecs(&(base->all_timeouts));
	if (ret < 0) {
		DEBUG("No timeout events");
		ret = 1000;
	}
	else if (0 == ret) {
		DEBUG("Next timeout event in 1 msec");
		ret = 1;
	}
	else {
		DEBUG("Next timeout event in %ld msec(s), now %04ld.%09ld\n\n\n", ret, utilTimeout_GetSysupTime().tv_sec, utilTimeout_GetSysupTime().tv_nsec);
		ret += 1;
	}
	return ret;
}


/* --------------------_loop_handle_error----------------------- */
static void _loop_handle_error(struct AMCEpoll *base, int error)
{
	if (EINTR == error) {
		/* nothing needs to be done */
	}
	else {
		ERROR("Failed in epoll_wait(): %s", strerror(error));
		BITS_SET(base->base_status, EP_STAT_EPOLL_ERROR);
	}
	
	return;
}


/* --------------------_dispatch_main_loop----------------------- */
static void _loop_handle_timeout(struct AMCEpoll *base)
{
	struct timespec sysTime = utilTimeout_GetSysupTime();
	struct timespec minTime = {0};
	struct AMCEpollEvent *event = NULL;
	int callStat = 0;
	int compare = 0;
	BOOL isDone = FALSE;

	//utilTimeout_Debug(&(base->all_timeouts));

	do {
		callStat = utilTimeout_GetSmallestTime(&(base->all_timeouts), &minTime, &event);
		if (callStat < 0) {
			DEBUG("<%04ld.%09ld> Enjoy your peace...", (long)(sysTime.tv_sec), (long)(sysTime.tv_nsec));
			isDone = TRUE;
		}
		else {
			compare = utilTimeout_CompareTime(&minTime, &sysTime);
			if (compare <= 0)
			{
				DEBUG("<%04ld.%09ld> %s timeout at %04ld.%09ld", 
						(long)(sysTime.tv_sec), (long)(sysTime.tv_nsec),
						event->description, 
						(long)(minTime.tv_sec), (long)(minTime.tv_nsec));
			
				/* invoke callback */
				epEvent_DetachTimeout(base, event);
				epEvent_InvokeCallback(base, event, 0, TRUE);

				/* add again */
				if (BITS_ALL_SET(event->status, EpEvStat_FreeLater)) {
					epEvent_Free(event);
					event = NULL;
				}
				else if (FALSE == BITS_ALL_SET(event->events, EP_MODE_PERSIST)) {
					epEvent_DelFromBase(base, event);
				}
				else {
					if (utilTimeout_ObjectExists(&(base->all_timeouts), event))
					{
						/* timeout already added by user. Nothing more needs to be done */
					}
					else if ((base == event->owner) &&
							BITS_ALL_SET(event->events, EP_EVENT_TIMEOUT))
					{
						/* User not added timeout event, let us add it for programmer. */
						epEvent_AttachTimeout(base, event);
					}
					else {
						/* event is detatched from base */
					}
				}

			}
			else {
				isDone = TRUE;
			}
		}
	}
	while (FALSE == isDone);

	return;
}


/* --------------------_dispatch_main_loop----------------------- */
static void _loop_handle_events(struct AMCEpoll *base, struct epoll_event *evBuff, size_t nTotal)
{
	int epollWhat = 0;
	size_t nIndex = 0;
	struct AMCEpollEvent *amcEvent = NULL;

	DEBUG("%d event(s) active", nTotal);

	for (nIndex = 0; nIndex < nTotal; nIndex ++)
	{
		epollWhat = evBuff[nIndex].events;
		amcEvent = (struct AMCEpollEvent *)(evBuff[nIndex].data.ptr);

		if (amcEvent)
		{
			if (BITS_ALL_SET(amcEvent->events, EP_EVENT_TIMEOUT)) {
				epEvent_DetachTimeout(base, amcEvent);
			}
			epEvent_InvokeCallback(base, amcEvent, epollWhat, FALSE);

			if (BITS_ALL_SET(amcEvent->status, EpEvStat_FreeLater)) {
				epEvent_Free(amcEvent);
				amcEvent = NULL;
			}
			else if (FALSE == BITS_ALL_SET(amcEvent->events, EP_MODE_PERSIST)) {
				epEvent_DelFromBase(base, amcEvent);
			}
			else {
				if (utilTimeout_ObjectExists(&(base->all_timeouts), amcEvent))
				{
					/* timeout already added by user. Nothing more needs to be done */
				}
				else if ((base == amcEvent->owner) &&
						BITS_ALL_SET(amcEvent->events, EP_EVENT_TIMEOUT))
				{
					/* User not added timeout event, let us add it for programmer. */
					epEvent_AttachTimeout(base, amcEvent);
				}
				else {
					/* event is detatched from base */
				}
			}
		}
		// end of "if (amcEvent)"
	}
	// end of "for (nIndex = 0; nIndex < nTotal; nIndex ++)"
}


/* --------------------_dispatch_main_loop----------------------- */
static int _dispatch_main_loop(struct AMCEpoll *base)
{
	struct epoll_event *evBuff = base->epoll_buff;
	int evFd = base->epoll_fd;
	int evSize = base->epoll_buff_size;
	int nTotal = 0;
	long waitMilisec = 0;
	BOOL shouldExit = FALSE;

	base->base_status = 0;
	BITS_SET(base->base_status, EP_STAT_RUNNING);

	/* This is actually a thread-like process */
	while (FALSE == shouldExit)
	{
		/******/
		/* invoke epoll */
		waitMilisec = _loop_get_timeout_msec(base);
		nTotal = epoll_wait(evFd, evBuff, evSize, waitMilisec);

		/******/
		/* handle epoll */
		if (nTotal < 0) {
			_loop_handle_error(base, errno);
		} else if (0 == nTotal) {
			_loop_handle_timeout(base);
		} else {
			_loop_handle_events(base, evBuff, nTotal);
			_loop_handle_timeout(base);
		}
		// end of "else (nTotal < 0) {..."

		/******/
		/* main loop status check */
		if (BITS_ANY_SET(base->base_status, EP_STAT_SHOULD_EXIT | EP_STAT_EPOLL_ERROR))
		{
			shouldExit = TRUE;
		}
	}
	// end of "while (FALSE == shouldExit)"

	/* clean status */
	BITS_CLR(base->base_status, EP_STAT_SHOULD_EXIT | EP_STAT_RUNNING);

	/* return */
	if (BITS_ANY_SET(base->base_status, EP_STAT_EPOLL_ERROR))
	{
		BITS_CLR(base->base_status, EP_STAT_EPOLL_ERROR);
		return ep_err(errno);
	}
	else {
		return ep_err(0);
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

		/* timeout chain */
		if (isOK) {
			int callStat = utilTimeout_Init(&(ret->all_timeouts));
			if (callStat < 0) {
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
		return ep_err(EINVAL);
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
				eachEvent = epEvent_GetEvent(base, eachKey->key);
				if (eachEvent) {
					if (NULL == eachEvent->detach_func) {
						CRIT("No detach function defined for event %p", eachEvent);
					} else {
						callStat = (eachEvent->detach_func)(base, eachEvent);
						if (callStat < 0) {
							CRIT("Failed to detach event %p: %s", eachEvent, strerror(errno));
						}
						callStat = epEvent_DelFromBaseAndFree(base, eachEvent);
						if (callStat < 0) {
							CRIT("Failed to free event %p: %s", eachEvent, strerror(errno));
						}
					}
				}

				eachKey = eachKey->next;
			}

			cAssocArray_Delete(base->all_events, TRUE);
			base->all_events = NULL;

			utilTimeout_Clean(&(base->all_timeouts));

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


/* --------------------AMCEpoll_NewEvent----------------------- */
struct AMCEpollEvent *AMCEpoll_NewEvent(int fd, events_t events, long timeout, ev_callback callback, void *userData)
{
	return epEvent_New(fd, events, timeout, callback, userData);
}


/* --------------------AMCEpoll_FreeEvent----------------------- */
int AMCEpoll_FreeEvent(struct AMCEpollEvent *event)
{
	return epEvent_Free(event);
}


/* --------------------AMCEpoll_AddEvent----------------------- */
int AMCEpoll_AddEvent(struct AMCEpoll * base, struct AMCEpollEvent * event)
{
	return epEvent_AddToBase(base, event);
}


/* --------------------AMCEpoll_DelEvent----------------------- */
int AMCEpoll_DelEvent(struct AMCEpoll * base, struct AMCEpollEvent * event)
{
	return epEvent_DelFromBase(base, event);
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


/* --------------------AMCEpoll_SetEventTimeout----------------------- */
int AMCEpoll_SetEventTimeout(struct AMCEpoll *base, struct AMCEpollEvent *event, long timeout)
{
	if ((NULL == base) || (NULL == event)) {
		return ep_err(EINVAL); 
	}

	if (event->timeout == timeout) {
		return ep_err(0);
	}
	else {
		event->timeout = timeout;
		return AMCEpoll_AddEvent(base, event);
	}
}


/* --------------------AMCEpoll_SetEventTimeout----------------------- */
long AMCEpoll_GetEventTimeout(struct AMCEpoll *base, struct AMCEpollEvent *event)
{
	if (event) {
		return event->timeout;
	}
	else {
		return -1;
	}
}


/* --------------------AMCEpoll_Dispatch----------------------- */
int AMCEpoll_Dispatch(struct AMCEpoll *base)
{
	if (NULL == base) {
		ERROR("Nil parameter");
		return ep_err(EINVAL);
	} 
	else if (0 == cAssocArray_Size(base->all_events)) {
		return 0;
	}
	else {
		return _dispatch_main_loop(base);
	}
}


/* --------------------AMCEpoll_LoopExit----------------------- */
int AMCEpoll_LoopExit(struct AMCEpoll *base)
{
	if (NULL == base) {
		ERROR("Nil parameter");
		return ep_err(EINVAL);
	} else {
		base->base_status |= EP_STAT_SHOULD_EXIT;
		return 0;
	}
}


/* --------------------AMCEpoll_StrError----------------------- */
const char *AMCEpoll_StrError(int error)
{
	static const char *errStr[AMC_EP_ERR_BOUNDARY - AMC_EP_ERR_UNKNOWN + 1] = {
		"Unknown error beyond AMCEpoll",
		"Event callback not initialized",
		"Event key not initialized",
		"Event class methods not initialized",
		"Base timeout chain not initialized",
		"Event not found",
		"Event already exists",
		"Epoll event codes empty",
		
		"Illegal AMCEpoll error"	// SHOULD placed in the end
	};

	if (0 == error) {
		return "success";
	}
	if (-1 == error) {
		return strerror(errno);
	}

	if (error < 0) {
		error = -error;
	}

	if (error <= RB_ERR_BOUNDARY) {
		return utilRbTree_StrError(error);
	}
	else if (error < AMC_EP_ERR_UNKNOWN) {
		return errStr[0];
	}
	else if (error >= AMC_EP_ERR_BOUNDARY) {
		return errStr[0];
	}
	else {
		return errStr[error - AMC_EP_ERR_UNKNOWN];
	}
}


/* --------------------AMCFd_MakeNonBlock----------------------- */
int AMCFd_MakeNonBlock(int fd)
{
	if (fd < 0) {
		ERROR("Invalid file descriptor");
		return ep_err(EINVAL);
	}
	else {
		int flags = fcntl(fd, F_GETFL, NULL);
		flags = fcntl(fd, F_SETFL, (flags | O_NONBLOCK));
		if (0 == flags) {
			return ep_err(0);
		}
		else {
			ERROR("Failed to set O_NONBLOCK for fd %d: %s", fd, strerror(errno));
			return ep_err(errno);
		}
	}
}


/* --------------------AMCFd_MakeCloseOnExec----------------------- */
int AMCFd_MakeCloseOnExec(int fd)
{
	if (fd < 0) {
		ERROR("Invalid file descriptor");
		return ep_err(EINVAL);
	}
	else {
		int flags = fcntl(fd, F_GETFD, NULL);
		flags = fcntl(fd, F_SETFD, (flags | FD_CLOEXEC));
		if (0 == flags) {
			return ep_err(0);
		}
		else {
			ERROR("Failed to set FD_CLOEXEC for fd %d: %s", fd, strerror(errno));
			return ep_err(errno);
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
		return ep_err(EBADF);
	}
	if (NULL == buff) {
		return ep_err(EINVAL);
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
		return ep_err(EBADF);
	}
	if (NULL == buff) {
		return ep_err(EINVAL);
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


/* --------------------AMCFd_SendTo----------------------- */
ssize_t AMCFd_SendTo(int fd, const void *buff, size_t nbyte, int flags, const struct sockaddr *to, socklen_t tolen)
{
	int err = 0;
	ssize_t ret = 0;
	ssize_t callStat = 0;
	BOOL isDone = FALSE;

	if (fd < 0) {
		return ep_err(EBADF);
	}
	if (NULL == buff) {
		return ep_err(EINVAL);
	}
	if (0 == nbyte) {
		return 0;
	}

	do {
		callStat = sendto(fd, buff + ret, nbyte - ret, flags, to, tolen);
		if (callStat < 0) {
			err = errno;
			if (EINTR == errno) {
				/* continue */
			} else if (EAGAIN == errno) {
				isDone = TRUE;
			} else {
				DEBUG("Fd %d error in sendto(): %s", strerror(err));
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


/* --------------------AMCFd_RecvFrom----------------------- */
ssize_t AMCFd_RecvFrom(int fd, void *rawBuf, size_t nbyte, int flags, struct sockaddr *from, socklen_t *fromlen)
{
	int err = 0;
	ssize_t callStat = 0;
	ssize_t ret = 0;
	uint8_t *buff = (uint8_t *)rawBuf;
	BOOL isDone = FALSE;

	if (fd < 0) {
		return ep_err(EBADF);
	}
	if (NULL == buff) {
		return ep_err(EINVAL);
	}
	if (0 == nbyte) {
		return 0;
	}

	/* loop read */
	do {
		callStat = recvfrom(fd, buff + ret, nbyte - ret, flags, from, fromlen);
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
				/* continue */
			} else if (EAGAIN == err) {
				DEBUG("Fd %d EAGAIN", fd);
				isDone = TRUE;
			} else {
				DEBUG("Fd %d error in recvfrom(): %s", strerror(err));
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


/* --------------------AMCFd_SockaddrToStr----------------------- */
int AMCFd_SockaddrToStr(const struct sockaddr *addr, char *buff, size_t buffSize)
{
	if (addr && buff && buffSize)
	{
		int ret = 0;

		switch (addr->sa_family)
		{
		case AF_UNIX:
		//case AF_LOCAL:
			{
				const struct sockaddr_un *addrUn = (const struct sockaddr_un *)addr;
				buff[buffSize - 1] = '\0';
				strncpy(buff, addrUn->sun_path, buffSize - 1);
			}
			break;
		case AF_INET:
			{
				const struct sockaddr_in *addrIn = (const struct sockaddr_in *)addr;
				inet_ntop(AF_INET, &(addrIn->sin_addr), buff, (socklen_t)buffSize);
			}
			break;
		case AF_INET6:
			{
				const struct sockaddr_in6 *addrIn6 = (const struct sockaddr_in6 *)addr;
				inet_ntop(AF_INET6, &(addrIn6->sin6_addr), buff, (socklen_t)buffSize);
			}
			break;
		default:
			ERROR("Unsupported type: %d", (int)(addr->sa_family));
			break;
		}

		return ret;
	}
	else {
		return ep_err(EINVAL);
	}
}



#endif


/* EOF */

