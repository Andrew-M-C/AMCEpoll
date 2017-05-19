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

#define _TIMEOUT_DEBUG_FLAG

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
#define _TM_DB(fmt, args...)		printf("[TME - %04d] "fmt"\n", (int)__LINE__, ##args)
#define _TM_MARK()					_TM_DB("--- MARK ---");
#else
#define _TM_DB(fmt, args...)
#define _TM_DB()
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
	if (RB_ERR_NO_FOUND == callStat)
	{
		/* no data exists at this time, just add it */
		DEBUG("Add new time %04%ld.%09%ld", (long)(time->tv_sec), (long)(time->tv_nsec));
		objValue.next = NULL;
		objValue.obj  = obj;

		callStat = utilRbTree_SetData(&(chain->time_obj_chain), timeKey, &objValue, NULL);
		if (callStat < 0) {
			ERROR("Failed to set obj data: %s", utilRbTree_StrError(callStat));
			return ep_err(EPERM);
		}

		timeValue.time = _rbkey_from_timespec(time);
		callStat = utilRbTree_SetData(&(chain->obj_time_chain), objKey, &timeValue, NULL);
		if (callStat < 0) {
			ERROR("Failed to set time data: %s", utilRbTree_StrError(callStat));
			utilRbTree_DelData(&(chain->time_obj_chain), timeKey, NULL);
			return ep_err(EPERM);
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
			int err = errno;
			ERROR("Failed to set time data: %s", utilRbTree_StrError(callStat));

			objValue.next = newObjValue->next;
			objValue.obj  = newObjValue->obj;
			free(newObjValue);
			newObjValue = NULL;

			return ep_err(err);
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

	callStat = utilRbTree_GetData(&(chain->obj_time_chain), objKey, &timeValue);
	/* add new object */
	if (RB_ERR_NO_FOUND == callStat) {
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
	if (RB_ERR_NO_FOUND == callStat) {
		WARN("Failed to locat timeout object %p", obj);
		return ep_err(ENOENT);
	}

	timeKey = timeValue.time;
	callStat = utilRbTree_GetData(&(chain->time_obj_chain), timeKey, &objValue);
	if (RB_ERR_NO_FOUND == callStat) {
		WARN("Failed to locat timeout object %p in obj chain", obj);
		return ep_err(ENOENT);
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

		return ep_err(0);
	}


	return -1;
}


#endif


/********/
#define __RB_TREE_CALLBACKS
#if 1




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
		return ep_err(EPERM);
	}

	callStat = utilRbTree_Init(&(chain->time_obj_chain), sizeof(struct ObjItem));
	if (callStat < 0) {
		ERROR("Failed to allocate obj RB-tree: %s", utilRbTree_StrError(callStat));
		callStat = ep_err(EPERM);
		goto ENDS;
	}

	callStat = utilRbTree_Init(&(chain->obj_time_chain), sizeof(struct TimeItem));
	if (callStat < 0) {
		ERROR("Failed to allocate time RB-tree: %s", utilRbTree_StrError(callStat));
		callStat = ep_err(EPERM);
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
		return 0;
	}

	utilRbTree_Clean(&(chain->time_obj_chain));
	utilRbTree_Clean(&(chain->obj_time_chain));
	chain->init_OK = FALSE;

	return 0;
}


/* --------------------utilTimeout_SetObject----------------------- */
int utilTimeout_SetObject(struct UtilTimeoutChain *chain, void *object, struct timespec inTime)
{
	if (NULL == chain) {
		return ep_err(EINVAL);
	}
	if (FALSE == chain->init_OK) {
		return ep_err(ENOENT);
	}
	else {
		RbKey_t timeKey = _rbkey_from_timespec(&inTime);

		if (0 == timeKey) {
			return utilTimeout_DelObject(chain, object);
		}
		else {
			DEBUG("Add timeout item: %04ld.%09ld --> %p", (long)(inTime.tv_sec), (long)(inTime.tv_nsec), object);
			return _set_object(chain, object, &inTime);
		}
	}
}


/* --------------------utilTimeout_DelObject----------------------- */
int utilTimeout_DelObject(struct UtilTimeoutChain *chain, void *object)
{
	if (NULL == chain) {
		return ep_err(EINVAL);
	}
	if (NULL == object) {
		return ep_err(EINVAL);
	}
	if (FALSE == chain->init_OK) {
		return ep_err(ENOENT);
	}
	else {
		return _del_object(chain, object);
	}
}


/* --------------------utilTimeout_GetSmallestTime----------------------- */
int utilTimeout_GetSmallestTime(struct UtilTimeoutChain *chain, struct timespec *timeOut, void **objOut)
{
	if (NULL == chain) {
		return ep_err(EINVAL);
	}
	if (FALSE == chain->init_OK) {
		return ep_err(ENOENT);
	}
	else {
		RbKey_t timeKey;
		struct ObjItem objValue;
		int callStat = 0;

		callStat = utilRbTree_FindMinimum(&(chain->time_obj_chain), &timeKey, &objValue);
		if (callStat < 0) {
			ERROR("Failed to get smallest time: %s", utilRbTree_StrError(callStat));
			return ep_err(ENOENT);
		}

		if (timeOut) {
			struct timespec time;
			time = _timespec_from_rbkey(timeKey);
			timeOut->tv_sec  = time.tv_sec;
			timeOut->tv_nsec = time.tv_nsec;
		}
		if (objOut) {
			*objOut = objValue.obj;
		}

		return ep_err(0);
	}
}


#endif


/* EOF */

