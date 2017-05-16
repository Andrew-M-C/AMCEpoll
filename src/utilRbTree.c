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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define _RB_TREE_DEBUG_FLAG


#endif


/********/
#define __DATA_TYPES
#ifdef __DATA_TYPES


#ifdef _RB_TREE_DEBUG_FLAG
#define _RB_DB(fmt, args...)		printf("[DNS - %04d] "fmt"\n", (int)__LINE__, ##args)
#define _RB_MARK()					_RB_DB("--- MARK ---");
#else
#define _RB_DB(fmt, args...)
#define _RB_MARK()
#endif


#ifndef NULL
#ifndef _DO_NOT_DEFINE_NULL
#define NULL	((void*)0)
#endif
#endif


#ifndef BOOL
#ifndef _DO_NOT_DEF_BOOL
#define BOOL	int
#define FALSE	(0)
#define TRUE	(!FALSE)
#endif
#endif


struct RbTreeNode {
	RbKey_t    key;
	void      *obj;
	struct RbTreeNode *left;
	struct RbTreeNode *right;
};


#define _BITS_ANY_SET(val, bits)		(0 != ((val) & (bits)))
#define _BITS_ALL_SET(val, bits)		((bits) == ((val) & (bits)))
#define _BITS_SET(val, bits)			((val) |= (bits))
#define _BITS_CLR(val, bits)			((val) &= ~(bits))


#define _RET_RB_ERR(err)	\
	do{\
		if (err >= RB_ERR_UNKNOWN) {\
			errno = EPERM;\
			return (0 - err);\
		} else if (err <= (0 - RB_ERR_UNKNOWN)) {\
			errno = EPERM;\
			return err;\
		} else if (err < 0) {\
			errno = 0 - err;\
			return err;\
		} else {\
			errno = err;\
			return (0 - err);\
		}\
	}while(0)


#endif


/********/
#define __HASH_FUNCTIONS
#ifdef __HASH_FUNCTIONS

/* --------------------_hash_u64_func----------------------- */
static RbKey_t _hash_u64_func(const char *key)
{
	/* use BKDRHash */
	const RbKey_t seed = 1313131; // 31 131 1313 13131 131313 etc..
	RbKey_t hash = 0;
	while (*key) {
		hash = hash * seed + (*key++);
	}
	return hash & ((RbKey_t)0x7FFFFFFFFFFFFFFF);
}


/* --------------------_hash_u32_func----------------------- */
static RbKey_t _hash_u32_func(const char *key)
{
	/* use BKDRHash */
	const RbKey_t seed = 131; // 31 131 1313 13131 131313 etc..
	RbKey_t hash = 0;
	while (*key) {
		hash = hash * seed + (*key++);
	}
	return hash & 0x7FFFFFFF;
}


/* --------------------_hash_u64_func----------------------- */
static void _hash_set_default_func(struct UtilRbTree *tree)
{
	if (8 == sizeof(RbKey_t)) {
		tree->hash_func = _hash_u64_func;
	} else {
		tree->hash_func = _hash_u32_func;
	}
}


#endif


/********/
#define __CHECK_FUNCTIONS
#ifdef __CHECK_FUNCTIONS

/* --------------------_check_invoke----------------------- */
static inline void _check_invoke(struct UtilRbTree *tree, RbCheck_t how, RbKey_t than, struct RbTreeNode *node, check_func callback, void *checkArg)
{
	struct RbCheckPara para;
	para.tree = tree;
	para.how  = how;
	para.than = than;
	para.key  = node->key;
	para.object = node->obj;

	callback(&para, checkArg);
}


/* --------------------_check_tree_children_inc_all_recursively----------------------- */
static void _check_tree_children_inc_all_recursively(struct UtilRbTree *tree, struct RbTreeNode *node, check_func callback, void *checkArg)
{
	if (_BITS_ALL_SET(tree->status, RbStatus_AbortCheck)) {
		return;
	}
	if (NULL == node) {
		return;
	}

	/* left */
	_check_tree_children_inc_all_recursively(tree, node->left, callback, checkArg);

	/* self */
	if (FALSE == _BITS_ALL_SET(tree->status, RbStatus_AbortCheck)) {
		_check_invoke(tree, RbCheck_IncAll, 0, node, callback, checkArg);
	}

	/* right */
	_check_tree_children_inc_all_recursively(tree, node->right, callback, checkArg);

}


/* --------------------_check_tree_children_dec_all_recursively----------------------- */
static void _check_tree_children_dec_all_recursively(struct UtilRbTree *tree, struct RbTreeNode *node, check_func callback, void *checkArg)
{
	if (_BITS_ALL_SET(tree->status, RbStatus_AbortCheck)) {
		return;
	}
	if (NULL == node) {
		return;
	}

	/* right */
	_check_tree_children_dec_all_recursively(tree, node->right, callback, checkArg);

	/* self */
	if (FALSE == _BITS_ALL_SET(tree->status, RbStatus_AbortCheck)) {
		_check_invoke(tree, RbCheck_DecAll, 0, node, callback, checkArg);
	}

	/* left */
	_check_tree_children_dec_all_recursively(tree, node->left, callback, checkArg);

}


/* --------------------_check_tree_children_inc_lower_recursively----------------------- */
static void _check_tree_children_inc_lower_recursively(struct UtilRbTree *tree, struct RbTreeNode *node, RbKey_t than, check_func callback, void *checkArg)
{
	if (_BITS_ALL_SET(tree->status, RbStatus_AbortCheck)) {
		return;
	}
	if (NULL == node) {
		return;
	}

	/* left */
	_check_tree_children_inc_lower_recursively(tree, node->left, than, callback, checkArg);

	/* self */
	if (FALSE == _BITS_ALL_SET(tree->status, RbStatus_AbortCheck))
	{
		if (node->key < than) {
			_check_invoke(tree, RbCheck_IncLower, than, node, callback, checkArg);
		}
		else {
			/* done */
			return;
		}
	}

	/* right */
	_check_tree_children_inc_lower_recursively(tree, node->right, than, callback, checkArg);

}


/* --------------------_check_tree_children_inc_lowerequal_recursively----------------------- */
static void _check_tree_children_inc_lowerequal_recursively(struct UtilRbTree *tree, struct RbTreeNode *node, RbKey_t than, check_func callback, void *checkArg)
{
	if (_BITS_ALL_SET(tree->status, RbStatus_AbortCheck)) {
		return;
	}
	if (NULL == node) {
		return;
	}

	/* left */
	_check_tree_children_inc_lowerequal_recursively(tree, node->left, than, callback, checkArg);

	/* self */
	if (FALSE == _BITS_ALL_SET(tree->status, RbStatus_AbortCheck))
	{
		if (node->key <= than) {
			_check_invoke(tree, RbCheck_IncLowerEqual, than, node, callback, checkArg);
		}
		else {
			/* done */
			return;
		}
	}

	/* right */
	_check_tree_children_inc_lowerequal_recursively(tree, node->right, than, callback, checkArg);

}


/* --------------------_check_tree_children_dec_lower_recursively----------------------- */
static void _check_tree_children_dec_lower_recursively(struct UtilRbTree *tree, struct RbTreeNode *node, RbKey_t than, check_func callback, void *checkArg)
{
	if (_BITS_ALL_SET(tree->status, RbStatus_AbortCheck)) {
		return;
	}
	if (NULL == node) {
		return;
	}

	/* right */
	if (node->right && node->right->key < than) {
		_check_tree_children_dec_lower_recursively(tree, node->right, than, callback, checkArg);
	}
	
	/* self */
	if (FALSE == _BITS_ALL_SET(tree->status, RbStatus_AbortCheck))
	{
		if (node->key < than) {
			_check_invoke(tree, RbCheck_DecLower, than, node, callback, checkArg);
		}
	}

	/* left */
	_check_tree_children_dec_lower_recursively(tree, node->left, than, callback, checkArg);

}


/* --------------------_check_tree_children_dec_lowereuqal_recursively----------------------- */
static void _check_tree_children_dec_lowereuqal_recursively(struct UtilRbTree *tree, struct RbTreeNode *node, RbKey_t than, check_func callback, void *checkArg)
{
	if (_BITS_ALL_SET(tree->status, RbStatus_AbortCheck)) {
		return;
	}
	if (NULL == node) {
		return;
	}

	/* right */
	if (node->right && node->right->key <= than) {
		_check_tree_children_dec_lowereuqal_recursively(tree, node->right, than, callback, checkArg);
	}

	/* self */
	if (FALSE == _BITS_ALL_SET(tree->status, RbStatus_AbortCheck))
	{
		if (node->key <= than) {
			_check_invoke(tree, RbCheck_DecLowerEqual, than, node, callback, checkArg);
		}
	}

	/* left */
	_check_tree_children_dec_lowereuqal_recursively(tree, node->left, than, callback, checkArg);

}


/* --------------------_check_tree_children_dec_greater_recursively----------------------- */
static void _check_tree_children_dec_greater_recursively(struct UtilRbTree *tree, struct RbTreeNode *node, RbKey_t than, check_func callback, void *checkArg)
{
	if (_BITS_ALL_SET(tree->status, RbStatus_AbortCheck)) {
		return;
	}
	if (NULL == node) {
		return;
	}

	/* right */
	_check_tree_children_dec_greater_recursively(tree, node->right, than, callback, checkArg);

	/* self */
	if (FALSE == _BITS_ALL_SET(tree->status, RbStatus_AbortCheck))
	{
		if (node->key > than) {
			_check_invoke(tree, RbCheck_DecGreater, than, node, callback, checkArg);
		}
		else {
			return;
		}
	}

	/* left */
	_check_tree_children_dec_greater_recursively(tree, node->left, than, callback, checkArg);

}


/* --------------------_check_tree_children_dec_greaterequal_recursively----------------------- */
static void _check_tree_children_dec_greaterequal_recursively(struct UtilRbTree *tree, struct RbTreeNode *node, RbKey_t than, check_func callback, void *checkArg)
{
	if (_BITS_ALL_SET(tree->status, RbStatus_AbortCheck)) {
		return;
	}
	if (NULL == node) {
		return;
	}

	/* right */
	_check_tree_children_dec_greaterequal_recursively(tree, node->right, than, callback, checkArg);

	/* self */
	if (FALSE == _BITS_ALL_SET(tree->status, RbStatus_AbortCheck))
	{
		if (node->key >= than) {
			_check_invoke(tree, RbCheck_DecGreaterEuqal, than, node, callback, checkArg);
		}
		else {
			return;
		}
	}

	/* left */
	_check_tree_children_dec_greaterequal_recursively(tree, node->left, than, callback, checkArg);

}


/* --------------------_check_tree_children_inc_greater_recursively----------------------- */
static void _check_tree_children_inc_greater_recursively(struct UtilRbTree *tree, struct RbTreeNode *node, RbKey_t than, check_func callback, void *checkArg)
{
	if (_BITS_ALL_SET(tree->status, RbStatus_AbortCheck)) {
		return;
	}
	if (NULL == node) {
		return;
	}

	/* left */
	if (node->left && node->left->key > than) {
		_check_tree_children_inc_greater_recursively(tree, node->left, than, callback, checkArg);
	}

	/* self */
	if (FALSE == _BITS_ALL_SET(tree->status, RbStatus_AbortCheck)) {
		if (node->key > than) {
			_check_invoke(tree, RbCheck_IncGreater, than, node, callback, checkArg);
		}
	}

	/* right */
	_check_tree_children_inc_greater_recursively(tree, node->right, than, callback, checkArg);

}


/* --------------------_check_tree_children_inc_greaterequal_recursively----------------------- */
static void _check_tree_children_inc_greaterequal_recursively(struct UtilRbTree *tree, struct RbTreeNode *node, RbKey_t than, check_func callback, void *checkArg)
{
	if (_BITS_ALL_SET(tree->status, RbStatus_AbortCheck)) {
		return;
	}
	if (NULL == node) {
		return;
	}

	/* left */
	if (node->left && node->left->key >= than) {
		_check_tree_children_inc_greaterequal_recursively(tree, node->left, than, callback, checkArg);
	}

	/* self */
	if (FALSE == _BITS_ALL_SET(tree->status, RbStatus_AbortCheck)) {
		if (node->key >= than) {
			_check_invoke(tree, RbCheck_IncGreaterEqual, than, node, callback, checkArg);
		}
	}

	/* right */
	_check_tree_children_inc_greaterequal_recursively(tree, node->right, than, callback, checkArg);

}


/* --------------------_check_tree_children----------------------- */
static int _check_tree_children(struct UtilRbTree *tree, RbCheck_t how, RbKey_t than, check_func callback, void *checkArg)
{
	switch(how)
	{
	case RbCheck_IncAll:
		_check_tree_children_inc_all_recursively(tree, tree->nodes, callback, checkArg);
		break;
	case RbCheck_DecAll:
		_check_tree_children_dec_all_recursively(tree, tree->nodes, callback, checkArg);
		break;

	case RbCheck_IncLower:
		_check_tree_children_inc_lower_recursively(tree, tree->nodes, than, callback, checkArg);
		break;
	case RbCheck_IncLowerEqual:
		_check_tree_children_inc_lowerequal_recursively(tree, tree->nodes, than, callback, checkArg);
		break;
	case RbCheck_DecLower:
		_check_tree_children_dec_lower_recursively(tree, tree->nodes, than, callback, checkArg);
		break;
	case RbCheck_DecLowerEqual:
		_check_tree_children_dec_lowereuqal_recursively(tree, tree->nodes, than, callback, checkArg);
		break;

	case RbCheck_DecGreater:
		_check_tree_children_dec_greater_recursively(tree, tree->nodes, than, callback, checkArg);
		break;
	case RbCheck_DecGreaterEuqal:
		_check_tree_children_dec_greaterequal_recursively(tree, tree->nodes, than, callback, checkArg);
		break;
	case RbCheck_IncGreater:
		_check_tree_children_inc_greater_recursively(tree, tree->nodes, than, callback, checkArg);
		break;
	case RbCheck_IncGreaterEqual:
		_check_tree_children_inc_greaterequal_recursively(tree, tree->nodes, than, callback, checkArg);
		break;

	default:
		_RET_RB_ERR(EINVAL);
		break;
	}

	return 0;
}




#endif


/********/
#define __PUBLIC_FUNCTIONS
#ifdef __PUBLIC_FUNCTIONS

/* --------------------utilRbTree_New----------------------- */
struct UtilRbTree *utilRbTree_New()
{
	struct UtilRbTree *ret = malloc(sizeof(*ret));
	if (ret) {
		ret->status = 0;
		utilRbTree_Init(ret);
	}
	return ret;
}


/* --------------------utilRbTree_Destory----------------------- */
int utilRbTree_Destory(struct UtilRbTree *tree)
{
	if (NULL == tree) {
		_RET_RB_ERR(EINVAL);
	}
	else {
		int ret = 0;
		ret = utilRbTree_Clean(tree);
		if (ret < 0) {
			return ret;
		}

		free(tree);
		return 0;
	}
}


/* --------------------utilRbTree_Init----------------------- */
int utilRbTree_Init(struct UtilRbTree *tree)
{
	if (NULL == tree) {
		_RET_RB_ERR(EINVAL);
	}

	if (0 != tree->status) {
		_RET_RB_ERR(RB_ERR_ALREADY_INIT);
	}

	tree->status = RbStatus_InitOK;
	tree->count  = 0;
	tree->nodes  = NULL;
	_hash_set_default_func(tree);
		
	return 0;
}


/* --------------------utilRbTree_Clean----------------------- */
int utilRbTree_Clean(struct UtilRbTree *tree)
{
	// TODO:
	_RET_RB_ERR(ENOSYS);
}


/* --------------------utilRbTree_SetHashFunc----------------------- */
int utilRbTree_SetHashFunc(struct UtilRbTree *tree, hash_func func)
{
	if (tree && func)
	{
		if (FALSE == _BITS_ANY_SET(tree->status, RbStatus_InitOK)) {
			_RET_RB_ERR(RB_ERR_NOT_INIT);
		}
		else if (tree->count > 0) {
			_RET_RB_ERR(RB_ERR_NOT_EMPTY);
		}
		else {
			tree->hash_func = func;
			return 0;
		}
	}
	else
	{
		_RET_RB_ERR(EINVAL);
	}
}


/* --------------------utilRbTree_GetHashFunc----------------------- */
hash_func utilRbTree_GetHashFunc(const struct UtilRbTree *tree)
{
	if (tree && tree->hash_func) {
		return tree->hash_func;
	} else {
		return NULL;
	}
}


/* --------------------utilRbTree_CheckObjects----------------------- */
int utilRbTree_CheckObjects(struct UtilRbTree *tree, RbCheck_t how, RbKey_t than, check_func callback, void *checkArg)
{
	int ret = 0;

	if (NULL == tree) {
		_RET_RB_ERR(EINVAL);
	}
	if (((int)how < 0) || (how >= RbCheck_IllegalWay)) {
		_RET_RB_ERR(EINVAL);
	}
	if (NULL == callback) {
		_RET_RB_ERR(EINVAL);
	}

	if (FALSE == _BITS_ALL_SET(tree->status, RbStatus_InitOK)) {
		_RET_RB_ERR(RB_ERR_NOT_INIT);
	}
	if (_BITS_ALL_SET(tree->status, RbStatus_IsChecking)) {
		_RET_RB_ERR(RB_ERR_RECURSIVE_CHECK);
	}

	_BITS_SET(tree->status, RbStatus_IsChecking);
	ret = _check_tree_children(tree, how, than, callback, checkArg);
	_BITS_CLR(tree->status, RbStatus_IsChecking | RbStatus_AbortCheck);

	return ret;
}


/* --------------------utilRbTree_AbortCheck----------------------- */
int utilRbTree_AbortCheck(struct UtilRbTree *tree)
{
	if (NULL == tree) {
		_RET_RB_ERR(EINVAL);
	}
	if (FALSE == _BITS_ALL_SET(tree->status, RbStatus_InitOK)) {
		_RET_RB_ERR(RB_ERR_NOT_INIT);
	}
	if (FALSE == _BITS_ALL_SET(tree->status, RbStatus_IsChecking)) {
		_RET_RB_ERR(RB_ERR_NOT_CHECKING);
	}

	_BITS_SET(tree->status, RbStatus_AbortCheck);
	return 0;
}



#endif


/* EOF */

