/*******************************************************************************
	Copyright (C) 2017 by Andrew Chang <laplacezhang@126.com>
	Licensed under the LGPL v2.1, see the file COPYING in base directory.
	
	File name: 	utilRbTree.h
	
	Description: 	
	    This file provides common Red-black tree with uint64_t key. 
			
	History:
		2017-05-07: File created as "utilRbTree.h"

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

#ifndef __UTIL_RB_TREE_H__
#define __UTIL_RB_TREE_H__

/* headers */
#include <stdint.h>

/* data definitions */
/* all data structures should be used internally */
struct UtilRbTree {
	// TODO:
	
};


/* public functions */
int 
	utilRbTree_Init(struct UtilRbTree *tree);
int 
	utilRbTree_Clean(struct UtilRbTree *tree);
int 
	utilRbTree_SetObject(struct UtilRbTree *tree, void *obj, uint64_t key);


#endif
/* EOF */

