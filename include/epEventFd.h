/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	epEventFd.h
	
	Description: 	
	    This file declares file descriptor event interfaces for AMCEpoll.
			
	History:
		2017-04-26: File created as "epEventFd.h"

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

#ifndef __EP_EVENT_FD_H__
#define __EP_EVENT_FD_H__

#include "epCommon.h"
#include <errno.h>

struct AMCEpollEvent *
	epEventFd_Create(int fd, uint16_t events, int timeout, ev_callback callback, void *userData);
BOOL 
	epEventFd_IsFileEvent(events_t what);

#endif
/* EOF */

