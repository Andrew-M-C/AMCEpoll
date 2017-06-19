/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	utilTimeout.c
	
	Description: 	
	    This file definds simple common function for AMCEpoll project.
			
	History:
		2017-05-18: File created as "utilTimeout.c"

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
#if 1

#include "utilTimeout.h"
#include "epCommon.h"
#include "utilLog.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

//#define _TIMEOUT_DEBUG_FLAG

#endif


/********/
#define __DATA_DEFINITIONS
#if 1

struct ObjItem {
	struct ObjItem *next;
	void *obj;
};


struct TimeItem {
	RbKey_t time;
};


#ifdef _TIMEOUT_DEBUG_FLAG
#define _TM_DB(fmt, args...)		DEBUG(fmt, ##args)
#define _TM_MARK()					_TM_DB("--- MARK ---");
#else
#define _TM_DB(fmt, args...)
#define _TM_MARK()
#endif


#endif



/********/
#define __GENERAL_INTERNAL_FUNCTIONS
#if 1

/* --------------------_rbkey_from_timespec----------------------- */
static inline RbKey_t _rbkey_from_timespec(struct timespec *timespec)
{
	RbKey_t ret = 0;
	ret |= (((RbKey_t)(timespec->tv_sec)) & 0x00000000FFFFFFFF) << 32;
	ret |= ((timespec->tv_nsec) & 0x00000000FFFFFFFF);
	return ret;
}


/* --------------------_timespec_from_rbkey----------------------- */
static inline struct timespec _timespec_from_rbkey(RbKey_t key)
{
	struct timespec ret;
	ret.tv_sec  = (time_t)((key & 0xFFFFFFFF00000000) >> 32);
	ret.tv_nsec = (long)(key & 0x00000000FFFFFFFF);
	return ret;
}


/* --------------------_rbKey_from_objptr----------------------- */
/* Please refer to: 
 * 		http://stackoverflow.com/questions/11796909/how-to-resolve-cast-to-pointer-from-integer-of-different-size-warning-in-c-co
 * 		http://www.cnblogs.com/Anker/p/3438480.html
 */
static inline RbKey_t _rbKey_from_objptr(void *obj)
{
	return (RbKey_t)(uintptr_t)obj;
}


/* --------------------_objptr_from_rbkey----------------------- */
static inline void *_objptr_from_rbkey(RbKey_t key)
{
	return (void *)(uintptr_t)key;
}


#endif


/********/
#define __RB_TREE_OPERATIONS
#if 1


/* --------------------_add_new_object----------------------- */
static int _add_new_object(struct UtilTimeoutChain *chain, void *obj, struct timespec *time)
{
	RbKey_t timeKey = _rbkey_from_timespec(time);
	RbKey_t objKey  = _rbKey_from_objptr(obj);
	int callStat;
	struct ObjItem objValue;
	struct TimeItem timeValue;

	callStat = utilRbTree_GetData(&(chain->time_obj_chain), timeKey, &objValue);
	if (-RB_ERR_NO_FOUND == callStat)
	{
		/* no data exists at this time, just add it */
		_TM_DB("Add new time %04ld.%09ld", (long)(time->tv_sec), (long)(time->tv_nsec));
		objValue.next = NULL;
		objValue.obj  = obj;

		callStat = utilRbTree_SetData(&(chain->time_obj_chain), timeKey, &objValue, NULL);
		if (callStat < 0) {
			ERROR("Failed to set obj data: %s", utilRbTree_StrError(callStat));
			return ep_err(callStat);
		}

		timeValue.time = _rbkey_from_timespec(time);
		callStat = utilRbTree_SetData(&(chain->obj_time_chain), objKey, &timeValue, NULL);
		if (callStat < 0) {
			ERROR("Failed to set time data: %s", utilRbTree_StrError(callStat));
			utilRbTree_DelData(&(chain->time_obj_chain), timeKey, NULL);
			return ep_err(callStat);
		}

		return ep_err(0);
	}
	else
	{
		/* specific time already exists, append it to list */
		struct ObjItem *newObjValue = malloc(sizeof(*newObjValue));
		if (NULL == newObjValue) {
			return ep_err(errno);
		}

		newObjValue->next = objValue.next;
		newObjValue->obj  = objValue.obj;

		objValue.next = newObjValue;
		objValue.obj  = obj;

		timeValue.time = _rbkey_from_timespec(time);
		callStat = utilRbTree_SetData(&(chain->obj_time_chain), timeKey, &timeValue, NULL);
		if (callStat < 0) {
			ERROR("Failed to set time data: %s", utilRbTree_StrError(callStat));

			objValue.next = newObjValue->next;
			objValue.obj  = newObjValue->obj;
			free(newObjValue);
			newObjValue = NULL;

			return ep_err(callStat);
		}

		return ep_err(0);
	}
}


/* --------------------_set_object----------------------- */
static int _set_object(struct UtilTimeoutChain *chain, void *obj, struct timespec *time)
{
	int callStat = 0;
	struct ObjItem objValue;
	struct TimeItem timeValue;
	RbKey_t timeKey = _rbkey_from_timespec(time);
	RbKey_t objKey  = _rbKey_from_objptr(obj);

	_TM_DB("Add time %04ld.%09ld, object %s", (long)(time->tv_sec), (long)(time->tv_nsec), ((struct AMCEpollEvent *)obj)->description);

	callStat = utilRbTree_GetData(&(chain->obj_time_chain), objKey, &timeValue);
	/* add new object */
	if (-RB_ERR_NO_FOUND == callStat) {
		return _add_new_object(chain, obj, time);
	}

	/* update old object */
	// 1. Update time_obj_chain
	callStat = utilRbTree_GetData(&(chain->time_obj_chain), timeValue.time, &objValue);
	if (0 == callStat)
	{
		/* search for object */
		if (obj == objValue.obj)
		{
			if (objValue.next)
			{
				/* This is the start point, delete it */
				struct ObjItem *next = objValue.next;
				objValue.next = next->next;
				objValue.obj  = next->obj;

				free(next);
				utilRbTree_SetData(&(chain->time_obj_chain), timeKey, &objValue, NULL);
			}
			else
			{
				/* Search for list chain */
				struct ObjItem *prev = &objValue;
				struct ObjItem *next = objValue.next;
				while(next && (next->obj != obj))
				{
					prev = next;
					next = next->next;
				}

				if (next) {
					prev->next = next->next;
					free(next);
					next = NULL;

					if (prev == &objValue) {
						utilRbTree_SetData(&(chain->time_obj_chain), timeKey, &objValue, NULL);
					}
				}
			}
		}
	}

	// 2. update obj_time_chain
	timeValue.time = _rbkey_from_timespec(time);
	utilRbTree_SetData(&(chain->obj_time_chain), objKey, &timeValue, NULL);

	/* return */
	return 0;
}


/* --------------------_del_object----------------------- */
static int _del_object(struct UtilTimeoutChain *chain, void *obj)
{
	int callStat = 0;

	RbKey_t objKey = _rbKey_from_objptr(obj);
	struct TimeItem timeValue;

	RbKey_t timeKey;
	struct ObjItem objValue;

	callStat = utilRbTree_DelData(&(chain->obj_time_chain), objKey, &timeValue);
	if (-RB_ERR_NO_FOUND == callStat) {
//		WARN("Failed to locate timeout object %p", obj);
		return ep_err(AMC_EP_ERR_OBJ_NOT_FOUND);
	}

	timeKey = timeValue.time;
	callStat = utilRbTree_GetData(&(chain->time_obj_chain), timeKey, &objValue);
	if (-RB_ERR_NO_FOUND == callStat) {
//		WARN("Failed to locate timeout object %p in obj chain", obj);
		return ep_err(AMC_EP_ERR_OBJ_NOT_FOUND);
	}

	if (objValue.obj == obj)
	{
		if (NULL == objValue.next)
		{
			utilRbTree_DelData(&(chain->time_obj_chain), timeKey, NULL);
			return ep_err(0);
		}
		else
		{
			struct ObjItem *next = objValue.next;

			objValue.next = next->next;
			objValue.obj  = next->obj;

			free(next);
			next = NULL;

			utilRbTree_SetData(&(chain->time_obj_chain), timeKey, &objValue, NULL);
			return ep_err(0);
		}
	}
	else
	{
		struct ObjItem *prev = objValue.next;
		struct ObjItem *next = prev->next;

		do {
			if (next->obj == obj)
			{
				prev->next = next->next;
				free(next);
				break;
			}
		} while(next);

		_TM_DB("Del time %04ld.%09ld, object %s", 
				(long)(_timespec_from_rbkey(timeValue.time).tv_sec), 
				(long)(_timespec_from_rbkey(timeValue.time).tv_nsec), 
				((struct AMCEpollEvent *)obj)->description);

		return ep_err(0);
	}

	return -1;
}


#endif


/********/
#define __GENERAL_INTERNAL_FUNCTIONS
#if 1


/* --------------------_timespec_comp----------------------- */
int _timespec_comp(const struct timespec *left, const struct timespec *right)
{
	if (left->tv_sec < right->tv_sec) {
		return -1;
	}
	else if (left->tv_sec == right->tv_sec)
	{
		if (left->tv_nsec < right->tv_nsec) {
			return -1;
		}
		else if (left->tv_nsec == right->tv_nsec) {
			return 0;
		}
		else {
			return 1;
		}
	}
	else {
		return 1;
	}
}


/* --------------------_timespec_sub----------------------- */
static void _timespec_sub(struct timespec *result, struct timespec *a, struct timespec *b)
{
	struct timespec resCopy = {0, 0};
	if (a->tv_nsec < b->tv_nsec) {
		resCopy.tv_nsec = a->tv_nsec + 1000000000 - b->tv_nsec;
		resCopy.tv_sec = a->tv_sec - 1 - b->tv_sec;
	}
	else {
		resCopy.tv_nsec = a->tv_nsec + 1000000000 - b->tv_nsec;
		resCopy.tv_sec = a->tv_sec - 1 - b->tv_sec;
	}

	result->tv_sec  = resCopy.tv_sec;
	result->tv_nsec = resCopy.tv_nsec;
	
	return;
}


/* --------------------_timespec_from_milisecs----------------------- */
static struct timespec _timespec_from_milisecs(long milisecs)
{
	struct timespec ret = {0, 0};
	if (milisecs <= 0) {
		return ret;
	}
	else {
		time_t sec = milisecs / 1000;
		long nsec = (milisecs - (sec * 1000)) * 1000000;

		ret.tv_sec  = sec;
		ret.tv_nsec = nsec;

		return ret;
	}
}


/* --------------------_milisecs_from_timespec----------------------- */
static long _milisecs_from_timespec(struct timespec *spec)
{
	return (spec->tv_sec * 1000) + (spec->tv_nsec / 1000000);
}


#endif


/********/
#define __PUBLIC_FUNCTIONS
#if 1

/* --------------------utilTimeout_Init----------------------- */
int utilTimeout_Init(struct UtilTimeoutChain *chain)
{
	int callStat = 0;

	if (NULL == chain) {
		return ep_err(EINVAL);
	}
	if (chain->init_OK) {
		return ep_err(AMC_EP_ERR_BASE_TIMEOUT_NOT_INIT);
	}

	callStat = utilRbTree_Init(&(chain->time_obj_chain), sizeof(struct ObjItem));
	if (callStat < 0) {
		ERROR("Failed to allocate obj RB-tree: %s", utilRbTree_StrError(callStat));
		goto ENDS;
	}

	callStat = utilRbTree_Init(&(chain->obj_time_chain), sizeof(struct TimeItem));
	if (callStat < 0) {
		ERROR("Failed to allocate time RB-tree: %s", utilRbTree_StrError(callStat));
		goto ENDS;
	}

	chain->init_OK = TRUE;
ENDS:
	if (callStat < 0) {
		int retCopy = callStat;
		utilRbTree_Clean(&(chain->time_obj_chain));
		utilRbTree_Clean(&(chain->obj_time_chain));
		callStat = ep_err(retCopy);
	}
	return callStat;
}


/* --------------------utilTimeout_Clean----------------------- */
int utilTimeout_Clean(struct UtilTimeoutChain *chain)
{
	if (NULL == chain) {
		return ep_err(EINVAL);
	}
	if (FALSE == chain->init_OK) {
		return ep_err(0);
	}

	utilRbTree_Clean(&(chain->time_obj_chain));
	utilRbTree_Clean(&(chain->obj_time_chain));
	chain->init_OK = FALSE;

	return ep_err(0);
}


/* --------------------utilTimeout_SetObject----------------------- */
int utilTimeout_SetObject(struct UtilTimeoutChain *chain, struct AMCEpollEvent *event, struct timespec inTime)
{
	if (NULL == chain) {
		return ep_err(EINVAL);
	}
	if (FALSE == chain->init_OK) {
		return ep_err(AMC_EP_ERR_BASE_TIMEOUT_NOT_INIT);
	}
	else {
		RbKey_t timeKey = _rbkey_from_timespec(&inTime);

		if (0 == timeKey) {
			return utilTimeout_DelObject(chain, event);
		}
		else {
			int ret = 0;
			struct timespec target = utilTimeout_GetSysupTime();
			uint64_t nsec = inTime.tv_nsec + target.tv_nsec;
			if (nsec >= 1000000000) {
				target.tv_sec += inTime.tv_sec + 1;
				target.tv_nsec = (long)(nsec - 1000000000);
			}
			else {
				target.tv_sec += inTime.tv_sec;
				target.tv_nsec = (long)(nsec);
			}
			
			_TM_DB("Add timeout item %s: %04ld.%09ld <-- %04ld.%09ld", 
						event->description, 
						(long)(target.tv_sec), (long)(target.tv_nsec),
						(long)(inTime.tv_sec), (long)(inTime.tv_nsec));
			ret = _set_object(chain, event, &target);
#ifdef _TIMEOUT_DEBUG_FLAG
			_TM_DB("After del:");
			utilTimeout_Debug(chain);
#endif
			return ret;
		}
	}
}


/* --------------------utilTimeout_ObjectExists----------------------- */
BOOL utilTimeout_ObjectExists(struct UtilTimeoutChain *chain, struct AMCEpollEvent *event)
{
	if (NULL == chain) {
		return FALSE;
	}
	if (NULL == event) {
		return FALSE;
	}
	else {
		int callStat = 0;
		RbKey_t objKey = _rbKey_from_objptr(event);

		callStat = utilRbTree_GetData(&(chain->obj_time_chain), objKey, NULL);
		if (0 == callStat) {
			return TRUE;
		}
		else {
			return FALSE;
		}
	}
}


/* --------------------utilTimeout_DelObject----------------------- */
int utilTimeout_DelObject(struct UtilTimeoutChain *chain, struct AMCEpollEvent *event)
{
	if (NULL == chain) {
		return ep_err(EINVAL);
	}
	if (NULL == event) {
		return ep_err(EINVAL);
	}
	if (FALSE == chain->init_OK) {
		return ep_err(AMC_EP_ERR_BASE_TIMEOUT_NOT_INIT);
	}
	else {
		int ret = _del_object(chain, event);
#ifdef _TIMEOUT_DEBUG_FLAG
		_TM_DB("After del:");
		utilTimeout_Debug(chain);
#endif
		return ret;
	}
}


/* --------------------utilTimeout_GetSmallestTime----------------------- */
int utilTimeout_GetSmallestTime(struct UtilTimeoutChain *chain, struct timespec *timeOut, struct AMCEpollEvent **eventOut)
{
	if (NULL == chain) {
		return ep_err(EINVAL);
	}
	if (FALSE == chain->init_OK) {
		return ep_err(AMC_EP_ERR_BASE_TIMEOUT_NOT_INIT);
	}
	else {
		RbKey_t timeKey;
		struct ObjItem objValue;
		int callStat = 0;

		callStat = utilRbTree_FindMinimum(&(chain->time_obj_chain), &timeKey, &objValue);
		if (callStat < 0) {
			ERROR("Failed to get smallest time: %s", utilRbTree_StrError(callStat));
			return ep_err(callStat);
		}

		if (timeOut) {
			struct timespec time;
			time = _timespec_from_rbkey(timeKey);
			timeOut->tv_sec  = time.tv_sec;
			timeOut->tv_nsec = time.tv_nsec;
		}
		if (eventOut) {
			*eventOut = (struct AMCEpollEvent *)(objValue.obj);
		}

		return ep_err(0);
	}
}


/* --------------------utilTimeout_GetSysupTime----------------------- */
struct timespec utilTimeout_GetSysupTime()
{
	struct timespec currtime;
	clock_gettime(CLOCK_MONOTONIC, &currtime);
	return currtime;
}


/* --------------------utilTimeout_TimespecFromMilisecs----------------------- */
struct timespec utilTimeout_TimespecFromMilisecs(long milisecs)
{
	return _timespec_from_milisecs(milisecs);
}


/* --------------------utilTimeout_TimespecFromMilisecs----------------------- */
signed long utilTimeout_MinimumSleepMilisecs(struct UtilTimeoutChain *chain)
{
	int callStat = 0;
	struct AMCEpollEvent *event = NULL;
	struct timespec nextTime = {0};

	if (NULL == chain) {
		return ep_err(EINVAL);
	}

	callStat = utilTimeout_GetSmallestTime(chain, &nextTime, &event);
	if (0 == callStat) {
		int comp = 0;
		struct timespec diffTime = {0};
		struct timespec currTime = utilTimeout_GetSysupTime();

		comp = _timespec_comp(&nextTime, &currTime);
		if (comp < 0) {
			_TM_DB("Curr: %04ld.%09ld  |  Min: %04ld.%09ld", (long)currTime.tv_sec, (long)currTime.tv_nsec, (long)nextTime.tv_sec, (long)nextTime.tv_nsec);
			return 0;
		}
		else {
			_timespec_sub(&diffTime, &nextTime, &currTime);
			return _milisecs_from_timespec(&diffTime);
		}
	}
	else {
		/* no timeout exists */
		return -1;
	}
}


/* --------------------utilTimeout_CompareTime----------------------- */
int utilTimeout_CompareTime(const struct timespec *left, const struct timespec *right)
{
	if (left && right) {
		return _timespec_comp(left, right);
	} else {
		return 0;
	}
}


#endif


/*******/
#define __RB_TREE_TEST_FUNCTIONS
#ifndef _TIMEOUT_DEBUG_FLAG

void utilTimeout_Debug(struct UtilTimeoutChain *chain)
{
	return;
}

#else
#include <string.h>
static void _check_time_obj_chain(const struct RbCheckPara *para, void *arg)
{
	struct ObjItem eventObj = {0};
	struct ObjItem *eventEach = &eventObj;
	struct timespec time = {0};
	size_t *count = (size_t *)arg;

	memcpy(&eventObj, para->data, sizeof(eventObj));
	time = _timespec_from_rbkey(para->key);

	while(eventEach)
	{
		struct AMCEpollEvent *event = (struct AMCEpollEvent *)(eventEach->obj);
		_TM_DB("\t[%02ld] %04ld.%09ld: %s", (long)(*count), (long)time.tv_sec, (long)time.tv_nsec, event->description);
		*count += 1;
		eventEach = eventEach->next;
	}

	return;
}


static void _check_obj_time_chain(const struct RbCheckPara *para, void *arg)
{
	struct TimeItem timeObj = {0};
	struct timespec time = {0};
	struct AMCEpollEvent *event = NULL;
	size_t *count = (size_t *)arg;

	memcpy(&timeObj, para->data, sizeof(timeObj));
	time = _timespec_from_rbkey(timeObj.time);
	event = _objptr_from_rbkey(para->key);

	_TM_DB("\t[%02ld] %04ld.%09ld: %s", (long)(*count), (long)time.tv_sec, (long)time.tv_nsec, event->description);

	*count += 1;
	return;
}


void utilTimeout_Debug(struct UtilTimeoutChain *chain)
{
	size_t count = 0;

	count = 0;
	_TM_DB("time_obj_chain has %ld memers", (long)utilRbTree_GetNodeCount(&(chain->time_obj_chain)));
	utilRbTree_CheckData(&(chain->time_obj_chain), RbCheck_IncAll, 0, _check_time_obj_chain, &count);
	utilRbTree_Dump(&(chain->time_obj_chain), 2);

	count = 0;
	_TM_DB("obj_time_chain has %ld memers", (long)utilRbTree_GetNodeCount(&(chain->obj_time_chain)));
	utilRbTree_CheckData(&(chain->obj_time_chain), RbCheck_IncAll, 0, _check_obj_time_chain, &count);
	utilRbTree_Dump(&(chain->obj_time_chain), 2);

	return;
}


#if 0
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define DO_AND_TEST(x)	do{	\
	int callStat = (x);	\
	if (callStat < 0) {	\
		ERROR("Failed in Line %d: %s", (int)__LINE__, utilRbTree_StrError(callStat));	\
		return;	\
	}	\
}while(0)

static uint8_t _rand_u8()
{
	uint8_t ret = 0;
	ssize_t callStat = 0;
	char errorMsg[128] = "";
	int fd = open("/dev/urandom", O_RDONLY);

	if (fd < 0) {
		snprintf(errorMsg, sizeof(errorMsg), "Failed to open ramdom device: %s\n", strerror(errno));
		goto ENDS;
	}

	callStat = read(fd, &(ret), sizeof(ret));
	if (callStat < 0) {
		snprintf(errorMsg, sizeof(errorMsg), "Failed to read from ramdom device: %s\n", strerror(errno));
		goto ENDS;
	}

ENDS:
	if (fd > 0) {
		close(fd);
		fd = -1;
	}

	if (errorMsg[0]) {
		write(2, errorMsg, strlen(errorMsg));
	}
	
	return ret;
}

static int _rand()
{
	uint32_t ret = 0;

	ret += _rand_u8() << 0;
	ret += _rand_u8() << 8;
	ret += _rand_u8() << 16;
	ret += _rand_u8() << 24;

	return (int)(ret & 0x7FFFFFFF);
}

static int _rand_int(int minInt, int maxInt)
{
	int result;

	if (minInt == maxInt)
	{
		result = minInt;
	}
	else
	{
		if (minInt > maxInt)
		{
			/* swap */
			result = minInt;
			minInt = maxInt;
			maxInt = result;
		}

		/* randomize */
		result = minInt + (int)((float)(maxInt - minInt + 1) * (float)(_rand() / (RAND_MAX + 1.0)));
	}
	return result;
}




void utilTimeout_Debug()
{
	struct UtilRbTree tree = {0};

	utilRbTree_Clean(&tree);

	DO_AND_TEST(utilRbTree_Init(&tree, 4));
	DO_AND_TEST(utilRbTree_SetData(&tree, 1, "0001", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 2, "0002", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 3, "0003", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 4, "0004", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 5, "0005", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 6, "0006", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 7, "0007", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 8, "0008", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 9, "0009", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 10, "0010", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 11, "0011", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 12, "0012", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 13, "0013", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 14, "0014", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 15, "0015", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 16, "0016", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 17, "0017", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 18, "0018", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 19, "0019", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 20, "0020", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 21, "0021", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 22, "0022", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 23, "0023", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 24, "0024", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 25, "0025", NULL));
	DO_AND_TEST(utilRbTree_SetData(&tree, 26, "0026", NULL));
	utilRbTree_Dump(&tree, 1);

	/* test remove */
	{
		long tmp;
		for (tmp = 0; tmp < 26; tmp ++)
		{
			utilRbTree_DelData(&tree, (RbKey_t)_rand_int(1, 26), NULL);
		}
	}

	utilRbTree_Dump(&tree, 1);
	utilRbTree_Clean(&tree);

	return;
}
#endif


#endif

/* EOF */

