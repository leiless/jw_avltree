/*
 * Created 18E29
 *
 * User-space avltree fuzzing test program
 */

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "jw_avltree.h"

void loselose_srand(void)
{
    struct timeval tv;
    unsigned long junk = 0xdeadbeef;
    uintptr_t p = (uintptr_t) &junk;
    gettimeofday(&tv, NULL);
    srand((unsigned int) ((getpid() << 16) ^ tv.tv_sec ^ tv.tv_usec ^ (p & 0xffff00)));
}

void print_data(void *data)
{
    fprintf(stderr, "%d, ", (int) data);
}

void test1(void)
{
    struct avl_root *t = avl_alloc(NULL, NULL);
    avl_print(t, print_data);
    avl_assert(t);

    size_t i;
    const size_t LIM = 25000000;
    const int RNDLIM = LIM << 3;
    uintptr_t rnd;

    size_t ok, exist, nomem, unk;

    ok = exist = nomem = unk = 0;
    for (i = 0; i < LIM; i++) {
        rnd = rand() % RNDLIM;
        switch (avl_insert(t, (void *) rnd)) {
        case 0:
            ok++;
            assert(avl_find(t, (void *) rnd) != 0);
            break;
        case EEXIST: exist++; break;
        case ENOMEM: nomem++; break;
        default: unk++; break;
        }
    }

    assert(unk == 0);
    assert(ok == avl_size(t));
    avl_assert(t);
#if 0
    avl_print(t, print_data);
    putchar('\n');
#endif
    fprintf(stderr, "ok: %zu exist: %zu nomem: %zu\n", ok, exist, nomem);

    size_t dok, dnoent;

    dok = dnoent = unk = 0;
    for (i = 0; i < LIM; i++) {
        rnd = rand() % RNDLIM;
        switch (avl_remove(t, (void *) rnd)) {
        case 0:
            dok++;
            assert(avl_find(t, (void *) rnd) == 0);
            break;
        case ENOENT: dnoent++; break;
        default: unk++; break;
        }
    }

    assert(unk == 0);
    assert(dok + avl_size(t) == ok);
    avl_assert(t);
#if 0
    avl_print(t, print_data);
    putchar('\n');
#endif
    fprintf(stderr, "sz: %zu dok: %zu dnoent: %zu\n", avl_size(t), dok, dnoent);

    avl_clear(t);
    avl_assert(t);
    avl_print(t, print_data);

    avl_free(&t);
    assert(t == NULL);
}

void test2(void)
{
    /* TODO: wrap up more */
}

int main(void)
{
    loselose_srand();

    system("/bin/date");
    test1();
    system("/bin/date");
    test2();
    system("/bin/date");
    return 0;
}
