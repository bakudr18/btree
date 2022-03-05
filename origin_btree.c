#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

typedef struct tnode *tree;                   
typedef struct tnode {
    int data;   
    struct tnode *left;
    struct tnode *right;
} tnode_t;

tnode_t *new_node(int d)
{
    tnode_t *node = malloc(sizeof(tnode_t));
    if(!node)
        return NULL;
    node->data = d;
    node->left = node->right = NULL;
}

void insert_data(tree *t, int d)
{
    if(!*t){
        *t = new_node(d);
        return;
    }
    tnode_t **p = t;
    while(*p != NULL){
        if(d < (*p)->data)
            p = &(*p)->left;
        else
            p = &(*p)->right;
    }
    *p = new_node(d);
}

void remove_data(tree *t, int d)
{
    tnode_t **p = t;
    while (*p != NULL && (*p)->data != d) {
        if (d < (*p)->data)
            p = &(*p)->left;
        else
            p = &(*p)->right;
    }
    tnode_t *q = *p;
    if (!q)
        return;

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

bool isValid(tnode_t *root, tnode_t *lbound, tnode_t *rbound)
{
    if(!root)
         return true;
    if(lbound && lbound->data >= root->data)
        return false;
    if(rbound && rbound->data <= root->data)
        return false;
    return isValid(root->left, lbound, root) && isValid(root->right, root, rbound);
}
    
bool isValidBST(tree *t) {
    tnode_t *root = *t;
    return isValid(root, NULL, NULL);    
}

void inorder_rec(tnode_t *root)
{
    if(!root)
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

int main(){
    tree t = NULL;
    for(int i = 0; i < 10; i++){
        insert_data(&t, i);
    }
    remove_data(&t, 3);
    remove_data(&t, 9);
    remove_data(&t, 6);
    
    printf("%d\n", isValidBST(&t));
    inorder_taversal(&t);
    return 0;
}
