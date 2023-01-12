#ifndef AVL_TREE_H
#define AVL_TREE_H

#include <stdlib.h>
#include <string.h>

struct Node{
  int height;
  struct Node *left, *right;
};

typedef struct Node Node;

typedef struct{
  Node *root;
  size_t msize;
  int (*cmp)(const void *,const void *);
}Tree;

typedef enum{
  TREE_INORDER,
  TREE_PREORDER,
  TREE_POSTORDER
}Tree_Order;

Tree *tree_init(size_t msize, int (*cmp)(const void *,const void *));
void tree_traverse(Tree *tree, void (*f)(const void *), Tree_Order order);
void tree_insert(Tree *tree, const void *data);
Node *tree_min(Tree *tree);
Node *tree_max(Tree *tree);
Node *tree_contains(Tree *tree, const void *data);
void tree_delete(Tree *tree, const void *data);
void tree_free(Tree *tree);

#ifdef AVL_TREE_IMPLEMENTATION

#define AVL_TREE_INIT_SIZE 16

#define NODE_VAL(node, type) (*( type *) (((char *) node) + sizeof(Node)))

Tree *tree_init(size_t dsize, int (*cmp)(const void *,const void *)) {
  Tree *tree = (Tree *) malloc(sizeof(Tree));
  if(!tree) {
    fprintf(stderr, "ERROR: Can not allocate enough memory: tree_init\n");
    exit(1);
  }

  tree->msize  = dsize;
  tree->cmp    = cmp;
  tree->root   = NULL;

  return tree;
}

int node_height(Node *node) {
  if(!node) return 0;
  return node->height;
}

int max(int a, int b) {
  return a > b ? a : b;
}

void node_rotate_right(Node **node) {
  if(node==NULL || (*node)==NULL || (*node)->left==NULL) {
    fprintf(stderr, "ERROR: Nullpointer-Exception: tree_rotate_right\n");
    exit(1);
  }
  Node *left = (*node)->left;
  Node *left_right = left->right;

  left->right = *node;
  (*node)->left = left_right;

  (*node)->height = 1 + max(node_height((*node)->left),
			    node_height((*node)->right));
  left->height    = 1 + max(node_height(left->left), node_height(left->right));

  *node = left;
}

void node_rotate_left(Node **node) {
  if(node==NULL || (*node)==NULL || (*node)->right==NULL) {
    fprintf(stderr, "ERROR: Nullpointer-Exception: tree_rotate_left\n");
    exit(1);
  }
  Node *right = (*node)->right;
  Node *right_left = right->left;

  right->left = *node;
  (*node)->right = right_left;

  (*node)->height = 1 + max(node_height((*node)->left),
			    node_height((*node)->right));
  right->height = 1 + max(node_height(right->left),
			  node_height(right->right));

  *node = right;
}

void node_insert(Node **node, const void *data, Tree *tree) {
  if(!(*node)) {
      Node *tmp = (Node *) malloc(sizeof(Node) + tree->msize);
      if(!tmp) {
	fprintf(stderr, "ERROR: Can not allocate enough memory: node_insert\n");
	exit(1);
      }
      memset(tmp, 0, sizeof(Node));
      tmp->height = 1;
      memcpy((char *) tmp + sizeof(Node), data, tree->msize);
      *node = tmp;
    return;
  }

#define NODE_COMPARE(node, data) (tree->cmp(((char *) node) + sizeof(Node), data))
  //data is greater than val=> -1
  //data is smaller than val=>  1
  int d = NODE_COMPARE((*node), data);

  if(d < 0) node_insert(&(*node)->right, data, tree);
  else if(d > 0) node_insert(&(*node)->left, data, tree);
  else return;
  
  int hr = node_height((*node)->right);
  int hl = node_height((*node)->left);

  (*node)->height = 1 + (hl > hr ? hl : hr);

  int diff = hl - hr;
  if(diff > 1 && NODE_COMPARE((*node)->left, data) > 0) {
    node_rotate_right(node);
  }
  else if (diff < -1 && NODE_COMPARE((*node)->right, data) < 0) {
    node_rotate_left(node);
  }
  else if (diff > 1 && NODE_COMPARE((*node)->left, data) < 0) {
    node_rotate_left(&(*node)->left);
    node_rotate_right(node);
  }
  else if (diff < -1 && NODE_COMPARE((*node)->right, data) > 0) {
    node_rotate_right(&(*node)->right);
    node_rotate_left(node);    
  }
}

int node_diff(Node *node) {
  if(!node) return 0;
  int diff = node_height(node->left) - node_height(node->right);
  return diff;
}

void node_free(Node *node) {
  if(!node) return;
  node_free(node->left);
  node_free(node->right);
}

void node_delete(Node **node, const void *data, Tree *tree) {
  if(!node || !(*node)) {
    return;
  }

  int d = NODE_COMPARE((*node), data);
  if(d < 0) {
    node_delete(&(*node)->right, data, tree);
  }
  else if(0 < d) {
    node_delete(&(*node)->left, data, tree);
  }
  else {
    Node *old = *node;
    Node *tmp = NULL;
    if(old->left) {
      Node *before = old->left;
      tmp = old->left;
      int f = 1;
      while(tmp->right) {
	tmp = tmp->right;
	if(f) f = 0;
	else before = before->right;
      }
      if(tmp!=old->left) tmp->left = old->left;
      tmp->right = old->right;
      if(tmp!=before) before->right = NULL;      
    }
    else if(old->right) {
      tmp = old->right;
    }
    *node = tmp;
    node_free(old);
  }

  if(!node || !(*node)) return;

  int hr = node_height((*node)->right);
  int hl = node_height((*node)->left);

  (*node)->height = 1 + (hl > hr ? hl : hr);

  int diff = hl - hr;

  if (diff > 1 && node_diff((*node)->left) >= 0) {
    node_rotate_right(node);
  }
  else if (diff > 1 && node_diff((*node)->left) < 0) {
    node_rotate_left(&(*node)->left);
    node_rotate_right(node);
  }
  else if (diff < -1 && node_diff((*node)->right) <= 0) {
    node_rotate_left(node);
  }
  else if (diff < -1 && node_diff((*node)->right) > 0) {
    node_rotate_right(&(*node)->right);
    node_rotate_left(node);
  }  
}

void node_traverse(Node *node, void (*f)(const void *), Tree_Order order) {
  if(!node) return;
  switch(order) {
  case TREE_INORDER:
    if(node->left) node_traverse(node->left, f, order);
    f(node);
    if(node->right) node_traverse(node->right, f, order);
    break;
  case TREE_PREORDER:
    f(node);
    if(node->left) node_traverse(node->left, f, order);
    if(node->right) node_traverse(node->right, f, order);
    break;
  case TREE_POSTORDER:    
    if(node->left) node_traverse(node->left, f, order);
    if(node->right) node_traverse(node->right, f, order);
    f(node);
    break;
  default:
    break;
  }
}
void tree_traverse(Tree *tree, void (*f)(const void *), Tree_Order order) {
  node_traverse(tree->root, f, order);
}

int tree_height(Tree *tree) {
  return node_height(tree->root);
}

Node *tree_min(Tree *tree) {
  if(!(tree->root)) return NULL;
  Node *tmp = tree->root;
  while(tmp->left) {
    tmp = tmp->left;
  }
  return tmp;
}

Node *tree_max(Tree *tree) {
  if(!(tree->root)) return NULL;
  Node *tmp = tree->root;
  while(tmp->right) {
    tmp = tmp->right;
  }
  return tmp;
}

Node *tree_contains(Tree *tree, const void *data) {
  Node *tmp = tree->root;
  int d;
  while(tmp) {
    d = NODE_COMPARE(tmp, data);
    if(d < 0) tmp=tmp->right;
    else if(0 < d) tmp=tmp->left;
    else if(d==0) return tmp;
  }
  return NULL;
}

void tree_insert(Tree *tree, const void *data) {
  node_insert(&(tree->root), data, tree);  
}

void tree_delete(Tree *tree, const void *data) {
  node_delete(&(tree->root), data, tree);
}

void tree_free(Tree *tree) {
  node_free(tree->root);
  if(tree) free(tree);
}

#endif

#endif //AVL_TREE_H
