/*
 * Created 18E28
 *
 * Generic AVL tree implementation based on Julienne Walker's solution
 *
 * see:
 *  www.eternallyconfuzzled.com/tuts/datastructures/jsw_tut_avl.aspx
 *  en.wikipedia.org/wiki/AVL_tree
 *  cs.ecs.baylor.edu/~donahoo/tools/valgrind
 */

#ifndef JW_AVLTREE_H
#define JW_AVLTREE_H

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

/*
 * Do NOT use any conditional/logical operator inside assertion
 * It's extremely EVIL  Like those multi-nary operators && || ?:
 * Separate them  each statement per line
 */
#define __ASSERT(ex)    assert(ex)
#define __MALLOC(sz)    malloc(sz)
/*
 * The free function shouldn't fail upon NULL
 */
#define __FREE(ptr)     free(ptr)

/*
 * The max height shouldn't exceed 64  accord with size_t
 *  it may exceed 63(zero-based) due to avltree isn't perfect BST (duh)
 */
struct avl_node {
    struct avl_node *link[2];
    int height;             /* Balance factor */
    void *data;
};

typedef ptrdiff_t (*cmp_func_t)(void *, void *);
typedef void (*free_func_t)(void *);
typedef void (*iter_func_t)(void *);
typedef void (*print_func_t)(void *);

struct avl_root {
    struct avl_node *root;
    size_t size;
    cmp_func_t cmp;         /* Set once  read-only */
    free_func_t free;       /* ditto */
};

struct avl_root *avl_alloc(cmp_func_t, free_func_t);
void avl_free(struct avl_root **);
void avl_clear(struct avl_root *);
size_t avl_size(const struct avl_root *);
int avl_find(const struct avl_root *, void *);
int avl_insert(struct avl_root *, void *);
int avl_remove(struct avl_root *, void *);
void avl_foreach(const struct avl_root *, iter_func_t);
/* Those two no operation if DEBUG not defined */
void avl_assert(const struct avl_root *);
void avl_print(const struct avl_root *, print_func_t);

#endif /* JW_AVLTREE_H */
