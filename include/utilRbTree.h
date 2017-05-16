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
};

// RB-tree key type
typedef uint64_t RbKey_t;

// hash function
typedef RbKey_t (*hash_func)(const char *key);

// RB-tree. ALL data elements should be used internally
struct UtilRbTree {
	RbStatus_t         status;
	size_t             count;
	struct RbTreeNode *nodes;
	hash_func          hash_func;
};

// parameter for check callback
typedef enum {
	RbCheck_IncAll = 0,
	RbCheck_DecAll,

	RbCheck_IncLower,
	RbCheck_IncLowerEqual,
	RbCheck_DecLower,
	RbCheck_DecLowerEqual,

	RbCheck_DecGreater,
	RbCheck_DecGreaterEuqal,
	RbCheck_IncGreater,
	RbCheck_IncGreaterEqual,

	RbCheck_IllegalWay	// SHOULD placed in the end
} RbCheck_t;

// check callback parameter in order to shorten callback function length
struct RbCheckPara {
	struct UtilRbTree *tree;
	RbCheck_t          how;
	RbKey_t            than;
	RbKey_t            key;
	void              *object;
};

// check function
typedef void (*check_func)(const struct RbCheckPara *para, void *checkArg);

// error codes
enum {
	RB_NO_ERROR    = 0,
	RB_ERR_UNKNOWN = 256,
	RB_ERR_ALREADY_INIT,
	RB_ERR_NOT_INIT,
	RB_ERR_NOT_EMPTY,
	RB_ERR_NOT_CHECKING,
	RB_ERR_RECURSIVE_CHECK,
};



/* public functions */
struct UtilRbTree *
	utilRbTree_New(void);
int 
	utilRbTree_Destory(struct UtilRbTree *tree);
int 
	utilRbTree_Init(struct UtilRbTree *tree);
int 
	utilRbTree_Clean(struct UtilRbTree *tree);
int 
	utilRbTree_SetHashFunc(struct UtilRbTree *tree, hash_func func);
hash_func 
	utilRbTree_GetHashFunc(const struct UtilRbTree *tree);
int 
	utilRbTree_SetObject(struct UtilRbTree *tree, void *obj, RbKey_t key, void **prevObj);
void *
	utilRbTree_GetObject(const struct UtilRbTree *tree, RbKey_t key);
void *
	utilRbTree_DrainObject(struct UtilRbTree *tree, RbKey_t key);
int 
	utilRbTree_CheckObjects(struct UtilRbTree *tree, RbCheck_t how, RbKey_t than, check_func callback, void *checkArg);
int 
	utilRbTree_AbortCheck(struct UtilRbTree *tree);
const char *
	utilRbTree_StrError(int error);


#endif
/* EOF */

