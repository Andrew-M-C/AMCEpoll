/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	utilTimeout.h
	
	Description: 	
	    This file provides common timeout service for all types of events. 
			
	History:
		2017-05-07: File created as "utilTimeout.h"

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

#ifndef __UTIL_TIMEOUT_H__
#define __UTIL_TIMEOUT_H__

/* headers */
#include "epCommon.h"

#include <time.h>

/* data definitions */
/* all data structures should be used internally */
struct UtilTimeoutChain {
	// TODO:
	
};


/* public functions */
int 
	utilTimeout_Init(struct UtilTimeoutChain *chain);
int 
	utilTimeout_Clean(struct UtilTimeoutChain *chain);
int 
	utilTimeout_AddObject(struct UtilTimeoutChain *chain, void *obj, uint64_t usec);
int 
	utilTimeout_DelObject(struct UtilTimeoutChain *chain, void *obj);
struct timespec  
	utilTimeout_GetMinimumTimeout(struct UtilTimeoutChain *chain, void **objOut);
void *
	utilTimeout_DrainTimeoutObject(struct UtilTimeoutChain *chain);			/* should invoke until NULL returns */
struct timespec  
	utilTimeout_GetSysupTime(void);


#endif
/* EOF */

