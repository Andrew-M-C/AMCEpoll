/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	epCommon.c
	
	Description: 	
	    This file definds simple common function for AMCEpoll project.
			
	History:
		2017-05-18: File created as "epCommon.c"

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
#include <errno.h>

#endif


/********/
#define __PUBLIC_FUNCTIONS
#ifdef __PUBLIC_FUNCTIONS

/* --------------------ep_err----------------------- */
int ep_err(int err)
{
	if (0 == err) {
		errno = 0;
		return 0;
	}

	if (err < 0) {
		err = -err;
	}

	if (err >= AMC_EP_ERR_UNKNOWN) {
		errno = EPERM;
		return -err;
	}
	else {
		errno = err;
		return -err;
	}
}

#endif


/* EOF */

