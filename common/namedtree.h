#ifndef __NAMEDTREE_H__
#define __NAMEDTREE_H__

struct namedtree_s
{
	struct namedtree_s *lchild;
	struct namedtree_s *rchild;
	char *name;
	void *data;
	void (*destroydata)(void *);
};

struct namedtree_s **NamedTree_Add(struct namedtree_s **tree, char *name, void *data, void (*destroydata)(void *), int (*sortfunc)(void *, void *));

void NamedTree_Remove(struct namedtree_s **tree);
struct namedtree_s **NamedTree_GetByName(struct namedtree_s **tree, char *name);
struct namedtree_s **NamedTree_GetNextByName(struct namedtree_s **tree, char *name);
void NamedTree_RemoveByName(struct namedtree_s **tree, char *name);
void NamedTree_Unlink(struct namedtree_s **tree);
void NamedTree_Destroy(struct namedtree_s **tree);
void NamedTree_Destroy2(struct namedtree_s *tree);
void NamedTree_InOrder(struct namedtree_s **tree, void (*func)(void *));
void *NamedTree_InOrder2(struct namedtree_s **tree, void *(*func)(struct namedtree_s **, void *), void *userdata);
int NamedTree_CountNodes(struct namedtree_s **tree);
void NamedTree_RemoveFirst(struct namedtree_s **tree);
void NamedTree_ReinsertNode(struct namedtree_s **tree, struct namedtree_s **node, int (*sortfunc)(void *, void *));
void NamedTree_Resort(struct namedtree_s **tree, int (*sortfunc)(void *, void *));

#endif
