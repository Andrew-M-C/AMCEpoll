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
#if 1

#include "utilRbTree.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define _RB_TREE_DEBUG_FLAG


#endif


/********/
#define __DATA_TYPES
#if 1

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


typedef enum {
	Color_Black = 0,
	Color_Red,
} RbColor_t;


struct RbTreeNode {
	RbKey_t    key;
	RbColor_t  color;
	void      *obj;
	struct RbTreeNode *parent;
	struct RbTreeNode *left;
	struct RbTreeNode *right;
};


#define _BITS_ANY_SET(val, bits)		(0 != ((val) & (bits)))
#define _BITS_ALL_SET(val, bits)		((bits) == ((val) & (bits)))
#define _BITS_SET(val, bits)			((val) |= (bits))
#define _BITS_CLR(val, bits)			((val) &= ~(bits))

#define _SET_BLACK(node)	do{(node)->color = Color_Black;}while(0)
#define _SET_RED(node)		do{(node)->color = Color_Red;}while(0)


#endif

/********/
#define __FUNCTION_DECLARATIONS
#if 1
static void _rb_check_delete_node_case_1(struct UtilRbTree *tree, struct RbTreeNode *node);
static void _rb_check_delete_node_case_2(struct UtilRbTree *tree, struct RbTreeNode *node);
static void _rb_check_delete_node_case_3(struct UtilRbTree *tree, struct RbTreeNode *node);
static void _rb_check_delete_node_case_4(struct UtilRbTree *tree, struct RbTreeNode *node);
static void _rb_check_delete_node_case_5(struct UtilRbTree *tree, struct RbTreeNode *node);
static void _rb_check_delete_node_case_6(struct UtilRbTree *tree, struct RbTreeNode *node);


#endif


/********/
#define __GENERAL_INTERNAL_FUNCTIONS
#if 1

/* --------------------_rb_err----------------------- */
static int _rb_err(int err)
{
	if (0 == err) {
		errno = 0;
		return 0;
	}

	if (err < 0) {
		err = -err;
	}

	if (1 == err) {
		return 0 - RB_ERR_UNKNOWN;
	}
	else if (err >= RB_ERR_UNKNOWN) {
		errno = EPERM;
		return -err;
	}
	else {
		errno = -err;
		return err;
	}
}



#endif


/********/
#define __NODE_SEARCH_AND_ALLOCATION
#if 1

/* --------------------_rb_read_node_status----------------------- */
static void _node_properties(const struct RbTreeNode *node, BOOL *hasChild, BOOL *hasTwoChild, BOOL *hasLeftChild, BOOL *hasRightChild)
{
	if (node->left)
	{
		if (node->right) {
			*hasChild      = TRUE;
			*hasTwoChild   = TRUE;
			*hasLeftChild  = TRUE;
			*hasRightChild = TRUE;
		}
		else {
			*hasChild      = TRUE;
			*hasTwoChild   = FALSE;
			*hasLeftChild  = TRUE;
			*hasRightChild = FALSE;
		}
	}
	else
	{
		if (node->right) {
			*hasChild      = TRUE;
			*hasTwoChild   = FALSE;
			*hasLeftChild  = FALSE;
			*hasRightChild = TRUE;
		}
		else {
			*hasChild      = FALSE;
			*hasTwoChild   = FALSE;
			*hasLeftChild  = FALSE;
			*hasRightChild = FALSE;
		}
	}
}


/* --------------------_node_search----------------------- */
static struct RbTreeNode *_node_search(const struct UtilRbTree *tree, RbKey_t key)
{
	struct RbTreeNode *ret = NULL;
	struct RbTreeNode *current = tree->nodes;

	while ((NULL == ret) && current)
	{
		if (key < current->key) {
			current = current->left;
		}
		else if (key > current->key)  {
			current = current->right;
		}
		else {
			ret = current;
		}
	}

	return ret;
}


/* --------------------_node_new----------------------- */
static struct RbTreeNode *_node_new(RbKey_t key, struct RbTreeNode *parent, RbColor_t color, void *obj)
{
	struct RbTreeNode *ret = malloc(sizeof(*ret));
	if (ret) {
		ret->key   = key;
		ret->color = color;
		ret->obj   = obj;
		ret->parent= parent;
		ret->left  = NULL;
		ret->right = NULL;
	}
	return ret;
}


/* --------------------_node_new----------------------- */
static void _node_free(struct RbTreeNode *node)
{
	free(node);
	return;
}


/* --------------------_node_is_black----------------------- */
static inline BOOL _node_is_black(struct RbTreeNode *node)
{
	return (Color_Black == node->color);
}


/* --------------------_node_is_red----------------------- */
static inline BOOL _node_is_red(struct RbTreeNode *node)
{
	return (Color_Black != node->color);
}


/* --------------------_node_is_root----------------------- */
static inline BOOL _node_is_root(struct RbTreeNode *node)
{
	return (NULL == node->parent);
}


/* --------------------_node_brother----------------------- */
static struct RbTreeNode *_node_brother(struct RbTreeNode *node)
{
	if (_node_is_root(node)) {
		return NULL;
	}
	else {
		if (node == node->parent->left) {
			return node->parent->right;
		}
		else {
			return node->parent->left;
		}
	}
}


/* --------------------_node_uncle----------------------- */
static struct RbTreeNode *_node_uncle(struct RbTreeNode *node)
{
	return (node->parent) ? _node_brother(node->parent) : NULL;
}


/* --------------------_node_find_min_leaf----------------------- */
static struct RbTreeNode *_node_find_min_leaf(struct RbTreeNode *node)
{
	struct RbTreeNode *ret = node;
	while(ret->left) {
		ret = ret->left;
	}
	return ret;
}

#endif


/********/
#define __CORE_RB_TREE_OPERATIONS
#if 1

/* --------------------_rb_rotate_left----------------------- */
static void _rb_rotate_left(struct UtilRbTree *tree, struct RbTreeNode *node)
{
	struct RbTreeNode *prevRight = node->right;
	struct RbTreeNode *prevRightLeft = node->right->left;
	//_RB_DB("Rotate left %ld", node->id);
	//_RB_DB("prevRight %ld, prevRightLeft %ld", prevRight ? prevRight->id : -1, prevRightLeft ? prevRightLeft->id : -1);

	if (_node_is_root(node))
	{
		tree->nodes = prevRight;
		
		node->parent = prevRight;
		node->right = prevRightLeft;
		if (prevRightLeft) {
			prevRightLeft->parent = node;
		}
		
		prevRight->left = node;
		prevRight->parent = NULL;
	}
	else
	{
		struct RbTreeNode *parent = node->parent;

		//_rb_dump_node(parent, 5);
		if (node == node->parent->left) {
			parent->left = prevRight;
		}
		else {
			parent->right = prevRight;
		}

		node->parent = prevRight;
		node->right = prevRightLeft;
		if (prevRightLeft) {
			prevRightLeft->parent = node;
		}

		prevRight->left = node;
		prevRight->parent = parent;

		//_rb_dump_node(parent, 5);
	}
	
	return;
}


/* --------------------_rb_rotate_right----------------------- */
static void _rb_rotate_right(struct UtilRbTree *tree, struct RbTreeNode *node)
{
	struct RbTreeNode *prevLeft = node->left;
	struct RbTreeNode *prevLeftRight = node->left->right;
	//_RB_DB("Rotate right %s", node->key);

	if (_node_is_root(node))
	{
		tree->nodes = prevLeft;
		
		node->parent = prevLeft;
		node->left = prevLeftRight;
		if (prevLeftRight) {
			prevLeftRight->parent = node;
		}
		
		prevLeft->right = node;
		prevLeft->parent = NULL;
	}
	else
	{
		struct RbTreeNode *parent = node->parent;
		if (node == node->parent->left) {
			parent->left = prevLeft;
		}
		else {
			parent->right = prevLeft;
		}

		node->parent = prevLeft;
		node->left = prevLeftRight;
		if (prevLeftRight) {
			prevLeftRight->parent = node;
		}

		prevLeft->right = node;
		prevLeft->parent = parent;
	}
	
	return;
}


/* --------------------_rb_check_after_insert_by_rb_rule----------------------- */
static void _rb_check_after_insert_by_rb_rule(struct UtilRbTree *tree, struct RbTreeNode *node)
{
	struct RbTreeNode *uncleNode = _node_uncle(node);

	if (_node_is_root(node))							// root node. This is impossible because it is done previously
	{
		_RB_DB("MARK (%llu) case 1", node->key);
		_SET_BLACK(node);
	}
	else if (_node_is_black(node->parent))			// parent black. Nothing additional needed
	{
		_RB_DB("MARK (%llu) case 2", node->key);
	}
	else if (uncleNode && _node_is_red(uncleNode))	// parent and uncle node are both red
	{
		_RB_DB("MARK (%llu) case 3, uncle: %llu, parent: %llu", node->key, uncleNode->key, node->parent->key);
		_SET_BLACK(node->parent);
		_SET_BLACK(uncleNode);
		_SET_RED(node->parent->parent);
		_rb_check_after_insert_by_rb_rule(tree, node->parent->parent);
	}
	else				// parent red, meanwhile uncle is black or NULL 
	{
		// Step 1
		if ((node == node->parent->right) &&
			(node->parent == node->parent->parent->left))	// node is its parent's left child, AND parent is grandparent's right child. left-rotation needed.
		{
			_RB_DB("MARK (%llu) case 4", node->key);
			_rb_rotate_left(tree, node->parent);
			node = node->left;
			//_rb_dump_node(array->children, 0);
		}
		else if ((node == node->parent->left) &&
				(node->parent == node->parent->parent->right))
		{
			_RB_DB("MARK (%llu) case 4", node->key);
			_rb_rotate_right(tree, node->parent);
			node = node->right;
			//_rb_dump_node(array->children, 0);
		}
		else
		{}

		// Step 2 (Insert Case 5)
		_SET_BLACK(node->parent);
		_SET_RED(node->parent->parent);

		if ((node == node->parent->left) &&
			(node->parent == node->parent->parent->left))
		{
			_RB_DB("MARK (%llu) case 5", node->key);
			_rb_rotate_right(tree, node->parent->parent);
		}
		else
		{
			_RB_DB("MARK (%llu) case 5", node->key);
			_rb_rotate_left(tree, node->parent->parent);
			//_RB_DB("MARK test:");
			//_rb_dump_node(array->children, 0);
		}
	}
	return;
}


/* --------------------_rb_insert_node----------------------- */
static int _rb_insert_node(struct UtilRbTree *tree, RbKey_t key, void *obj)
{
	struct RbTreeNode*node = tree->nodes;
	struct RbTreeNode *newNode = NULL;

	if (NULL == node)	// root
	{
		newNode = _node_new(key, NULL, Color_Black, obj);
		if (NULL == newNode) {
			return _rb_err(errno);
		}

		tree->nodes = newNode;
		tree->count = 1;

		return 0;
	}

	/* first of all, insert a node by binary search tree method */
	do
	{
		if (key < node->key)
		{
			if (NULL == node->left) {		/* found a slot */
				newNode = _node_new(key, node, Color_Red, obj);
				if (NULL == newNode) {
					return _rb_err(errno);
				}
				node->left  = newNode;
				tree->count ++;
			}
			else {
				node = node->left;
			}
		}
		else if (key > node->key)
		{
			if (NULL == node->right) {		/* found a slot */
				newNode = _node_new(key, node, Color_Red, obj);
				if (NULL == newNode) {
					return _rb_err(errno);
				}
				node->right = newNode;
				tree->count ++;
			}
			else {
				node = node->right;
			}
		}
		else	/* conflict found */
		{
			if (obj == node->obj) {
				return 0;
			}
			else {
				return _rb_err(RB_ERR_INSERT_CONFLICT);
			}
		}
	}
	while(NULL == newNode);

	/* OK */
	_rb_check_after_insert_by_rb_rule(tree, newNode);
	return 0;
}


/* --------------------_rb_check_delete_node_case_6----------------------- */
static void _rb_check_delete_node_case_6(struct UtilRbTree *tree, struct RbTreeNode *node)
{
	struct RbTreeNode *sibling = _node_brother(node);

	sibling->color = node->parent->color;
	_SET_BLACK(node->parent);

	if (node == node->parent->left) {
		_SET_BLACK(sibling->right);
		_rb_rotate_left(tree, node->parent);
	}
	else {
		_SET_BLACK(sibling->left);
		_rb_rotate_right(tree, node->parent);
	}

	return;
}


/* --------------------_rb_check_delete_node_case_5----------------------- */
static void _rb_check_delete_node_case_5(struct UtilRbTree *tree, struct RbTreeNode *node)
{
	struct RbTreeNode *sibling = _node_brother(node);
	BOOL slbIsBlack;
	BOOL sblLeftIsBlack;
	BOOL sblRightIsBlack;

	if (sibling) {
		slbIsBlack = _node_is_black(sibling);
		sblLeftIsBlack = (sibling->left) ? _node_is_black(sibling->left) : TRUE;
		sblRightIsBlack = (sibling->right) ? _node_is_black(sibling->right) : TRUE;
	}
	else {_RB_DB("DEL: case 5 warning");exit(1);
		slbIsBlack = TRUE;
		sblLeftIsBlack = TRUE;
		sblRightIsBlack= TRUE;
	}

	if (FALSE == slbIsBlack)
	{}
	else if ((node == node->parent->left) &&
		(FALSE == sblLeftIsBlack) &&
		sblRightIsBlack)
	{
		if (sibling) {
			_SET_RED(sibling);
		}
		if (sibling && sibling->left) {
			_SET_BLACK(sibling->left);
		}
		_rb_rotate_right(tree, sibling);
	}
	else if ((node == node->parent->right) &&
			sblLeftIsBlack &&
			(FALSE == sblRightIsBlack))
	{
		if (sibling) {
			_SET_RED(sibling);
		}
		if (sibling && sibling->right) {
			_SET_BLACK(sibling->right);
		}
		_rb_rotate_left(tree, sibling);
	}
	else
	{}

	_rb_check_delete_node_case_6(tree, node);
}


/* --------------------_rb_check_delete_node_case_4----------------------- */
static void _rb_check_delete_node_case_4(struct UtilRbTree *tree, struct RbTreeNode *node)
{
	struct RbTreeNode *sibling = _node_brother(node);
	BOOL sblLeftIsBlack;
	BOOL sblRightIsBlack;
	BOOL sblIsBlack;

	if (NULL == sibling) {
		sblIsBlack = TRUE;
		sblLeftIsBlack = TRUE;
		sblRightIsBlack = TRUE;
	}
	else {
		sblIsBlack = _node_is_black(sibling);
		sblLeftIsBlack = (sibling->left) ? _node_is_black(sibling->left) : TRUE;
		sblRightIsBlack = (sibling->right) ? _node_is_black(sibling->right) : TRUE;
	}

	if (_node_is_red(node->parent)
		&& sblIsBlack
		&& sblLeftIsBlack
		&& sblRightIsBlack)
	{
		_RB_DB("DEL: case 4");
		if (sibling) {
			_SET_RED(sibling);
		}
		_SET_BLACK(node->parent);
	}
	else
	{
		_rb_check_delete_node_case_5(tree, node);
	}
}


/* --------------------_rb_check_delete_node_case_3----------------------- */
static void _rb_check_delete_node_case_3(struct UtilRbTree *tree, struct RbTreeNode *node)
{
	struct RbTreeNode *sibling = _node_brother(node);
	BOOL sblLeftIsBlack;
	BOOL sblRightIsBlack;
	BOOL sblIsBlack;

	if (NULL == sibling) {
		sblIsBlack = TRUE;
		sblLeftIsBlack = TRUE;
		sblRightIsBlack = TRUE;
	}
	else {
		sblIsBlack = _node_is_black(sibling);
		sblLeftIsBlack = (sibling->left) ? _node_is_black(sibling->left) : TRUE;
		sblRightIsBlack = (sibling->right) ? _node_is_black(sibling->right) : TRUE;
	}

	if (_node_is_black(node->parent)
		&& sblLeftIsBlack
		&& sblRightIsBlack
		&& sblIsBlack)
	{
		_RB_DB("DEL: case 3");
		if (sibling) {
			_SET_RED(sibling);
		}
		_rb_check_delete_node_case_1(tree, node->parent);
	}
	else
	{
		_rb_check_delete_node_case_4(tree, node);
	}
}


/* --------------------_rb_check_delete_node_case_2----------------------- */
static void _rb_check_delete_node_case_2(struct UtilRbTree *tree, struct RbTreeNode *node)
{
	struct RbTreeNode *sibling = _node_brother(node);

	if (sibling && _node_is_red(sibling))
	{
		_RB_DB("DEL: case 2");
		_SET_RED(node->parent);
		_SET_BLACK(sibling);

		if (node == node->parent->left) {
			_rb_rotate_left(tree, node->parent);
		}
		else {
			_rb_rotate_right(tree, node->parent);
		}
	}
	else
	{}

	return _rb_check_delete_node_case_3(tree, node);
}


/* --------------------_rb_check_delete_node_case_1----------------------- */
static void _rb_check_delete_node_case_1(struct UtilRbTree *tree, struct RbTreeNode *node)
{
	/* let's operate with diferent cases */
	if (_node_is_root(node)) {
		_RB_DB("DEL: case 1");
		return;
	}
	else {		// Right child version
		_rb_check_delete_node_case_2(tree, node);
	}
}



/* --------------------_rb_delete_node_and_reconnect_with----------------------- */
static void _rb_delete_node_and_reconnect_with(struct UtilRbTree *tree, struct RbTreeNode *nodeToDel, struct RbTreeNode *nodeToReplace)
{
	if (_node_is_root(nodeToDel))
	{
		tree->count --;
		tree->nodes = nodeToReplace;
		nodeToReplace->parent = NULL;
	}
	else
	{
		tree->count --;
		nodeToReplace->parent = nodeToDel->parent;

		if (nodeToDel == nodeToDel->parent->left) {
			nodeToDel->parent->left = nodeToReplace;
		}
		else {
			nodeToDel->parent->right = nodeToReplace;
		}
	}

	return;
}


/* --------------------_rb_delete_node----------------------- */
static int _rb_delete_node(struct UtilRbTree *tree, struct RbTreeNode *node)
{
	BOOL nodeHasChild, nodeHasTwoChildren, nodeHasLeftChild, nodeHasRightChild;
	_node_properties(node, &nodeHasChild, &nodeHasTwoChildren, &nodeHasLeftChild, &nodeHasRightChild);

	if (FALSE == nodeHasChild)
	{
		_RB_DB("DEL: node %llu is leaf", node->key);
		if (_node_is_root(node))
		{
			tree->nodes = NULL;
			tree->count = 0;
			_node_free(node);
			node = NULL;
		}
		else
		{
			if (node == node->parent->left) {
				_RB_DB("DEL left of %llu (%llu)", node->parent->key, node->key);
				node->parent->left = NULL;
			}
			else {
				_RB_DB("DEL left of %llu (%llu)", node->parent->key, node->key);
				node->parent->right = NULL;
			}

			tree->count --;
			_node_free(node);
			node = NULL;
		}
		return 0;
	}
	else if (nodeHasTwoChildren)
	{
		struct RbTreeNode tmpNode;
		struct RbTreeNode *smallestRightChild = _node_find_min_leaf(node->right);
		_RB_DB("DEL: node %llu has 2 children, replace with %llu", node->key, smallestRightChild->key);

		/* replace with smallest node */
		tmpNode.key = node->key;
		node->key = smallestRightChild->key;
		smallestRightChild->key = tmpNode.key;

		/* start operation with the smallest one */
		return _rb_delete_node(tree, smallestRightChild);
	}
	else
	{
		struct RbTreeNode *child = (node->left) ? node->left : node->right;

		if (_node_is_red(node))		// if node is red, both parent and child nodes are black. We could simply replace it with its child
		{
			_RB_DB("DEL: node %llu red", node->key);
			_rb_delete_node_and_reconnect_with(tree, node, child);
			_node_free(node);
			node = NULL;
			return 0;
		}
		else if (_node_is_red(child))		// if node is black but its child is red. we could repace it with its child and refill the child as black
		{
			_RB_DB("DEL: node %llu black but child red", node->key);
			_rb_delete_node_and_reconnect_with(tree, node, child);
			_SET_BLACK(child);
			_node_free(node);
			node = NULL;
			return 0;
		}
		else				// both node and its child are all black. This is the most complex one.
		{
			/* first of all, we should replace node with child */
			_rb_delete_node_and_reconnect_with(tree, node, child);
			_node_free(node);	/* "node" val is useless */
			node = NULL;
			_rb_check_delete_node_case_1(tree, child);
			return 0;
		}
	}
}


#endif


/********/
#define __CHECK_FUNCTIONS
#if 1

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
		return _rb_err(EINVAL);
		break;
	}

	return 0;
}




#endif


/********/
#define __PUBLIC_FUNCTIONS
#if 1

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
		return _rb_err(EINVAL);
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
		return _rb_err(EINVAL);
	}

	if (0 != tree->status) {
		return _rb_err(RB_ERR_ALREADY_INIT);
	}

	tree->status = RbStatus_InitOK;
	tree->count  = 0;
	tree->nodes  = NULL;
		
	return 0;
}


/* --------------------utilRbTree_Clean----------------------- */
int utilRbTree_Clean(struct UtilRbTree *tree)
{
	// TODO:
	return _rb_err(ENOSYS);
}


/* --------------------utilRbTree_SetObject----------------------- */
int utilRbTree_SetObject(struct UtilRbTree *tree, void *obj, RbKey_t key, void **prevObj)
{
	// TODO:
	return _rb_err(ENOSYS);
}


/* --------------------utilRbTree_GetObject----------------------- */
void *utilRbTree_GetObject(const struct UtilRbTree *tree, RbKey_t key)
{
	struct RbTreeNode *node = NULL;

	if (NULL == tree) {
		errno = EINVAL;
		return NULL;
	}

	node = _node_search(tree, key);
	if (node) {
		return node->obj;
	} else {
		return NULL;
	}
}


/* --------------------utilRbTree_DrainObject----------------------- */
void *utilRbTree_DrainObject(struct UtilRbTree *tree, RbKey_t key)
{
	// TODO:
	errno = ENOSYS;
	return NULL;
}


/* --------------------utilRbTree_CheckObjects----------------------- */
int utilRbTree_CheckObjects(struct UtilRbTree *tree, RbCheck_t how, RbKey_t than, check_func callback, void *checkArg)
{
	int ret = 0;

	if (NULL == tree) {
		return _rb_err(EINVAL);
	}
	if (((int)how < 0) || (how >= RbCheck_IllegalWay)) {
		return _rb_err(EINVAL);
	}
	if (NULL == callback) {
		return _rb_err(EINVAL);
	}

	if (FALSE == _BITS_ALL_SET(tree->status, RbStatus_InitOK)) {
		return _rb_err(RB_ERR_NOT_INIT);
	}
	if (_BITS_ALL_SET(tree->status, RbStatus_IsChecking)) {
		return _rb_err(RB_ERR_RECURSIVE_CHECK);
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
		return _rb_err(EINVAL);
	}
	if (FALSE == _BITS_ALL_SET(tree->status, RbStatus_InitOK)) {
		return _rb_err(RB_ERR_NOT_INIT);
	}
	if (FALSE == _BITS_ALL_SET(tree->status, RbStatus_IsChecking)) {
		return _rb_err(RB_ERR_NOT_CHECKING);
	}

	_BITS_SET(tree->status, RbStatus_AbortCheck);
	return 0;
}


/* --------------------utilRbTree_AbortCheck----------------------- */
const char *utilRbTree_StrError(int error)
{
	static const char *errStr[RB_ERR_BOUNDARY - RB_ERR_UNKNOWN + 1] = {
		"unknown error",
		"tree already initialized",
		"tree not initialized",
		"tree is not empty",
		"tree is not checking",
		"recursive check is forbidden",
		"another object with given key exists",
		
		"illegal tree error"	// SHOULD placed in the end
	};

	if (-1 == error) {
		return strerror(errno);
	}
	else if (error < 0) {
		error = -error;
	}
	else {
		// NOP
	}

	if (error < RB_ERR_UNKNOWN) {
		return strerror(error);
	}
	else if (error >= RB_ERR_BOUNDARY) {
		return errStr[0];
	}
	else {
		return errStr[error - RB_ERR_UNKNOWN];
	}
}



#endif


/* EOF */

