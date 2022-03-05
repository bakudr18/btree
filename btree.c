#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct tnode *tree;
typedef struct tnode {
    int data;
    struct tnode *left;
    struct tnode *right;
} tnode_t;

tnode_t *new_node(int d)
{
    tnode_t *node = malloc(sizeof(tnode_t));
    if (!node)
        return NULL;
    node->data = d;
    node->left = NULL;
    node->right = NULL;
    return node;
}

void insert_data(tree *t, int d)
{
    if (!*t) {
        *t = new_node(d);
        return;
    }
    tnode_t **p = t;
    while (*p != NULL) {
        if (d < (*p)->data)
            p = &(*p)->left;
        else
            p = &(*p)->right;
    }
    *p = new_node(d);
}

void remove_data_origin(tree *t, int d)
{
    tnode_t **p = t;
    // search node with d data
    while (*p != NULL && (*p)->data != d) {
        if (d < (*p)->data)
            p = &(*p)->left;
        else
            p = &(*p)->right;
    }
    tnode_t *q = *p;

    // d not found
    if (!q)
        return;

    // rebuild btree
    if (!q->left)
        *p = q->right;
    else if (!q->right)
        *p = q->left;
    else {
        tnode_t **r = &q->right;
        while ((*r)->left)
            r = &(*r)->left;
        q->data = (*r)->data;
        q = *r;
        *r = q->right;
    }
    free(q);
}

void remove_data_branchless(tree *t, int d)
{
    tnode_t **p = t;
    // search node with d data
    while (*p != NULL && (*p)->data != d) {
        p = (d < (*p)->data) ? &(*p)->left : &(*p)->right;
    }
    tnode_t *q = *p;

    // d not found
    if (!q)
        return;

    // rebuild btree
    if (!q->left || !q->right)
        *p = (tnode_t *) ((intptr_t) q->left | (intptr_t) q->right);
    else {
        tnode_t **r = &q->right;
        while ((*r)->left)
            r = &(*r)->left;
        q->data = (*r)->data;
        q = *r;
        *r = q->right;
    }
    free(q);
}

bool isValid(tnode_t *root, tnode_t *lbound, tnode_t *rbound)
{
    if (!root)
        return true;
    if (lbound && lbound->data > root->data)
        return false;
    if (rbound && rbound->data < root->data)
        return false;
    return isValid(root->left, lbound, root) &&
           isValid(root->right, root, rbound);
}

bool isValidBST(tree *t)
{
    tnode_t *root = *t;
    return isValid(root, NULL, NULL);
}

void inorder_rec(tnode_t *root)
{
    if (!root)
        return;
    inorder_rec(root->left);
    printf("%d ", root->data);
    inorder_rec(root->right);
}

void inorder_taversal(tree *t)
{
    tnode_t *root = *t;
    inorder_rec(root);
    printf("\n");
}

void (*remove_data[2])(tree *, int) = {&remove_data_origin,
                                       &remove_data_branchless};

int main(int argc, char **argv)
{
    tree t = NULL;
    srand(0);
    int method = 0;
    if (argc == 2)
        method = argv[1][0] - '0';

    for (int i = 0; i < 10000; i++) {
        insert_data(&t, i);
    }
    for (int i = 0; i < 10000; i++) {
        insert_data(&t, 10000 - i);
    }
    for (int i = 0; i < 10000; i++) {
        insert_data(&t, rand() % 5000);
        insert_data(&t, 10000 - rand() % 5000);
    }
    for (int i = 0; i < 20000; i++) {
        remove_data[method](&t, rand() % 10000);
    }

    // inorder_taversal(&t);
    return !isValidBST(&t);
}
