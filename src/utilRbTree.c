/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	utilRbTree.c
	
	Description: 	
	    This file implements common Red-black tree with uint64_t key. 
			
	History:
		2017-05-16: File created as "utilRbTree.c"

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

#include "utilRbTree.h"


#endif


/********/
#define __DATA_TYPES
#ifdef __DATA_TYPES


#ifndef NULL
#ifndef _DO_NOT_DEFINE_NULL
#define NULL	((void*)0)
#endif
#endif


struct RbTreeNode {
	key_t      key;
	void      *obj;
	struct RbTreeNode *left;
	struct RbTreeNode *right;
};


#define _BITS_ANY_SET(val, bits)		(0 != ((val) & (bits)))
#define _BITS_ALL_SET(val, bits)		((bits) == ((val) & (bits)))
#define _BITS_SET(val, bits)			((val) |= (bits))
#define _BITS_CLR(val, bits)			((val) &= ~(bits))


#endif


/********/
#define __PUBLIC_FUNCTIONS
#ifdef __PUBLIC_FUNCTIONS

/* --------------------utilRbTree_New----------------------- */
struct UtilRbTree *utilRbTree_New()
{
	return NULL;
}


#endif


/* EOF */

