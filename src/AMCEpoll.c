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

#include "utilLog.h"
#include "AMCEpoll.h"
#include "cAssocArray.h"

#endif


/********/
#define __DATA_DEFINITIONS
#ifdef __DATA_DEFINITIONS

struct epoll_event {
	int            fd;
	ev_callback    callback;
	void          *user_data;
	uint16_t       events;
};

struct AMCEpoll {
	int            epoll_fd;
	cAssocArray   *all_events;
};



#endif

/********/
#define __PUBLIC_FUNCTIONS
#ifdef __PUBLIC_FUNCTIONS




#endif


/* EOF */

