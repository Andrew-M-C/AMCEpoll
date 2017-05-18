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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>

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


#ifndef NULL
#ifndef _DO_NOT_DEFINE_NULL
#define NULL	((void*)0)
#endif
#endif


#define _BITS_ANY_SET(val, bits)		(0 != ((val) & (bits)))
#define _BITS_ALL_SET(val, bits)		((bits) == ((val) & (bits)))
#define _BITS_SET(val, bits)			((val) |= (bits))
#define _BITS_CLR(val, bits)			((val) &= ~(bits))

#define _DNS_RESOLV_FILE			"/etc/resolv.conf"
#define _RANDOM_FILE				"/dev/random"


typedef struct {
	uint16_t      transaction_ID;
	uint16_t      flags;
	uint16_t      questions;
	uint16_t      answer_RRs;
	uint16_t      authority_RRs;
	uint16_t      additional_RRs;
} DNSHeader_st;
#define _DNS_HEADER_LENGTH		(12)


typedef struct {
	uint16_t type;
	uint16_t classIN;
	uint32_t ttl;
	uint16_t length;
} RRHeader_st;		/* without name, the first part */
#define _RR_HEADER_LENGTH		(10)


typedef struct {
	uint16_t type;
	uint16_t classIN;
} QueryHeader_st;		/* without name, the first part */
#define _QUERY_HEADER_LENGTH		(4)


enum {
	DNS_OPCODE_INQUIRY = 0,
	// Others not supported in this file
};

enum {
	DNS_REPLYCODE_NO_ERROR               = 0,
	DNS_REPLYCODE_FMT_ERR                = 1,
	DNS_REPLYCODE_SRV_ERR                = 2,
	DNS_REPLYCODE_NAME_NOT_EXIST         = 3,
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
#define __FORWARD_FUNCTION_DECLARATIONS
#ifdef __FORWARD_FUNCTION_DECLARATIONS

static inline size_t _copy_uint16(void *dst, const void *src);
static inline size_t _copy_uint32(void *dst, const void *src);
static uint16_t _dns_get_reply_code(uint16_t flags);
static uint16_t _random_uint16(void);

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

static void _dump_data(const void *pData, const size_t size)
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
#define __INTERNAL_RESOLVE_FUNCTIONS
#ifdef __INTERNAL_RESOLVE_FUNCTIONS

/* --------------------_dns_resolve_name----------------------- */
static ssize_t _dns_resolve_name(uint8_t *pDNS, uint8_t *pName, char *domain, BOOL domainIsEmpty)
{
	//_DNS_DB("MARK 0x%02x, Offset 0x%04x, First byte 0x%02u", *pName, (unsigned int)(pName - pDNS), *pName);
	/* truncated label */
	if ('\0' == *pName)
	{
		domain[0] = '\0';
		return sizeof(uint8_t);
	}
	/* label */
	else if (_BITS_ANY_SET(*pName, 0xC0))
	{
		uint16_t offset;

		_copy_uint16(&offset, pName);
		offset = ntohs(offset);
		_BITS_CLR(offset, 0xC000);

		_dns_resolve_name(pDNS, pDNS + offset, domain, domainIsEmpty);
		return sizeof(uint16_t);
	}
	/* actual domain */
	else
	{
		uint8_t len = *pName;
		ssize_t ret = len;

		if (domainIsEmpty) {
			ret ++;
			memcpy(domain, pName + 1, len);
			ret += _dns_resolve_name(pDNS, pName + len + 1, domain + len, FALSE);
		}
		else {
			ret ++;
			domain[0] = '.';
			memcpy(domain + 1, pName + 1, len);
			ret += _dns_resolve_name(pDNS, pName + len + 1, domain + len + 1, FALSE);
		}

		return ret;
	}
}


/* --------------------_dns_resolve_RR----------------------- */
static ssize_t _dns_resolve_RR(uint8_t *pDNS, uint8_t *pRR, 
			char para[DNS_DOMAIN_LEN_MAX + 1], char name[DNS_DOMAIN_LEN_MAX + 1], int *rrTypeOut, time_t *ttl)
{
	ssize_t len = 0;
	RRHeader_st rrHeaders;

	//_DNS_DB("Start resolve RR at 0x%04x", (unsigned int)(pRR - pDNS));

	/* resolve name */
	{
		len += _dns_resolve_name(pDNS, pRR, para, TRUE);
		//_DNS_DB("Start domain: %s", para);
	}

	/* resolve headers */
	{
		memcpy(&rrHeaders, pRR + len, _RR_HEADER_LENGTH);
		rrHeaders.type    = ntohs(rrHeaders.type);
		rrHeaders.classIN = ntohs(rrHeaders.classIN);
		rrHeaders.ttl     = ntohl(rrHeaders.ttl);
		rrHeaders.length  = ntohs(rrHeaders.length);

		len += _RR_HEADER_LENGTH;
	}

	/* resolve RR text */
	{
		switch(rrHeaders.type)
		{
		default:
			_DNS_DB("Unsupported Type %d", rrHeaders.type);
			break;

		case DNS_RR_TYPE_AAAA:
			inet_ntop(AF_INET6, pRR, name, DNS_DOMAIN_LEN_MAX);
			//_DNS_DB("%s - IPv6 %s", para, name);
			break;

		case DNS_RR_TYPE_A:
			sprintf(name, "%u.%u.%u.%u", (pRR + len)[0], (pRR + len)[1], (pRR + len)[2], (pRR + len)[3]);
			//_DNS_DB("%s - IPv4 %u.%u.%u.%u", para, (pRR + len)[0], (pRR + len)[1], (pRR + len)[2], (pRR + len)[3]);
			break;

		case DNS_RR_TYPE_CNAME:
			_dns_resolve_name(pDNS, pRR + len, name, TRUE);
			//_DNS_DB("%s - CNAME: %s", para, name);
			break;

		case DNS_RR_TYPE_NS:
			_dns_resolve_name(pDNS, pRR + len, name, TRUE);
			//_DNS_DB("%s - name server: %s", para, name);
			break;
		}

		len += rrHeaders.length;

		if (ttl) {
			*ttl = (time_t)(rrHeaders.ttl);
		}

		if (rrTypeOut) {
			*rrTypeOut = (int)(rrHeaders.type);
		}
	}

	/* return */
	return len;
}	


/* --------------------_dns_resolve_query----------------------- */
static ssize_t _dns_resolve_query(uint8_t *pDNS, uint8_t *pQuery, char name[DNS_DOMAIN_LEN_MAX + 1], int *queryTypeOut)
{
	ssize_t len = 0;
	QueryHeader_st queryHeaders;

	/* resolve name */
	{
		len += _dns_resolve_name(pDNS, pQuery, name, TRUE);
		//_DNS_DB("Start domain: %s", name);
	}

	/* resolve headers */
	{
		memcpy(&queryHeaders, pQuery + len, _QUERY_HEADER_LENGTH);
		queryHeaders.type    = ntohs(queryHeaders.type);
		queryHeaders.classIN = ntohs(queryHeaders.classIN);

		len += _QUERY_HEADER_LENGTH;
	}

	/* return */
	return len;
}


/* --------------------_dns_resolve_and_return_answers----------------------- */
static struct AMCDnsResult *_dns_resolve_and_return_answers(uint8_t *pDNS, uint8_t *pAns, size_t ansCount, struct AMCDnsResult **last, size_t *dataLen)
{
	/* local variables */
	struct AMCDnsResult *ret = NULL;
	struct AMCDnsResult *next = NULL;

	char para[DNS_DOMAIN_LEN_MAX + 1];
	char value[DNS_DOMAIN_LEN_MAX + 1];
	int rrType;
	uint8_t *pOrig = pAns;

	size_t index = 0;

	/* resolve the whole answer */
	for (index = 0; index < ansCount; index ++)
	{
		size_t len = 0;
		time_t ttl = 0;
		pAns += _dns_resolve_RR(pDNS, pAns, para, value, &rrType, &ttl);

		switch(rrType)
		{
		case DNS_RR_TYPE_A:
		case DNS_RR_TYPE_AAAA:
		case DNS_RR_TYPE_CNAME:
		case DNS_RR_TYPE_NS:
			if (NULL == ret) {
				ret = malloc(sizeof(struct AMCDnsResult));
				if (NULL == ret) {
					_DNS_DB("Failed in malloc: %s", strerror(errno));
					return NULL;
				}
				else {
					next = ret;
				}
			}
			else {
				next->next = malloc(sizeof(struct AMCDnsResult));
				if (NULL == next->next) {
					_DNS_DB("Failed in malloc: %s", strerror(errno));
					continue;												/* !!!!!! */
				}
				else {
					next = next->next;
				}
			}

			memset(next, 0, sizeof(*next));
			len = strlen(para) + 1;
			next->ttl = ttl;
			next->name = malloc(len);
			if (next->name) {
				memcpy(next->name, para, len);
			}
			break;
		default:
			_DNS_DB("Invalid type");
			break;
		}

		switch(rrType)
		{
		case DNS_RR_TYPE_A:
			len = strlen(value) + 1;
			next->ipv4 = malloc(len);
			if (next->ipv4) {
				memcpy(next->ipv4, value, len);
			}
			break;
		case DNS_RR_TYPE_AAAA:
			len = strlen(value) + 1;
			next->ipv6 = malloc(len);
			if (next->ipv6) {
				memcpy(next->ipv6, value, len);
			}
			break;
		case DNS_RR_TYPE_CNAME:
			len = strlen(value) + 1;
			next->cname = malloc(len);
			if (next->cname) {
				memcpy(next->cname, value, len);
			}
			break;
		case DNS_RR_TYPE_NS:
			len = strlen(value) + 1;
			next->namesvr = malloc(len);
			if (next->namesvr) {
				memcpy(next->namesvr, value, len);
			}
			break;
		default:
			break;
		}
	}

	/* return */
	if (last) {
		*last = next;
	}
	if (dataLen) {
		*dataLen = (size_t)(pAns - pOrig);
	}
	return ret;
}


/* --------------------_dns_resolve----------------------- */
static struct AMCDnsResult *_dns_resolve(uint8_t *pDNS, size_t len, BOOL detail)
{
	struct AMCDnsResult *ret = NULL;
	struct AMCDnsResult *last = NULL;
	char name[DNS_DOMAIN_LEN_MAX + 1] = "";
	uint8_t *data = pDNS;

	size_t quesRRs = 0;
	size_t ansRRs = 0;
	size_t authRRs = 0;
	size_t addiRRs = 0;

	/* read DNS header */
	if (len < _DNS_HEADER_LENGTH) {
		return NULL;
	} else {
		DNSHeader_st header;
		uint16_t replyCode = 0;
		BOOL isOK = FALSE;

		memcpy(&header, data, _DNS_HEADER_LENGTH);
		_DNS_DB("Get DNS reply flags: 0x%04x", (int)ntohs(header.flags));

		quesRRs = ntohs(header.questions);
		ansRRs  = ntohs(header.answer_RRs);
		authRRs = ntohs(header.authority_RRs);
		addiRRs = ntohs(header.additional_RRs);
		replyCode = _dns_get_reply_code(ntohs(header.flags));
		switch (replyCode)
		{
		case DNS_REPLYCODE_NO_ERROR:
			isOK = TRUE;
			break;
		case DNS_REPLYCODE_FMT_ERR:
			errno = ENOEXEC;
			break;
		case DNS_REPLYCODE_SRV_ERR:
			errno = EBUSY;
			break;
		case DNS_REPLYCODE_NAME_NOT_EXIST:
			errno = ENXIO;
			break;
		case DNS_REPLYCODE_REJECTED:
			errno = EACCES;
			break;
		case DNS_REPLYCODE_NAME_SHOULD_NOT_APPEAR:
			errno = ENOENT;
			break;
		case DNS_REPLYCODE_RR_NOT_EXIST:
			errno = ENXIO;
			break;
		case DNS_REPLYCODE_RR_INQUIRY_FAIL:
			errno = ECHILD;
			break;
		case DNS_REPLYCODE_SRV_AUTH_FAIL:
			errno = EACCES;
			break;
		case DNS_REPLYCODE_NAME_OUT_OF_AREA:
			errno = ENFILE;
			break;
		default:
			errno = EIO;
			break;
		}

		if (FALSE == isOK) {
			_DNS_DB("Get DNS failed, code %d", (int)replyCode);
			return NULL;
		}
		else {
			// TODO:
			/* examine other flags */
			uint16_t flags = ntohs(header.flags);
			if (_BITS_ANY_SET(flags, (1 << 10))) {
				_DNS_DB("Server is authoritative.");
			} else {
				_DNS_DB("Server is NOT authoritative.");
			}
		}

		_DNS_DB("Got %d question(s)", (int)quesRRs);

		data += _DNS_HEADER_LENGTH;
		len -= _DNS_HEADER_LENGTH;
	}

	/* read queries */
	if (quesRRs > 0)
	{
		_DNS_DB("Queries:");
		do {
			size_t thisLen = _dns_resolve_query(pDNS, data, name, NULL);

			//_DNS_DB("%s", name);
			data += thisLen;
			len -= thisLen;
			quesRRs--;
			
		} while ((quesRRs > 0) && (len > 0));
	}

	/* read answers */
	if (ansRRs > 0)
	{
		size_t thisLen = 0;
		ret = _dns_resolve_and_return_answers(pDNS, data, ansRRs, &last, &thisLen);

		data += thisLen;
		len -= thisLen;
		//_DNS_DB("Last: %s", last ? last->name : "NULL");
	}

	/* read auth RRs */
	if (detail && (authRRs > 0))
	{
		size_t thisLen = 0;
		if (last) {
			struct AMCDnsResult *lastlast = last;
			lastlast->next = _dns_resolve_and_return_answers(pDNS, data, authRRs, &last, &thisLen);
		}
		else {
			ret = _dns_resolve_and_return_answers(pDNS, data, authRRs, &last, &thisLen);
		}

		data += thisLen;
		len -= thisLen;
		//_DNS_DB("Last: %s", last ? last->name : "NULL");
	}

	/* read additional RRs */
	if (detail && (addiRRs > 0))
	{
		size_t thisLen = 0;
		if (last) {
			struct AMCDnsResult *lastlast = last;
			lastlast->next = _dns_resolve_and_return_answers(pDNS, data, addiRRs, &last, &thisLen);
		}
		else {
			ret = _dns_resolve_and_return_answers(pDNS, data, addiRRs, &last, &thisLen);
		}

		data += thisLen;
		len -= thisLen;
		//_DNS_DB("Last: %s", last ? last->name : "NULL");
	}

	/* return */
	return ret;
}


#endif


/********/
#define __INTERNAL_MISC_TOOLS
#ifdef __INTERNAL_MISC_TOOLS

/* --------------------_copy_uint16----------------------- */
static inline size_t _copy_uint16(void *dst, const void *src)
{
	((uint8_t *)dst)[0] =( (uint8_t *)src)[0];
	((uint8_t *)dst)[1] =( (uint8_t *)src)[1];
	return 2;
}


/* --------------------_copy_uint32----------------------- */
static inline size_t _copy_uint32(void *dst, const void *src)
{
	((uint8_t *)dst)[0] =( (uint8_t *)src)[0];
	((uint8_t *)dst)[1] =( (uint8_t *)src)[1];
	((uint8_t *)dst)[2] =( (uint8_t *)src)[2];
	((uint8_t *)dst)[3] =( (uint8_t *)src)[3];
	return 2;
}


/* --------------------_random_uint16----------------------- */
static uint16_t _random_uint16()
{
	uint16_t ret = 0;
	int fd = open(_RANDOM_FILE, O_RDONLY);
	if (fd < 0) {
		ret = (uint16_t)rand();
	}
	else {
		read(fd, &ret, sizeof(ret));
		close(fd);
	}

	return ret;
}


/* --------------------_trim_str----------------------- */
/* remove all heading and trailing blanks and replace all "isspace()" as ' ' */
static size_t _trim_str(char *str)
{
	char *srh = str;	/* current examing from */
	char *dst = str;	/* current copy target */
	BOOL isLastBlank = FALSE;

	if ('\0' == *srh) {
		return 0;
	}

	/* remove leading blanks */
	while (isspace(*srh)) {
		srh ++;
	}

	/* remove inner mutiple blanks */
	while ('\0' != *srh)
	{
		if (isspace(*srh))
		{
			if (isLastBlank) {
				/* do nothing */
			}
			else {
				*dst = ' ';
				dst ++;
				isLastBlank = TRUE;
			}
		}
		else
		{
			isLastBlank = FALSE;
			*dst = *srh;
			dst ++;
		}

		srh ++;
	}

	/* return */
	if (isLastBlank) {
		dst --;
	}
	*dst = '\0';
	dst ++;
	return (size_t)(dst - str);
}


#endif


/********/
#define __INTERNAL_PACKAGING_FUNCTIONS
#ifdef __INTERNAL_PACKAGING_FUNCTIONS

/* --------------------_dns_gen_req_flags----------------------- */
static uint16_t _dns_gen_req_flags()
{
	uint16_t ret = 0;
	_BITS_SET(ret, (DNS_OPCODE_INQUIRY & 0x0F) << 11);
	_BITS_SET(ret, (1 << 8));		/* need recursion */
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
static uint16_t _dns_get_reply_code(uint16_t flags)
{
	return (uint16_t)(flags & 0x0F);
}


#endif


/********/
#define __INTERNAL_NETWORKING_FUNCTIONS
#ifdef __INTERNAL_NETWORKING_FUNCTIONS

/* --------------------_sock_sendto----------------------- */
static ssize_t _sock_sendto(int fd, const void *buff, size_t nbyte, int flags, const struct sockaddr *to, socklen_t tolen)
{
	ssize_t ret = 0;
	ssize_t callStat = 0;
	BOOL isDone = FALSE;

	if (fd < 0) {
		RETURN_ERR(EBADF);
	}
	if (NULL == buff) {
		RETURN_ERR(EINVAL);
	}
	if (0 == nbyte) {
		return 0;
	}

	do {
		callStat = sendto(fd, buff + ret, nbyte - ret, flags, to, tolen);
		if (callStat < 0)
		{
			if (EINTR == errno) {
				/* continue */
			} else if (EAGAIN == errno) {
				isDone = TRUE;
			} else {
				_DNS_DB("Fd %d error in sendto(): %s", fd, strerror(errno));
				ret = (ret > 0) ? ret : -1;
				isDone = TRUE;
			}
		}
		else {
			ret += callStat;
			if (ret >= nbyte) {
				isDone = TRUE;
			}
		}
	}
	while (FALSE == isDone);

	return ret;
}


/* --------------------_sock_recvfrom----------------------- */
static ssize_t _sock_recvfrom(int fd, void *rawBuf, size_t nbyte, int flags, struct sockaddr *from, socklen_t *fromlen)
{
	int err = 0;
	ssize_t callStat = 0;
	ssize_t ret = 0;
	uint8_t *buff = (uint8_t *)rawBuf;
	BOOL isDone = FALSE;

	if (fd < 0) {
		RETURN_ERR(EBADF);
	}
	if (NULL == buff) {
		RETURN_ERR(EINVAL);
	}
	if (0 == nbyte) {
		return 0;
	}

	/* loop read */
	do {
		callStat = recvfrom(fd, buff + ret, nbyte - ret, flags, from, fromlen);
		err = errno;

		if (0 == callStat) {
			/* EOF */
			_DNS_DB("Fd %d EOF", fd);
			//ret = 0;
			isDone = TRUE;
		}
		else if (callStat < 0)
		{
			if (EINTR == err) {
				/* continue */
			} else if (EAGAIN == err) {
				_DNS_DB("Fd %d EAGAIN", fd);
				isDone = TRUE;
			} else {
				_DNS_DB("Fd %d error in recvfrom(): %s", fd, strerror(err));
				ret = -1;
				isDone = TRUE;
			}
		}
		else
		{

#ifdef _DNS_DEBUG_FLAG
#include "AMCEpoll.h"
			char addr[256] = "";
			AMCFd_SockaddrToStr(from, addr, sizeof(addr));
			_DNS_DB("Read from %s", addr);
#endif
			ret += callStat;

			if (ret >= nbyte) {
				isDone = TRUE;
			}
		}
	} while (FALSE == isDone);
	// end of "while (FALSE == isDone)"

	return ret;
}

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
		char addr[IPV4_STR_LEN_MAX + 1];
		inet_ntop(AF_INET, (const void *)(&(dnsSrv.sin_addr)), addr, sizeof(addr));
		_DNS_DB("Request URL \"%s\", DNS server %s", domain, addr);
	}
#endif

	/* set DNS header */
	{
		DNSHeader_st header;

		header.transaction_ID = _random_uint16();
		header.flags = htons(_dns_gen_req_flags());
		header.questions = htons(1);		/* IPv4 only */
		header.answer_RRs = 0;
		header.authority_RRs = 0;
		header.additional_RRs = 0;

		memcpy(next, &header, _DNS_HEADER_LENGTH);

		len += _DNS_HEADER_LENGTH;
		next += _DNS_HEADER_LENGTH;
	}

	/* set inquiry */
	{
		size_t thisLen = 0;

		thisLen = _dns_append_req_data(next, DNS_RR_TYPE_A, domain, 0, &nameInPkg);
		len += thisLen;
		next += thisLen;
	}

	/* send */
	_DNS_DB("Ready to send data:");
	_dump_data(buff, len);
	callStat = _sock_sendto(fd, buff, len, 0, (const struct sockaddr *)(&dnsSrv), sizeof(dnsSrv));
	if (callStat < 0) {
		_DNS_DB("Failed to send DNS request: %s", strerror(errno));
		RETURN_ERR(errno);
	}
	else {
		_DNS_DB("DNS sent %ld bytes", (long)callStat);
		return 0;
	}
}


/* --------------------_dns_INET6_send----------------------- */
static int _dns_INET6_send(int fd, const char *domain, const struct sockaddr_in6 *to, socklen_t toLen)
{
	// TODO:
	RETURN_ERR(ENOSYS);
}


#endif


/********/
#define __PUBLIC_FUNCTIONS
#ifdef __PUBLIC_FUNCTIONS


/* --------------------AMCDns_GetDefaultServer----------------------- */
int AMCDns_GetDefaultServer(struct sockaddr *dns, int index)
{
	if (NULL == dns) {
		RETURN_ERR(EINVAL);
	}
	else if (index < 0) {
		RETURN_ERR(EINVAL);
	}
	else
	{
		BOOL isDone = FALSE;
		int err = 0;
		size_t dnsHitCount = 0;
		char line[DNS_DOMAIN_LEN_MAX * 2 + 1] = "";
		size_t lineLen = 0;
		const char nameserver[] = "nameserver";
		FILE *resoFile = NULL;

		/* open system config file */
		resoFile = fopen(_DNS_RESOLV_FILE, "r");
		if (NULL == resoFile) {
			err = errno;
			goto ENDS;
		}

		/* read data */
		err = ENOENT;		/* preset as NOT FOUND status */
		do {
			if (NULL == fgets(line, sizeof(line), resoFile)) {
				isDone = TRUE;
			}
			else {
				lineLen = _trim_str(line);
				//_DNS_DB("Get DNS resolv: %s", line);

				if (lineLen <= sizeof(nameserver)) {
					/* line is too short, it can't be */
				}
				else if (0 == memcmp(line, nameserver, sizeof(nameserver) - 1))
				{
					/* DNS config hit! */
					if (dnsHitCount == index)
					{
						char *addrStr = line + sizeof(nameserver);
						int isOK = 0;

						_DNS_DB("Hit name server: %s", addrStr);
						if (strstr(addrStr, ":"))
						{
							/* IPv6 address */
							struct sockaddr_in6 *addr = (struct sockaddr_in6 *)dns;
							addr->sin6_family = AF_INET6;
							addr->sin6_port = htons(DNS_SERVER_PORT);
							isOK = inet_pton(AF_INET6, addrStr, &(addr->sin6_addr));
						}
						else
						{
							/* IPv4 address */
							struct sockaddr_in *addr  = (struct sockaddr_in *)dns;
							addr->sin_family = AF_INET;
							addr->sin_port = htons(DNS_SERVER_PORT);
							isOK = inet_pton(AF_INET, addrStr, &(addr->sin_addr));
						}

						if (isOK) {
							err = 0;
						} else {
							err = EIO;
						}
					}
					else {
						dnsHitCount ++;
						/* continue searching for next DNS server IP address */
					}
				}
				else {
					/* not the line we are searching */
				}
				// end of: "else (0 == memcmp(line, nameserver, sizeof(nameserver) - 1))"
			}
			// end of: "else (NULL == fgets(line, sizeof(line), resoFile))"
		}
		while (FALSE == isDone);
		

		/* return */
ENDS:
		if (resoFile) {
			fclose(resoFile);
			resoFile = NULL;
		}

		if (0 == err) {
			return 0;
		} else {
			RETURN_ERR(err);
		}
	}
}


/* --------------------AMCDns_SendRequest----------------------- */
int AMCDns_SendRequest(int fd, const char *domain, const struct sockaddr *to, socklen_t toLen)
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


/* --------------------AMCDns_RecvAndResolve----------------------- */
struct AMCDnsResult *AMCDns_RecvAndResolve(int fd, struct sockaddr *from, BOOL detail)
{
	struct AMCDnsResult *ret = NULL;
	struct sockaddr addr;
	ssize_t len = 0;
	socklen_t sockLen = sizeof(addr);
	uint8_t buff[UDP_PACKAGE_LEN_MAX * 2];

	if (fd > 0) {
		/* OK */
	} else {
		errno = EINVAL;
		goto ENDS;
	}

	/* recvfrom */
	len = _sock_recvfrom(fd, buff, sizeof(buff), 0, &addr, &sockLen);
	if (len > 0) {
		_DNS_DB("Read data:");
		_dump_data(buff, len);
	} 
	else if (0 == len) {
		errno = 0;
		_DNS_DB("EOF");
		goto ENDS;
	}
	else {
		_DNS_DB("Failed in recvfrom(): %s", strerror(errno));
		goto ENDS;
	}

	/* resolve */
	ret = _dns_resolve(buff, len, detail);

	/* return */
ENDS:
	if (from) {
		memcpy(from, &addr, sockLen);
	}
	return ret;
}


/* --------------------AMCDns_FreeResult----------------------- */
int AMCDns_FreeResult(struct AMCDnsResult *obj)
{
	if (obj) {
		struct AMCDnsResult *next;
		while(obj)
		{
			next = obj->next;
			if (obj->name) {
				free(obj->name);
			}
			if (obj->cname) {
				free(obj->cname);
			}
			if (obj->ipv4) {
				free(obj->ipv4);
			}
			if (obj->ipv6) {
				free(obj->ipv6);
			}
			free(obj);
			obj = next;
		}
		return 0;
	}
	else {
		RETURN_ERR(EINVAL);
	}
}


#endif


/* EOF */

