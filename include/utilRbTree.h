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
#include <unistd.h>

/* data definitions */

// Ahead declaration
struct RbTreeNode;

// RB-tree status code masks
typedef unsigned int RbStatus_t;
enum {
	RbStatus_InitOK         = (1 << 0),
	RbStatus_IsChecking     = (1 << 1),
	RbStatus_AbortCheck     = (1 << 2),
	RbStatus_BanHashFuncSet = (1 << 3),
};

// RB-tree key type
typedef uint64_t key_t;

// hash function
typedef key_t (*hash_func)(const char *key);

// RB-tree. ALL data elements should be used internally
struct UtilRbTree {
	RbStatus_t         status;
	size_t             count;
	struct RbTreeNode *start;
	hash_func          hash_func;
};

// parameter for check callback
typedef enum {
	RbCheck_All           = 0,
	RbCheck_Lower,
	RbCheck_LowerEqual,
	RbCheck_Greater,
	RbCheck_GreaterEqual,

	RbCheck_IllegalWay	// SHOULD placed in the end
} RbCheck_t;

// check callback parameter in order to shorten callback function length
struct RbCheckPara {
	struct UtilRbTree *tree;
	RbCheck_t          how;
	key_t              key;
	void              *object;
};

// check function
typedef void (*check_func)(const struct RbCheckPara *para, void *checkArg);



/* public functions */
struct UtilRbTree *
	utilRbTree_New(void);
int 
	utilRbTree_Destory(struct UtilRbTree *tree);
int 
	utilRbTree_SetHashFunc(struct UtilRbTree *tree, hash_func func);
hash_func 
	utilRbTree_GetHashFunc(const struct UtilRbTree *tree);
int 
	utilRbTree_Init(struct UtilRbTree *tree);
int 
	utilRbTree_Clean(struct UtilRbTree *tree);
int 
	utilRbTree_SetObject(struct UtilRbTree *tree, void *obj, key_t key, void **prevObj);
void *
	utilRbTree_GetObject(const struct UtilRbTree *tree, key_t key);
void *
	utilRbTree_DrainObject(struct UtilRbTree *tree, key_t key);
int 
	utilRbTree_CheckObjects(struct UtilRbTree *tree, RbCheck_t how, key_t target, void *checkArg);
int 
	utilRbTree_AbortCheck(struct UtilRbTree *tree);


#endif
/* EOF */

