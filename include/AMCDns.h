/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	AMCDns.h
	
	Description: 	
	    This file declares main interfaces of DNS tool.
			
	History:
		2017-05-11: File created as "AMCDns.h"

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

#ifndef __AMC_DNS_H__
#define __AMC_DNS_H__

/********/
/* headers */
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

/********/
/* data types */

#define DNS_SERVER_PORT		53
#define DNS_DOMAIN_LEN_MAX	255
#define DNS_DOMAIN_PART_LEN_MAX	63

#define IPV4_STR_LEN_MAX		15
#define IPV6_STR_LEN_MAX		45	/* 8000:0000:0000:0000:0123:4567:192.168.168.168, this is the maximum length of an IPv6 string */

#ifndef NULL
#ifndef _DO_NOT_DEFINE_NULL
#define NULL	((void*)0)
#endif
#endif

#ifndef BOOL
#ifndef _DO_NOT_DEFINE_BOOL
#define BOOL	int
#define FALSE	0
#define TRUE	(!(FALSE))
#endif
#endif

/********/
/* functions */

void 
	AMCDns_Debug(void);
int 
	AMCDns_GetDefaultServer(struct sockaddr *dns, int index);
int 
	AMCDns_SendRequest(int fd, const char *domain, const struct sockaddr * to, socklen_t toLen);
ssize_t 
	AMCDns_RecvResponse(int fd, void *buff, size_t len);


#endif
/* EOF */

