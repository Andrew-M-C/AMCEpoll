/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	utilLog.c
	
	Description: 	
	    This file definds log informations for AMCEpoll tool.
			
	History:
		2017-04-08: File created as "utilLog.c"

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

#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#ifndef CFG_LOG_LEVEL
#define CFG_LOG_LEVEL	LOG_NOTICE;
#endif

static const char *g_log_level_string[] = {
	"EMERG ",
	"ALERT ",
	"CRIT  ",
	"ERROR ",
	"WARN  ",
	"NOTICE",
	"INFO  ",
	"DEBUG ",
};

static int g_log_level = CFG_LOG_LEVEL;

#endif


/********/
#define __PUBLIC_FUNCTIONS
#ifdef __PUBLIC_FUNCTIONS

/* --------------------utilLogSetLevel----------------------- */
void utilLogSetLevel(int level)
{
	if (level >= LOG_LV_EMERG && level <= LOG_LV_DEBUG) {
		g_log_level = CFG_LOG_LEVEL;
	}
	return;
}


/* --------------------utilLog----------------------- */
ssize_t utilLog(int level, const char *format, ...)
{
	if (level < 0 || level > 7) {
		errno = EINVAL;
		return -EINVAL;
	}
	else if (level > g_log_level) {
		return 0;
	}
	else {
		char buff[CFG_MAX_LOG_LEN] = "";
		char usec[8] = "";
		ssize_t ret = 0;
		int errCopy = errno;
		va_list vaList;
		
		size_t prefixLen = 0;
		time_t secOfAll = 0;
		struct tm timeOfDay;
		struct timeval currDayTime;

		tzset();
		secOfAll = time(0);
		localtime_r(&secOfAll, &timeOfDay);
		gettimeofday(&currDayTime, NULL);

		sprintf(usec, "%d", (int)(currDayTime.tv_usec));
		usec[3] = '\0';

		prefixLen = sprintf(buff, "%04d-%02d-%02d,%02d:%02d:%02d.%s %s ", 
						timeOfDay.tm_year + 1900, timeOfDay.tm_mon + 1, timeOfDay.tm_mday,
						timeOfDay.tm_hour, timeOfDay.tm_min, timeOfDay.tm_sec, usec,
						g_log_level_string[level]);

		va_start(vaList, format);
		vsnprintf((char *)(buff + prefixLen), sizeof(buff) - prefixLen - 1, format, vaList);
		va_end(vaList);

		prefixLen = strlen(buff);
		buff[prefixLen + 0] = '\n';
		buff[prefixLen + 1] = '\0';

		ret = write(1, buff, prefixLen + 1);
		errno = errCopy;
		return ret;
	}
}



#endif


/* EOF */

