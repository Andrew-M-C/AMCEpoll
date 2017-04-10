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
	LOG_LV_EMERG = 0,
	LOG_LV_ALERT,
	LOG_LV_CRIT,
	LOG_LV_ERR,
	LOG_LV_WARNING,
	LOG_LV_NOTICE,
	LOG_LV_INFO,
	LOG_LV_DEBUG,
};

#define CFG_MAX_LOG_LEN			512

#define DEBUG(fmt, args...)		utilLog(LOG_LV_DEBUG, __FILE__" %d:"fmt, __LINE__, ##args)
#define INFO(fmt, args...)		utilLog(LOG_LV_INFO, __FILE__" %d:"fmt, __LINE__, ##args)
#define NOTICE(fmt, args...)		utilLog(LOG_LV_NOTICE, __FILE__" %d:"fmt, __LINE__, ##args)
#define WARN(fmt, args...)		utilLog(LOG_LV_WARNING, __FILE__" %d:"fmt, __LINE__, ##args)
#define ERROR(fmt, args...)		utilLog(LOG_LV_ERR, __FILE__" %d:"fmt, __LINE__, ##args)
#define CRIT(fmt, args...)		utilLog(LOG_LV_CRIT, __FILE__" %d:"fmt, __LINE__, ##args)
#define ALERT(fmt, args...)		utilLog(LOG_LV_ALERT, __FILE__" %d:"fmt, __LINE__, ##args)
#define EMERG(fmt, args...)		utilLog(LOG_LV_EMERG, __FILE__" %d:"fmt, __LINE__, ##args)

/********/
/* functions */
void 
	utilLogSetLevel(int level);
ssize_t 
	utilLog(int level, const char *format, ...);


#endif
/* EOF */

