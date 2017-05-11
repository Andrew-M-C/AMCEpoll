/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	AMCDns.c
	
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

#include "AMCDns.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define _DNS_DEBUG_FLAG

#endif


/********/
#define __DATA_DEFINITIONS
#ifdef __DATA_DEFINITIONS


#define UDP_PACKAGE_LEN_MAX		1480


#ifndef RETURN_ERR
#define RETURN_ERR(err)	\
	do{\
		if (err > 0) {\
			errno = err;\
			return (0 - err);\
		} else if (err < 0) {\
			errno = 0 - err;\
			return err;\
		} else {\
			return -1;\
		}\
	}while(0)
#endif


#ifdef _DNS_DEBUG_FLAG
#define _DNS_DB(fmt, args...)		printf("[DNS - %04ld] "fmt"\n", (long)__LINE__, ##args)
#define _DNS_MARK()					_DNS_DB("--- MARK ---");
#else
#define _DNS_DB(fmt, args...)
#define _DNS_MARK()
#endif


#define _BITS_ANY_SET(val, bits)		(0 != ((val) & (bits)))
#define _BITS_ALL_SET(val, bits)		((bits) == ((val) & (bits)))
#define _BITS_SET(val, bits)			((val) |= (bits))


typedef struct {
	uint16_t      transaction_ID;
	uint16_t      flags;
	uint16_t      questions;
	uint16_t      andwer_RRs;
	uint16_t      authority_RRs;
	uint16_t      additional_RRs;
} DNSHeader_st;


enum {
	DNS_OPCODE_INQUIRY = 0,
	// Others not supported in this file
};

enum {
	DNS_REPLYCODE_OK                     = 0,
	DNS_REPLYCODE_FMT_ERR                = 1,
	DNS_REPLYCODE_NAME_NOT_EXIST         = 2,
	DNS_REPLYCODE_REJECTED               = 5,
	DNS_REPLYCODE_NAME_SHOULD_NOT_APPEAR = 6,
	DNS_REPLYCODE_RR_NOT_EXIST           = 7,
	DNS_REPLYCODE_RR_INQUIRY_FAIL        = 8,
	DNS_REPLYCODE_SRV_AUTH_FAIL          = 9,
	DNS_REPLYCODE_NAME_OUT_OF_AREA       = 10,
};


enum {
	DNS_RR_TYPE_A      = 1,
	DNS_RR_TYPE_AAAA   = 28,
	DNS_RR_TYPE_NS     = 2,
	DNS_RR_TYPE_CNAME  = 5,
	// Others not supported in this file
};


enum {
	DNS_CLASS_INTERNET = 1,
};


#endif


/********/
#define __DEBUG_FUNCTIONS
#ifdef __DEBUG_FUNCTIONS

/* --------------------_dump_data----------------------- */
#ifdef _DNS_DEBUG_FLAG
static char _byte_to_char(uint8_t byte)
{
	if ((byte >= '!') && (byte <= 0x7F))
	{
		return (char)byte;
	}
	else if ('\n' == byte)
	{
		return '.';
	}
	else if ('\r' == byte)
	{
		return '.';
	}
	else if (' ' == byte)
	{
		return ' ';
	}
	else
	{
		return '.';
	}
}

void _dump_data(const void *pData, const size_t size)
{
	size_t column, tmp;
	char lineString[64] = "";
	char linechar[24] = "";
	size_t lineLen = 0;
	uint8_t byte;
	const uint8_t *data = pData;

	printf ("---------------------------------------------------------------------------\n");
	printf ("Base: 0x%08lx, length %ld(0x%04lx)\n", (unsigned long)(data), (long)size, (long)size);
	printf ("----  +0 +1 +2 +3 +4 +5 +6 +7  +8 +9 +A +B +C +D +E +F    01234567 89ABCDEF\n");
//	printf ("---------------------------------------------------------------------------\n");
	
	for (tmp = 0; 
		(tmp + 16) <= size; 
		tmp += 16)
	{
		memset(lineString, 0, sizeof(lineString));
		memset(linechar, 0, sizeof(linechar));
	
		for (column = 0, lineLen = 0;
			column < 16;
			column ++)
		{
			byte = data[tmp + column];
			sprintf(lineString + lineLen, "%02X ", byte & 0xFF);
			
			lineLen += 3;

			if (column < 7)
			{
				linechar[column] = _byte_to_char(byte);
			}
			else if (7 == column)
			{
				linechar[column] = _byte_to_char(byte);
				linechar[column+1] = ' ';
				sprintf(lineString + lineLen, " ");
				lineLen += 1;
			}
			else
			{
				linechar[column+1] = _byte_to_char(byte);
			}
		}

		printf ("%04lX: %s   %s\n", (long)tmp, lineString, linechar);
	}

	/* last line */
	if (tmp < size)
	{
		memset(lineString, 0, sizeof(lineString));
		memset(linechar, 0, sizeof(linechar));
	
		for (column = 0, lineLen = 0;
			column < (size - tmp);
			column ++)
		{
			byte = data[tmp + column];
			sprintf(lineString + lineLen, "%02X ", byte & 0xFF);
			lineLen += 3;

			if (column < 7)
			{
				linechar[column] = _byte_to_char(byte);
			}
			else if (7 == column)
			{
				linechar[column] = _byte_to_char(byte);
				linechar[column+1] = ' ';
				sprintf(lineString + lineLen, " ");
				lineLen += 1;
			}
			else
			{
				linechar[column+1] = _byte_to_char(byte);
			}
		}
#if 1
		for (/* null */;
			column < 16;
			column ++)
		{
			sprintf(lineString + lineLen, "   ");
			lineLen += 3;
		
			if (7 == column)
			{
				sprintf(lineString + lineLen, " ");
				lineLen += 1;
			}
		}
#endif
		printf ("%04lX: %s   %s\n", (long)tmp, lineString, linechar);
	}
	
	printf ("---------------------------------------------------------------------------\n");
	
	/* ends */
}
#else
#define _dump_data(pData, size)
#endif


#endif


/********/
#define __INTERNAL_PACKAGING_FUNCTIONS
#ifdef __INTERNAL_PACKAGING_FUNCTIONS

/* --------------------_copy_uint16----------------------- */
static inline size_t _copy_uint16(void *dst, const void *src)
{
	((uint8_t *)dst)[0] =( (uint8_t *)src)[0];
	((uint8_t *)dst)[1] =( (uint8_t *)src)[1];
	return 2;
}


/* --------------------_dns_gen_req_flags----------------------- */
static uint16_t _dns_gen_req_flags()
{
	uint16_t ret = 0;
	_BITS_SET(ret, (DNS_OPCODE_INQUIRY & 0x0F) << 11);
	return ret;
}


/* --------------------_dns_append_req_data----------------------- */
/* If "name" is NULL, we will use offset to generate this data */
static size_t _dns_append_req_data(uint8_t *buff, int type, const char *name, off_t offset, uint8_t **namePtr)
{
	size_t totalLen = 0;
	uint8_t *start = buff;

	/* complete name */
	if (name)
	{
		uint8_t len = 0;
		uint8_t *lenPtr = buff;
		uint8_t *charPtr = buff + 1;
		BOOL isDone = FALSE;

		do {
			switch (*name)
			{
			/* ordinary characters */
			default:
				*charPtr = *name;
				charPtr ++;
				name ++;
				len ++;
				break;

			/* domain part terminated */
			case '.':
				*lenPtr = len;
				totalLen += len + 1;
				buff += len + 1;
				name ++;
				len = 0;
				lenPtr = buff;
				charPtr = buff + 1;
				break;

			/* string terminated */
			case '\0':
				*charPtr = '\0';
				*lenPtr = len;
				totalLen += len + 2;
				buff += len + 2;
				isDone = TRUE;
				break;
			}
		}
		while (FALSE == isDone);
	}
	//
	/* compressed pointer */
	else
	{
		uint16_t comprVal = 0xC000 | ((uint16_t)offset);
		uint16_t netVal = htons(comprVal);
		buff += _copy_uint16(buff, &netVal);
	}

	/* type */
	{
		uint16_t rrType = htons(type);
		buff += _copy_uint16(buff, &rrType);
	}

	/* class */
	{
		uint16_t inClass = htons(DNS_CLASS_INTERNET);
		buff += _copy_uint16(buff, &inClass);
	}

	/* return */
	if (namePtr) {
		*namePtr = start;
	}
	return (size_t)(buff - start);
}


/* --------------------_dns_get_reply_code----------------------- */
static uint8_t _dns_get_reply_code(int flags)
{
	return (uint8_t)(flags & 0x0F);
}

#endif


/********/
#define __INTERNAL_NETWORKING_FUNCTIONS
#ifdef __INTERNAL_NETWORKING_FUNCTIONS

/* --------------------_dns_INET4_send----------------------- */
static int _dns_INET4_send(int fd, const char *domain, const struct sockaddr_in *to, socklen_t toLen)
{
	struct sockaddr_in dnsSrv;
	uint8_t buff[UDP_PACKAGE_LEN_MAX];
	uint8_t *next = buff;
	uint8_t *nameInPkg = NULL;
	size_t len = 0;
	ssize_t callStat = 0;

	memcpy(&dnsSrv, to, sizeof(dnsSrv));
	dnsSrv.sin_port = htons(DNS_SERVER_PORT);

#ifdef _DNS_DEBUG_FLAG
	{
		char addr[IPV4_STR_LEN + 1];
		inet_ntop(AF_INET, (const void *)(&(dnsSrv.sin_addr)), addr, sizeof(addr));
		_DNS_DB("Request URL \"%s\", DNS server %s", domain, addr);
	}
#endif

	/* set DNS header */
	{
		DNSHeader_st *header = (DNSHeader_st *)next;
		header->transaction_ID = htons(0x1234);		// TODO:
		header->flags = htons(_dns_gen_req_flags());
		header->questions = htons(2);		/* IPv4 and IPv6 */
		header->andwer_RRs = 0;
		header->authority_RRs = 0;
		header->additional_RRs = 0;

		len += sizeof(*header);
		next += sizeof(*header);
	}

	/* set inquiry */
	{
		size_t thisLen = 0;

		thisLen = _dns_append_req_data(next, DNS_RR_TYPE_A, domain, 0, &nameInPkg);
		len += thisLen;
		next += thisLen;

		thisLen = _dns_append_req_data(next, DNS_RR_TYPE_AAAA, domain, 0, &nameInPkg);
		len += thisLen;
		next += thisLen;
	}

	/* send */
	_DNS_DB("Ready to send data:");
	_dump_data(buff, len);
	callStat = sendto(fd, buff, len, 0, (const struct sockaddr *)(&dnsSrv), sizeof(dnsSrv));
	if (callStat < 0) {
		_DNS_DB("Failed to send DNS request: %s", strerror(errno));
		RETURN_ERR(errno);
	}
	else {
		return 0;
	}
}


/* --------------------_dns_INET6_send----------------------- */
static int _dns_INET6_send(int fd, const char *domain, const struct sockaddr_in6 *to, socklen_t toLen)
{
	RETURN_ERR(ENOSYS);
}


#endif


/********/
#define __PUBLIC_FUNCTIONS
#ifdef __PUBLIC_FUNCTIONS

/* --------------------AMCDns_WriteRequest----------------------- */
int AMCDns_WriteRequest(int fd, const char *domain, const struct sockaddr *to, socklen_t toLen)
{
	if ((fd > 0) && domain && to && toLen) {
		/* OK */
	}
	else if (strlen(domain) > DNS_DOMAIN_LEN_MAX) {
		RETURN_ERR(ENAMETOOLONG);
	}
	else {
		RETURN_ERR(EINVAL);
	}

	switch(to->sa_family)
	{
	case AF_INET:
		return _dns_INET4_send(fd, domain, (const struct sockaddr_in *)to, toLen);
		break;
	case AF_INET6:
		return _dns_INET6_send(fd, domain, (const struct sockaddr_in6 *)to, toLen);
		break;
	default:
		RETURN_ERR(EFAULT);
		break;
	}

	return -1;
}



#endif


/* EOF */

