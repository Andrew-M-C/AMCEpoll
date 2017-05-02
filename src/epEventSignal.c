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

#include "epEvent.h"
#include "epEventSignal.h"
#include "utilLog.h"
#include "AMCEpoll.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/epoll.h>

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

#define _SIG_EVENT_LOCK()		do{pthread_mutex_lock(&_signal_lock);}while(0)
#define _SIG_EVENT_UNLOCK()		do{pthread_mutex_unlock(&_signal_lock);}while(0)

static int _signal_epoll_code_from_amc_code(events_t amcCode);
static int epEventSignal_AddToBase(struct AMCEpoll *base, struct AMCEpollEvent *event);
static int epEventSignal_GenKey(struct AMCEpollEvent *event, char *keyOut, size_t nBuffLen);
static int epEventSignal_DetachFromBase(struct AMCEpoll *base, struct AMCEpollEvent *event);
static int epEventSignal_Destroy(struct AMCEpollEvent *event);
static int epEventSignal_InvokeCallback(struct AMCEpoll *base, struct AMCEpollEvent *event, int epollEvent);

#endif


/********/
#define __STATIC_VARIABLES
#ifdef __STATIC_VARIABLES

enum {
	PIPE_READ = 0,
	PIPE_WRITE = 1,
};

struct EpSigPipe {
	int fd[2];
};

static BOOL            _signal_is_init = FALSE;
static pthread_mutex_t _signal_lock = PTHREAD_MUTEX_INITIALIZER;
static int             _signal_pipes[SIGNAL_NUM_MAX + 1];



#endif


/********/
#define __EPOLL_FUNCTIONS
#ifdef __EPOLL_FUNCTIONS

/* --------------------_epoll_signal_add----------------------- */
static int _epoll_signal_add(struct AMCEpoll *base, struct AMCEpollEvent *amcEvent)
{
	int callStat = 0;
	struct epoll_event epollEvent;
	struct EpSigPipe *sigPipe = (struct EpSigPipe *)(amcEvent->inter_data);
	
	epollEvent.events = _signal_epoll_code_from_amc_code(amcEvent->events);
	epollEvent.data.ptr = amcEvent;

	callStat = epoll_ctl(base->epoll_fd, EPOLL_CTL_ADD, sigPipe->fd[PIPE_READ], &epollEvent);
	if (0 == callStat) {
		return 0;
	} else {
		int err = errno;
		ERROR("Failed in epoll_add(): %s", strerror(err));
		_RETURN_ERR(err);
	}
}


/* --------------------_epoll_signal_mod----------------------- */
static int _epoll_signal_mod(struct AMCEpoll *base, struct AMCEpollEvent *amcEvent)
{
	int callStat = 0;
	struct epoll_event epollEvent;
	struct EpSigPipe *sigPipe = (struct EpSigPipe *)(amcEvent->inter_data);
	
	epollEvent.events = _signal_epoll_code_from_amc_code(amcEvent->events);
	epollEvent.data.ptr = amcEvent;

	callStat = epoll_ctl(base->epoll_fd, EPOLL_CTL_MOD, sigPipe->fd[PIPE_READ], &epollEvent);
	if (0 == callStat) {
		return 0;
	} else {
		int err = errno;
		ERROR("Failed in epoll_mod(): %s", strerror(err));
		_RETURN_ERR(err);
	}
}


/* --------------------_epoll_signal_del----------------------- */
static int _epoll_signal_del(struct AMCEpoll *base, struct AMCEpollEvent *amcEvent)
{
	int callStat = 0;
	struct EpSigPipe *sigPipe = (struct EpSigPipe *)(amcEvent->inter_data);

	if (sigPipe->fd[PIPE_READ] > 0) {
		callStat = epoll_ctl(base->epoll_fd, EPOLL_CTL_DEL, sigPipe->fd[PIPE_READ], NULL);
		if (0 == callStat) {
			return 0;
		} else {
			int err = errno;
			ERROR("Failed in epoll_del(): %s", strerror(err));
			_RETURN_ERR(err);
		}
	}
	else {
		return 0;
	}
}


#endif


/********/
#define __SIGNAL_FUNCTIONS
#ifdef __SIGNAL_FUNCTIONS

/* --------------------_signal_handler----------------------- */
static void _signal_handler(int signum)
{
	if ((signum > 0) && (signum <= SIGNAL_NUM_MAX))
	{
		int fd = _signal_pipes[signum];
		if (fd > 0) {
			write(fd, &signum, sizeof(signum));
		}
	}
	return;
}


/* --------------------_snprintf_signal_key----------------------- */
static inline void _init_global_variables()
{
	if (FALSE == _signal_is_init) {
		_signal_is_init = TRUE;
		memset(_signal_pipes, -1, sizeof(_signal_pipes));
	}
}


/* --------------------_sys_signal_capture----------------------- */
static int _sys_signal_capture(int signum)
{
	struct sigaction sigAct;
	sigemptyset(&(sigAct.sa_mask));
	sigAct.sa_handler = _signal_handler;
	sigAct.sa_flags = 0;

	return sigaction(signum, &sigAct, NULL);
}


/* --------------------_sys_signal_giveup----------------------- */
static int _sys_signal_giveup(int signum)
{
	struct sigaction sigAct;
	sigemptyset(&(sigAct.sa_mask));
	sigAct.sa_handler = SIG_DFL;
	sigAct.sa_flags = 0;

	return sigaction(signum, &sigAct, NULL);
}


/* --------------------_signal_add_LOCKED----------------------- */
static int _signal_add_LOCKED(struct AMCEpollEvent *event)
{
	int callStat = 0;
	int sigNum = event->fd;
	struct EpSigPipe *sigPipe = (struct EpSigPipe *)(event->inter_data);

	callStat = pipe(sigPipe->fd);
	if (callStat < 0) {
		ERROR("Failed in pipe: %s", strerror(errno));
		goto FAILED;
	}
	else {
		AMCFd_MakeCloseOnExec(sigPipe->fd[PIPE_WRITE]);
		AMCFd_MakeCloseOnExec(sigPipe->fd[PIPE_READ]);
		AMCFd_MakeNonBlock(sigPipe->fd[PIPE_WRITE]);
		AMCFd_MakeNonBlock(sigPipe->fd[PIPE_READ]);
	}

	_SIG_EVENT_LOCK();			/* ---LOCK--- */
	{
		if (_signal_pipes[sigNum] <= 0) {
			/* Add a new signal handler */
			_signal_pipes[sigNum] = sigPipe->fd[PIPE_WRITE];
		}
		else if (_signal_pipes[sigNum] == sigPipe->fd[PIPE_WRITE]) {
 			/* OK, continue */
 		}
 		else {
			_SIG_EVENT_UNLOCK();	/* --UNLOCK-- */
			ERROR("Signal %d already been observed", sigNum);
			errno = EEXIST;
			goto FAILED;
		}
	}

	callStat = _sys_signal_capture(sigNum);
	if (callStat < 0)
	{
		_signal_pipes[sigNum] = -1;
		_SIG_EVENT_UNLOCK();		/* --UNLOCK-- */
		ERROR("Failed in sigaction(): %s", strerror(errno));
		goto FAILED;
	}

	_SIG_EVENT_UNLOCK();		/* --UNLOCK-- */

	return 0;
FAILED:
	{
		int err = errno;
		if (sigPipe->fd[PIPE_READ])
		{
			close(sigPipe->fd[PIPE_READ]);
			close(sigPipe->fd[PIPE_WRITE]);
			sigPipe->fd[PIPE_READ]  = -1;
			sigPipe->fd[PIPE_WRITE] = -1;
		}

		_RETURN_ERR(err);
	}
}


/* --------------------_signal_del_LOCKED----------------------- */
static int _signal_del_LOCKED(struct AMCEpollEvent *event)
{
	int callStat = 0;
	int sigNum = event->fd;
	struct EpSigPipe *sigPipe = (struct EpSigPipe *)(event->inter_data);

	_SIG_EVENT_LOCK();			/* ---LOCK--- */
	if (_signal_pipes[sigNum] == sigPipe->fd[PIPE_WRITE])
	{
		_signal_pipes[sigNum] = -1;
		callStat = _sys_signal_giveup(sigNum);		
	}
	_SIG_EVENT_UNLOCK();		/* --UNLOCK-- */

	if (sigPipe->fd[PIPE_READ] > 0)
	{
		close(sigPipe->fd[PIPE_READ]);
		close(sigPipe->fd[PIPE_WRITE]);
		sigPipe->fd[PIPE_READ]  = -1;
		sigPipe->fd[PIPE_WRITE] = -1;
	}

	return callStat;
}


/* --------------------_signal_re_add_LOCKED----------------------- */
static int _signal_re_add_LOCKED(struct AMCEpollEvent *event)
{
	int callStat = 0;
	int sigNum = event->fd;
	struct EpSigPipe *sigPipe = (struct EpSigPipe *)(event->inter_data);

	if (sigPipe->fd[PIPE_READ] <= 0) {
		DEBUG("Signal pipe for %d not initialized", sigNum);
		return _signal_add_LOCKED(event);
	}

	_SIG_EVENT_LOCK();			/* ---LOCK--- */
	{
		if (_signal_pipes[sigNum] <= 0) {
			/* Add a new signal handler */
			_signal_pipes[sigNum] = sigPipe->fd[PIPE_WRITE];
		}
		else if (_signal_pipes[sigNum] == sigPipe->fd[PIPE_WRITE]) {
 			/* OK, continue */
 		}
 		else {
			_SIG_EVENT_UNLOCK();	/* --UNLOCK-- */
			ERROR("Signal %d already been observed", sigNum);
			errno = EEXIST;
			goto FAILED;
		}
	}

	callStat = _sys_signal_capture(sigNum);
	if (callStat < 0)
	{
		_signal_pipes[sigNum] = -1;
		_SIG_EVENT_UNLOCK();		/* --UNLOCK-- */
		ERROR("Failed in sigaction(): %s", strerror(errno));
		goto FAILED;
	}

	_SIG_EVENT_UNLOCK();		/* --UNLOCK-- */

	return 0;
FAILED:
	{
		int err = errno;
		_RETURN_ERR(err);
	}
}


#endif


/********/
#define __NORMAL_FUNCTIONS
#ifdef __NORMAL_FUNCTIONS

/* --------------------_snprintf_signal_key----------------------- */
static void _snprintf_signal_key(struct AMCEpollEvent *event, char *str, size_t buffLen)
{
	str[buffLen - 1] = '\0';
	snprintf(str, buffLen - 1, "sig_%d", event->fd);
	return;
}


/* --------------------_check_sig_event_code----------------------- */
static BOOL _check_sig_event_code(events_t events)
{
	if (0 == events) {
		return FALSE;
	}
	if (FALSE == BITS_ALL_SET(events, EP_EVENT_SIGNAL)) {
		return FALSE;
	}
	return TRUE;
}


/* --------------------_signal_epoll_code_from_amc_code----------------------- */
static int _signal_epoll_code_from_amc_code(events_t amcCode)
{
	int ret = EPOLLIN | EPOLLPRI | EPOLLET;
	if (FALSE == BITS_ANY_SET(amcCode, EP_MODE_PERSIST)) {
		ret |= EPOLLONESHOT;
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


/* --------------------_amc_code_from_signal_epoll_code----------------------- */
static events_t _amc_code_from_signal_epoll_code(int epollCode)
{
	events_t ret = 0;
	if (BITS_ANY_SET(epollCode, EPOLLERR | EPOLLHUP)) {
		ret |= EP_EVENT_ERROR;
	}
	else {
		ret |= EP_EVENT_SIGNAL;
	}
	return ret;
}


/* --------------------_add_signal_event_LOCKED----------------------- */
static int _add_signal_event_LOCKED(struct AMCEpoll *base, struct AMCEpollEvent *event)
{
	int callStat = 0;
	callStat = epEventIntnl_AttachToBase(base, event);
	if (callStat < 0) {
		ERROR("Failed to add signal event %d: %s", event->fd, strerror(errno));
		goto FAILED;
	}

	callStat = _signal_add_LOCKED(event);
	if (callStat < 0) {
		ERROR("Failed to create capture process: %s", strerror(errno));
		goto FAILED;
	}

	callStat = _epoll_signal_add(base, event);
	if (callStat < 0)
	{
		int err = errno;
		_epoll_signal_del(base, event);
		_signal_del_LOCKED(event);
		errno = err;
		ERROR("Failed to add signal: %s", strerror(errno));
		goto FAILED;
	}

	return 0;
FAILED:
	return callStat;
}


/* --------------------_mod_signal_event_LOCKED----------------------- */
static int _mod_signal_event_LOCKED(struct AMCEpoll *base, struct AMCEpollEvent *event)
{
	int callStat = 0;
	DEBUG("Add new signal %d (%s)", event->fd, strsignal(event->fd));

	callStat = _signal_re_add_LOCKED(event);
	if (callStat < 0) {
		ERROR("Failed to re-capture signal %d: %s", event->fd, strerror(errno));
		goto FAILED;
	}

	callStat = _epoll_signal_mod(base, event);
	if (callStat < 0) {
		_signal_del_LOCKED(event);
		ERROR("Failed to mod signal: %s", strerror(errno));
		goto FAILED;
	}

	return 0;
FAILED:
	return callStat;
}


/* --------------------_del_signal_event_LOCKED----------------------- */
static int _del_signal_event_LOCKED(struct AMCEpoll *base, struct AMCEpollEvent *event)
{
	int callStat = 0;

	callStat = _signal_del_LOCKED(event);
	if (callStat < 0) {
		ERROR("Failed to delete signal event %d: %s", event->fd, strerror(errno));
		return callStat;
	}

	_epoll_signal_del(base, event);
	callStat = _signal_del_LOCKED(event);
	return callStat;
}


#endif


/********/
#define __PUBLIC_INTERFACES
#ifdef __PUBLIC_INTERFACES

/* --------------------epEventSignal_Create----------------------- */
struct AMCEpollEvent *epEventSignal_Create(int sig, events_t events, int timeout, ev_callback callback, void *userData)
{
	struct AMCEpollEvent *newEvent = NULL;
	struct EpSigPipe *sigPipe = NULL;

	if ((sig <= 0) || (sig > SIGNAL_NUM_MAX)) {
		ERROR("Invalid signal number: %d", sig);
		errno = EINVAL;
		return NULL;
	}
	if (NULL == callback) {
		ERROR("No callback specified for signal %d", sig);
		errno = EINVAL;
		return NULL;
	}
	if (FALSE == _check_sig_event_code(events)) {
		ERROR("Invalid events for signal %d: 0x%04lx", sig, (long)events);
		errno = EINVAL;
		return NULL;
	}

	_init_global_variables();

	newEvent = epEventIntnl_NewEmptyEvent();
	if (NULL == newEvent) {
		return NULL;
	}

	sigPipe = (struct EpSigPipe *)(newEvent->inter_data);
	sigPipe->fd[PIPE_READ]  = -1;
	sigPipe->fd[PIPE_WRITE] = -1;
	newEvent->fd = sig;
	newEvent->callback = callback;
	newEvent->user_data = userData;
	newEvent->epoll_events = _signal_epoll_code_from_amc_code(events);
	newEvent->events = events;
	newEvent->free_func = epEventSignal_Destroy;
	newEvent->genkey_func = epEventSignal_GenKey;
	newEvent->attach_func = epEventSignal_AddToBase;
	newEvent->detach_func = epEventSignal_DetachFromBase;
	newEvent->invoke_func = epEventSignal_InvokeCallback;
	_snprintf_signal_key(newEvent, newEvent->key, sizeof(newEvent->key));

	return newEvent;
}


/* --------------------epEventSignal_AddToBase----------------------- */
static int epEventSignal_AddToBase(struct AMCEpoll *base, struct AMCEpollEvent *event)
{
	if ((NULL == base) || (NULL == event)) {
		ERROR("Invalid parameter");
		_RETURN_ERR(EINVAL);
	}
	else {
		struct AMCEpollEvent *oldEvent = epEventIntnl_GetEvent(base, event->key);

		/* add a new event */
		if (NULL == oldEvent) {
			return _add_signal_event_LOCKED(base, event);
		}
		/* update event */
		else if (oldEvent == event) {
			return _mod_signal_event_LOCKED(base, event);
		}
		/* event duplicated */
		else {
			ERROR("Event for signal %d already existed", event->fd);
			_RETURN_ERR(EEXIST);
		}
	}
}


/* --------------------epEventSignal_IsSignalEvent----------------------- */
BOOL epEventSignal_IsSignalEvent(events_t what)
{
	return BITS_ANY_SET(what, EP_EVENT_SIGNAL);
}


/* --------------------epEventSignal_GenKey----------------------- */
static int epEventSignal_GenKey(struct AMCEpollEvent *event, char *keyOut, size_t nBuffLen)
{
	if (event && keyOut && nBuffLen) {
		/* OK */
	} else {
		ERROR("Invalid parameter");
		_RETURN_ERR(EINVAL);
	}

	_snprintf_signal_key(event, keyOut, nBuffLen);
	return 0;
}


/* --------------------epEventSignal_DetachFromBase----------------------- */
static int epEventSignal_DetachFromBase(struct AMCEpoll *base, struct AMCEpollEvent *event)
{
	if ((NULL == base) || (NULL == event)) {
		ERROR("Invalid parameter");
		_RETURN_ERR(EINVAL);
	}
	else {
		int callStat = 0;
		struct AMCEpollEvent *eventInBase = NULL;

		eventInBase = epEventIntnl_GetEvent(base, event->key);
		if (eventInBase != event) {
			ERROR("Event %p is not member of Base %p", event, base);
			_RETURN_ERR(ENOENT);
		}

		callStat = _del_signal_event_LOCKED(base, event);
		if (callStat < 0) {
			ERROR("Failed to del event in epoll_ctl(): %s", strerror(errno));
			return callStat;
		}

		return epEventIntnl_DetachFromBase(base, event);
	}
	return -1;
}


/* --------------------epEventSignal_Destroy----------------------- */
static int epEventSignal_Destroy(struct AMCEpollEvent *event)
{
	if (NULL == event) {
		_RETURN_ERR(EINVAL);
	} else {
		epEventIntnl_InvokeUserFreeCallback(event, event->fd);
		return epEventIntnl_FreeEmptyEvent(event);
	}
}


/* --------------------epEventSignal_InvokeCallback----------------------- */
static int epEventSignal_InvokeCallback(struct AMCEpoll *base, struct AMCEpollEvent *event, int epollEvent)
{
	events_t userWhat = 0;
	if (base && event && epollEvent)
	{
		int sigNum = 0;
		ssize_t callStat = 0;
		struct EpSigPipe *sigPipe = (struct EpSigPipe *)(event->inter_data);

		do {
			callStat = AMCFd_Read(sigPipe->fd[PIPE_READ], &sigNum, sizeof(sigNum));
		} while(callStat > 0);

		DEBUG("Get signal: %s (%d)", strsignal(sigNum), sigNum);
		
		userWhat = _amc_code_from_signal_epoll_code(epollEvent);
		if (BITS_HAVE_INTRSET(userWhat, event->events)) {
			epEventIntnl_InvokeUserCallback(event, event->fd, userWhat);
		}
		return 0;
	}
	else {
		RETURN_ERR(EINVAL);
	}
}


#endif
/* EOF */

