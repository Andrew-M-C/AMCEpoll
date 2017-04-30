/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	epEvent.h
	
	Description: 	
	    This file declares an abstract class named "epEvent". AMCEpoll imstances
	will only work with this class. Most of the class functions will NOT imple-
	mentated in this class.
			
	History:
		2017-04-30: File created as "epEvent.h"

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

#ifndef __EP_EVENT_H__
#define __EP_EVENT_H__

/* headers */
#include <stdint.h>
#include <unistd.h>
#include <sys/epoll.h>

#include "epCommon.h"
#include "cAssocArray.h"
#include "AMCEpoll.h"

/* Internal Class Functions */
struct AMCEpollEvent *
	epEventIntnl_NewEmptyEvent(void);
int 
	epEventIntnl_FreeEmptyEvent(AMCEpollEvent *event);
int 
	epEventIntnl_AttachToBase(AMCEpoll *base, AMCEpollEvent *event);
int 
	epEventIntnl_DetachFromBase(AMCEpoll *base, AMCEpollEvent *event);


/* Public Class Functions */
struct AMCEpollEvent *
	epEvent_New(events_t what);
int 
	epEvent_Free(AMCEpollEvent *event);
int 
	epEvent_GetKey(AMCEpollEvent *event);
int 
	epEvent_AddToBase(AMCEpoll *base, AMCEpollEvent *event);
int 
	epEvent_DelFromBase(AMCEpoll *base, AMCEpollEvent *event);
int 
	epEvent_InvokeCallback(AMCEpoll *base, AMCEpollEvent *event, int epollEvents);

#endif
/* EOF */

