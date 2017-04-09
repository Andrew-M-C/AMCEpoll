/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	utilLog.h
	
	Description: 	
	    This file declares log informations for AMCEpoll tool.
			
	History:
		2017-04-08: File created as "utilLog.h"

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

#ifndef __UTIL_LOG_H__
#define __UTIL_LOG_H__

#include <stdio.h>
#include <unistd.h>

enum {
	LOG_EMERG = 0,
	LOG_ALERT,
	LOG_CRIT,
	LOG_ERR,
	LOG_WARNING,
	LOG_NOTICE,
	LOG_INFO,
	LOG_DEBUG,
};

#define CFG_MAX_LOG_LEN			512

/********/
/* functions */
void 
	utilLogSetLevel(int level);
ssize_t 
	utilLog(int level, const char *format, ...);


#endif
/* EOF */

