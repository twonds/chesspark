#include <stdlib.h>
#include <string.h>

#include "log.h"

#include "namedtree.h"
#include "leak.h"

void DumpTree(struct namedtree_s **tree);

struct namedtree_s **NamedTree_AddNode(struct namedtree_s **tree, struct namedtree_s *node,  int (*sortfunc)(void *, void *))
{
	if (!*tree)
	{
		*tree = node;

		return tree;
	}

	if ((sortfunc && sortfunc(node->data, (*tree)->data)) /* || (!sortfunc && (*tree)->name && node->name && stricmp((*tree)->name, node->name) > 0)*/)
	{
		return NamedTree_AddNode(&((*tree)->lchild), node, sortfunc);
	}	

	return NamedTree_AddNode(&((*tree)->rchild), node, sortfunc);
	
}

struct namedtree_s **NamedTree_Add(struct namedtree_s **tree, char *name, void *data, void (*destroydata)(void *), int (*sortfunc)(void *, void *))
{
	/*Log_Write(0, "NamedTree_Add %s %d to tree %d (%d)\n", name, data, tree, *tree);*/
	/*DumpTree(tree);*/

	struct namedtree_s *node;

	node = malloc(sizeof(*node));
	memset(node, 0, sizeof(*node));

	if (name)
	{
		node->name = strdup(name);
	}
	node->data = data;
	node->destroydata = destroydata;

	return NamedTree_AddNode(tree, node, sortfunc);
}

struct namedtree_s *NamedTree_UnlinkNode(struct namedtree_s **tree)
{
	struct namedtree_s *old;

	/*Log_Write(0, "NamedTree_Unlink %d (%d)\n", tree, *tree);*/
	
	old = *tree;

	if (!old->rchild)
	{
		*tree = old->lchild;
	}
	else if (!old->lchild)
	{
		*tree = old->rchild;
	}
	else
	{
		struct namedtree_s **lefthigh = &(old->lchild);
		struct namedtree_s *swap, *swap2;

		while ((*lefthigh)->rchild)
		{
			lefthigh = &((*lefthigh)->rchild);
		}

		swap = (*lefthigh)->lchild;
		swap2 = *lefthigh;
		if (*lefthigh != (*tree)->lchild)
		{
			(*lefthigh)->lchild = (*tree)->lchild;
		}
		(*lefthigh)->rchild = (*tree)->rchild;
		*lefthigh = swap;
		*tree = swap2;
	}

	old->lchild = NULL;
	old->rchild = NULL;

	return old;
}

void NamedTree_Unlink(struct namedtree_s **tree)
{
	struct namedtree_s *old = NamedTree_UnlinkNode(tree);

	free(old->name);
	free(old);
}

void NamedTree_ReinsertNode(struct namedtree_s **tree, struct namedtree_s **node, int (*sortfunc)(void *, void *))
{
	struct namedtree_s *newnode = *node;

	NamedTree_UnlinkNode(node);
	NamedTree_AddNode(tree, newnode, sortfunc);
}


void NamedTree_MoveTree(struct namedtree_s **src, struct namedtree_s **dst, int (*sortfunc)(void *, void *))
{
	struct namedtree_s *node;
	if (!*src)
	{
		return;
	}

	NamedTree_MoveTree(&((*src)->lchild), dst, sortfunc);
	NamedTree_MoveTree(&((*src)->rchild), dst, sortfunc);

	node = NamedTree_UnlinkNode(src);
	NamedTree_AddNode(dst, node, sortfunc);
}

void NamedTree_Resort(struct namedtree_s **tree, int (*sortfunc)(void *, void *))
{
	struct namedtree_s *newtree = NULL;

	NamedTree_MoveTree(tree, &newtree, sortfunc);

	*tree = newtree;
}

int NamedTree_CountNodes(struct namedtree_s **tree)
{
	if (!tree || !*tree)
	{
		return 0;
	}

	return NamedTree_CountNodes(&((*tree)->lchild)) + NamedTree_CountNodes(&((*tree)->rchild)) + 1;
}

void DumpTree(struct namedtree_s **tree)
{
	if (*tree)
	{
		Log_Write(0, "Node %d (%s), data %d, children %d and %d\n", *tree, (*tree)->name, (*tree)->data, (*tree)->lchild, (*tree)->rchild);
		DumpTree(&((*tree)->lchild));
		DumpTree(&((*tree)->rchild));
	}
	else
	{
		Log_Write(0, "leaf.\n");
	}
}
	

void NamedTree_Remove(struct namedtree_s **tree)
{
	void (*destroydata)(void *);
	void *olddata;

	/*Log_Write(0, "NamedTree_Remove %d (%d)\n", tree, *tree);*/

	destroydata = (*tree)->destroydata;
	olddata = (*tree)->data;

	if (destroydata)
	{
		destroydata(olddata);
	}

	NamedTree_Unlink(tree);

}

void NamedTree_RemoveFirst(struct namedtree_s **tree)
{
	if (!tree || !*tree)
	{
		return;
	}

	while ((*tree)->lchild)
	{
		tree = &((*tree)->lchild);
	}

	NamedTree_Remove(tree);
}

struct namedtree_s **NamedTree_GetByNameNoSortFunc(struct namedtree_s **tree, char *name)
{
	if (*tree)
	{
		int comp = 1;

		if (!name && !(*tree)->name)
		{
		    comp = 0;
		}
		else if (!name && (*tree)->name)
		{
		    comp = -1;
		}
		else if (name && !(*tree)->name)
		{
		    comp = 1;
		}
		else if (name && (*tree)->name)
		{
		    comp = stricmp(name, (*tree)->name);
		}

		if (comp == 0)
		{
			return tree;
		}
		else if (comp < 0)
		{
		       return NamedTree_GetByNameNoSortFunc(&((*tree)->lchild), name);
		}
		else
		{
		       return NamedTree_GetByNameNoSortFunc(&((*tree)->rchild), name);
		}
	}
	return NULL;
}

struct namedtree_s **NamedTree_GetByName(struct namedtree_s **tree, char *name)
{
	/*Log_Write(0, "NamedTree_GetByName %d (%d) %s\n", tree, *tree, name);*/
	
	if (*tree)
	{
		struct namedtree_s **result;

		result = NamedTree_GetByName(&((*tree)->lchild), name);

		if (result)
		{
			return result;
		}

		if ((name && (*tree)->name && stricmp(name, (*tree)->name) == 0)
			|| !name && !(*tree)->name)
		{
			return tree;
		}
		
		result = NamedTree_GetByName(&((*tree)->rchild), name);

		return result;
	}

	return NULL;
}

void NamedTree_RemoveByName(struct namedtree_s **tree, char *name)
{
	struct namedtree_s **ppremove;

	/*Log_Write(0, "NamedTree_RemoveByName %d (%d) %s\n", tree, *tree, name);*/

	/*Log_Write(0, "RemoveByName tree before:\n");
	DumpTree(tree);*/

	while(ppremove = NamedTree_GetByName(tree, name))
	{
		NamedTree_Remove(ppremove);
	}

	/*Log_Write(0, "RemoveByName tree after:\n");
	DumpTree(tree);*/
}

void NamedTree_Destroy(struct namedtree_s **tree)
{
	/*Log_Write(0, "NamedTree_Destroy %d (%d)\n", tree, *tree);*/

	if (!*tree)
	{
		return;
	}

	NamedTree_Destroy(&((*tree)->lchild));
	NamedTree_Destroy(&((*tree)->rchild));

	NamedTree_Remove(tree);
}

void NamedTree_Destroy2(struct namedtree_s *tree)
{
	NamedTree_Destroy(&tree);
}

void NamedTree_InOrder(struct namedtree_s **tree, void (*func)(void *))
{
	/*Log_Write(0, "NamedTree_InOrder %d (%d) func %d\n", tree, *tree, func);*/

	if (!*tree)
	{
		return;
	}

	NamedTree_InOrder(&((*tree)->lchild), func);
	/*Log_Write(0, "Inorder %s\n", (*tree)->name);*/
	func((*tree)->data);
	NamedTree_InOrder(&((*tree)->rchild), func);
}

void *NamedTree_InOrder2(struct namedtree_s **tree, void *(*func)(struct namedtree_s **, void *), void *userdata)
{
	void *result;

	/*Log_Write(0, "NamedTree_InOrder2 %d (%d) func %d userdata %d\n", tree, *tree, func, userdata);*/

	if (!*tree)
	{
		return NULL;
	}

	result = NamedTree_InOrder2(&((*tree)->lchild), func, userdata);

	if (result)
	{
		return result;
	}

	/*Log_Write(0, "Inorder2 %s\n", (*tree)->name);*/
	result = func(tree, userdata);

	if (result)
	{
		return result;
	}

	return NamedTree_InOrder2(&((*tree)->rchild), func, userdata);
}
