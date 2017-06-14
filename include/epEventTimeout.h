/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	epEventTimeout.h
	
	Description: 	
	    This file declares pure timeout event interfaces for AMCEpoll.
			
	History:
		2017-06-14: File created as "epEventTimeout.h"

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

#ifndef __EP_EVENT_TIMEOUT_H__
#define __EP_EVENT_TIMEOUT_H__

#include "epCommon.h"
#include <errno.h>

#define EVENT_TIMEOUT_DESCRIPTION		"time event"

struct AMCEpollEvent *
	epEventTimeout_Create(int fd, events_t events, long timeout, ev_callback callback, void *userData);
BOOL 
	epEventTimeout_IsTimeoutEvent(events_t what);

#endif
/* EOF */

