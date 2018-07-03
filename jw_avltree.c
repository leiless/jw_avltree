#include <stdio.h>
#include <errno.h>

#include "jw_avltree.h"

#if __GNUC__
#define __UNUSED __attribute__((unused))
#if __STRICT_ANSI__
#define inline      /* */
#endif
#else
#define __UNUSED    /* */
#endif

#define __MAX(a, b) (((a) > (b)) ? (a) : (b))
#define __ABS(a, b) (((a) > (b)) ? ((a) - (b)) : ((b) - (a)))

static inline ptrdiff_t _avl_raw_cmp(void *a, void *b) { return a - b; }
static inline void _avl_raw_free(void *ptr __UNUSED) { /* NOP */ }

/**
 * Allocate and initialize an AVL tree
 * @cmp_f   The data comparator(NULL for raw comparison)
 * @free_f  The free function for data(NULL for no operation)
 */
struct avl_root *avl_alloc(cmp_func_t cmp_f, free_func_t free_f)
{
    struct avl_root *t = __MALLOC(sizeof(*t));
    if (t == NULL) goto out_exit;

    t->root = NULL;
    t->size = 0;
    t->cmp = cmp_f ? cmp_f : _avl_raw_cmp;
    t->free = free_f ? free_f : _avl_raw_free;

out_exit:
    return t;
}

/**
 * Core utility for avltree destruction
 * @param t         the avltree reference
 * @param fully     if free the rbtree fully(e.g. if free the root)
 */
static void _avl_free_core(struct avl_root **t, int fully)
{
    __ASSERT(t != NULL);
    __ASSERT(*t != NULL);

    struct avl_root *tree;
    struct avl_node *iter, *save;
#ifdef DEBUG
    size_t size0, size1;
#endif

    tree = *t;
    iter = tree->root;
    save = NULL;
#ifdef DEBUG
    size0 = tree->size;
    size1 = 0;
#endif

    /*
     * Rotate away the left links into a linked list so that
     *  we can perform iterative destruction of the avltree
     */
    while (iter != NULL) {
        if (iter->link[0] == NULL) {
            save = iter->link[1];
            tree->free(iter->data);
            __FREE(iter);
            tree->size--;
#ifdef DEBUG
            size1++;
#endif
        } else {
            save = iter->link[0];
            iter->link[0] = save->link[1];
            save->link[1] = iter;
        }
        iter = save;
    }

#ifdef DEBUG
    __ASSERT(size0 == size1);
#endif
    __ASSERT(tree->size == 0);

    tree->root = NULL;
    if (fully) {
        __FREE(tree);
        *t = NULL;
    }
}

/**
 * Free the whole avltree
 */
void avl_free(struct avl_root **t)
{
    _avl_free_core(t, 1);
}

/**
 * Clean the avltree
 */
void avl_clear(struct avl_root *t)
{
    struct avl_root **p = &t;
    _avl_free_core(p, 0);
}

/**
 * Retrieve size of the avltree
 */
size_t avl_size(const struct avl_root *t)
{
    __ASSERT(t != NULL);
    if (t->root == NULL)
        __ASSERT(t->size == 0);
    return t->size;
}

/**
 * Check if a specific item in avltree
 * @t       the avl tree
 * @data    the value to be find
 * @return  1 if the value found  0 o.w.
 */
int avl_find(const struct avl_root *t, void *data)
{
    __ASSERT(t != NULL);

    register struct avl_node *it;
    ptrdiff_t dir;

    it = t->root;
    while (it != NULL) {
        if ((dir = t->cmp(it->data, data)) == 0)
            return 1;
        it = it->link[dir < 0];
    }

    return 0;
}

static struct avl_node *_make_node(void *data)
{
    struct avl_node *n = __MALLOC(sizeof(*n));
    if (n == NULL) goto out_exit;

    n->data = data;
    n->height = 0;
    n->link[0] = n->link[1] = NULL;

out_exit:
    return n;
}

#define __HEIGHT(node) ((node) == NULL ? -1 : (node)->height)

static struct avl_node *_avl_rot1(struct avl_node *root, int dir)
{
    struct avl_node *save = root->link[!dir];
    int rlh, rrh, slh;

    /* Rotate */
    root->link[!dir] = save->link[dir];
    save->link[dir] = root;

    /* Update balance factors */
    rlh = __HEIGHT(root->link[0]);
    rrh = __HEIGHT(root->link[1]);
    slh = __HEIGHT(save->link[!dir]);

    root->height = __MAX(rlh, rrh) + 1;
    save->height = __MAX(slh, root->height) + 1;

    return save;
}

static struct avl_node *_avl_rot2(struct avl_node *root, int dir)
{
    root->link[!dir] = _avl_rot1(root->link[!dir], !dir);
    return _avl_rot1(root, dir);
}

/**
 * Insert an item into avltree
 * @t           the avltree
 * @data        the data to be inserted
 * @return      0 if inserted successfully
 *              ENOMEM if oom
 *              EEXIST if item already in tree
 *
 * NOTE: allow to insert/delete/find NULL(aka 0)
 *  which your cmp and free shouldn't fail upon it
 */
int avl_insert(struct avl_root *t, void *data)
{
    __ASSERT(t != NULL);

    if (t->root == NULL) {
        __ASSERT(t->size == 0);
        if ((t->root = _make_node(data)) != NULL) {
            t->size++;
            return 0;
        }
        return ENOMEM;
    }

    if (t->size + 1 == 0) return ENOMEM;    /* Should never happen */

    /* The path depth shouldn't exceed 64 levels  accord with size_t */
    struct avl_node *it, *up[64];
    int upd[64], top = 0;
    int done = 0;
    ptrdiff_t diff;

    it = t->root;

    /* Search for an empty link  save the path */
    while (1) {
        /* Push direction and node onto stack */
        diff = t->cmp(it->data, data);
        if (diff == 0) return EEXIST;

        upd[top] = diff < 0;
        up[top++] = it;

        if (it->link[upd[top - 1]] == NULL)
            break;

        it = it->link[upd[top - 1]];
    }

    /* Insert a new node at the bottom of the tree */
    it->link[upd[top - 1]] = _make_node(data);
    if (it->link[upd[top - 1]] == NULL)
        return ENOMEM;
    t->size++;

    /* Walk back up the search path */
    while (--top >= 0 && !done) {
        int lh, rh, max;

        lh = __HEIGHT(up[top]->link[upd[top]]);
        rh = __HEIGHT(up[top]->link[!upd[top]]);

        /* Terminate or rebalance as necessary */
        if (lh - rh == 0) {
            done = 1;
        } else if (lh - rh >= 2) {
            struct avl_node *a = up[top]->link[upd[top]]->link[upd[top]];
            struct avl_node *b = up[top]->link[upd[top]]->link[!upd[top]];

            if (__HEIGHT(a) >= __HEIGHT(b)) {
                up[top] = _avl_rot1(up[top], !upd[top]);
            } else {
                up[top] = _avl_rot2(up[top], !upd[top]);
            }

            /* Fix parent */
            if (top != 0) {
                up[top - 1]->link[upd[top - 1]] = up[top];
            } else {
                t->root = up[0];
            }

            done = 1;
        }

        /* Update balance factors */
        lh = __HEIGHT(up[top]->link[upd[top]]);
        rh = __HEIGHT(up[top]->link[!upd[top]]);
        max = __MAX(lh, rh);

        up[top]->height = max + 1;
    }

    return 0;
}

/**
 * Remove an item from avltree
 * @t           the avltree
 * @data        the data to be deleted
 * @return      0 if deleted successfully
 *              ENOENT if item not found
 */
int avl_remove(struct avl_root *t, void *data)
{
    __ASSERT(t != NULL);

    struct avl_node *it, *up[64];
    int upd[64], top = 0;
    ptrdiff_t diff;

    it = t->root;

    while (1) {
        /* Terminate if not found */
        if (it == NULL) return ENOENT;

        diff = t->cmp(it->data, data);
        if (diff == 0) break;

        /* Push direction and node onto stack */
        upd[top] = diff < 0;
        up[top++] = it;

        it = it->link[upd[top - 1]];
    }

    /* Remove the node */
    if (it->link[0] == NULL || it->link[1] == NULL) {
        /* Which child is not null?(if any) */
        int dir = it->link[0] == NULL;

        /* Fix parent */
        if (top != 0) {
            up[top - 1]->link[upd[top - 1]] = it->link[dir];
        } else {
            t->root = it->link[dir];
        }

        t->free(it->data);
        __FREE(it);
        t->size--;
    } else {
        /* Find the in-order successor */
        struct avl_node *heir = it->link[1];

        /* Save the path */
        upd[top] = 1;
        up[top++] = it;

        while (heir->link[0] != NULL) {
            upd[top] = 0;
            up[top++] = heir;
            heir = heir->link[0];
        }

        /* Swap data */
        it->data = heir->data;
        /* Unlink successor and fix parent */
        up[top - 1]->link[up[top - 1] == it] = heir->link[1];

        t->free(heir->data);
        __FREE(heir);
        t->size--;
    }

    /* Walk back up the search path */
    while (--top >= 0) {
        int lh = __HEIGHT(up[top]->link[upd[top]]);
        int rh = __HEIGHT(up[top]->link[!upd[top]]);
        int max = __MAX(lh, rh);

        /* Update balance factors */
        up[top]->height = max + 1;

        /* Terminate or rebalance as necessary */
        if (lh - rh == -1) break;

        if (lh - rh <= -2) {
            struct avl_node *a = up[top]->link[!upd[top]]->link[upd[top]];
            struct avl_node *b = up[top]->link[!upd[top]]->link[!upd[top]];

            if (__HEIGHT(a) <= __HEIGHT(b)) {
                up[top] = _avl_rot1(up[top], upd[top]);
            } else {
                up[top] = _avl_rot2(up[top], upd[top]);
            }

            /* Fix parent */
            if (top != 0) {
                up[top - 1]->link[upd[top - 1]] = up[top];
            } else {
                t->root = up[0];
            }
        }
    }

    return 0;
}

static void _avl_foreach_core(const struct avl_node *n, iter_func_t iter_f)
{
out_iter:
    if (n == NULL) return;

    if (n->link[0])
        _avl_foreach_core(n->link[0], iter_f);

    iter_f(n->data);
    n = n->link[1];
    goto out_iter;
}

/**
 * Iterate through the avltree(in-order)
 *  simple iterative functionality  you may add more functionalities
 *  like iterative flavour, frame flavour, completion, stop condition, etc.
 *
 * NOTE: the iterator function should have no side-effect
 *  o.w. the avltree may debalanced or even crash
 *
 * @t           the avltree
 * @iter_f      the iterative callback
 */
void avl_foreach(const struct avl_root *t, iter_func_t iter_f)
{
    __ASSERT(t != NULL);
    __ASSERT(iter_f != NULL);
    _avl_foreach_core(t->root, iter_f);
}

#ifdef DEBUG
/**
 * Core of avl_assert()
 * @return      size of the avltree
 */
static size_t _avl_assert_r(struct avl_node *n, cmp_func_t cmp)
{
    struct avl_node *link[2];
    size_t size = 0;

    if (n == NULL) goto out_exit;

out_iter:
    link[0] = n->link[0];
    link[1] = n->link[1];
    size++;

    if (link[0]) {
        __ASSERT(cmp(link[0]->data, n->data) < 0);
        size += _avl_assert_r(link[0], cmp);
    }

    if (link[1]) {
        __ASSERT(cmp(link[1]->data, n->data) > 0);
        n = link[1];
        goto out_iter;
    }

    __ASSERT(__ABS(__HEIGHT(link[0]), __HEIGHT(link[1])) <= 1);
out_exit:
    return size;
}

/**
 * Assert the whole avltree(mainly for debugging)
 * @t           the avltree
 */
void avl_assert(const struct avl_root *t)
{
    __ASSERT(t != NULL);
    __ASSERT(t->cmp != NULL);
    __ASSERT(t->free != NULL);
    __ASSERT(_avl_assert_r(t->root, t->cmp) == t->size);
}

/**
 * Print the whole avltree(mainly for debugging)
 * @t           the avltree
 * @print_f     the print function for data
 */
void avl_print(const struct avl_root *t, print_func_t print_f)
{
    __ASSERT(t != NULL);
    __ASSERT(print_f != NULL);
    printf("avltree %p  sz: %zu cmp: %p free: %p\n", t, t->size, t->cmp, t->free);
    avl_foreach(t, print_f);
}
#else
void avl_assert(const struct avl_root *t __UNUSED)
{

}

void avl_print(const struct avl_root *t __UNUSED, print_func_t print_f __UNUSED)
{

}
#endif  /* #ifdef DEBUG */
