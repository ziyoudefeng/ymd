#include "value.h"
#include "memory.h"
#include "state.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>

//------------------------------------------------------------------
// Skip List:
// -----------------------------------------------------------------
#define MAX_LEVEL 16

unsigned int rand_range(unsigned int mod) {
	unsigned int raw = (unsigned)rand();
	unsigned int rv = 0;
	rv |= ((raw & 0xfffff000) >> 12) ^ ((unsigned)time(NULL));
	rv |= (raw & 0x00000fff) << 20;
	return rv % mod;
}

unsigned short randlv() {
	unsigned short lvl = 1;
	while (rand_range(100) < 50 && lvl < MAX_LEVEL)
		++lvl;
	return lvl;
}

static struct sknd *mknode(unsigned short lv) {
	struct sknd *x = vm_zalloc(sizeof(struct sknd) +
	                           (lv - 1) * sizeof(struct sknd*));
	x->n = lv;
	return x;
}

static struct sknd *append(struct skls *list, struct sknd *update[]) {
	struct sknd *x;
	unsigned short i, lvl = randlv();
	if (lvl > list->lv) {
		for (i = list->lv; i < lvl; ++i)
			update[i] = list->head;
		list->lv = lvl;
	}
	x = mknode(lvl); ++list->count;
	for (i = 0; i < lvl; ++i) {
		x->fwd[i] = update[i]->fwd[i];
		update[i]->fwd[i] = x;
	}
	return x;
}

static struct sknd *skindex(struct skls *list,
                            const struct variable *key) {
	struct sknd *x = list->head, *update[MAX_LEVEL];
	int i;
	memset(update, 0, sizeof(update));
	for (i = list->lv - 1; i >= 0; --i) {
		while (x->fwd[i] && compare(&x->fwd[i]->k, key) < 0) {
			x = x->fwd[i];
		}
		update[i] = x;
	}
	x = x->fwd[0];
	return (x && equal(&x->k, key)) ? x : append(list, update);
}

static const struct variable *skfind(const struct skls *list,
                                     const struct variable *key) {
	const struct sknd *x = list->head;
	int i;
	for (i = list->lv - 1; i >= 0; --i) {
		while (x->fwd[i] && compare(&x->fwd[i]->k, key) < 0) {
			x = x->fwd[i];
		}
	}
	x = x->fwd[0];
	return (x && equal(&x->k, key)) ? &x->v : knax; // `knax` means not found;
}

struct skls *skls_new() {
	struct skls *x = gc_alloc(&vm()->gc, sizeof(struct skls), T_SKLS);
	x->lv = 0;
	x->head = mknode(MAX_LEVEL);
	return x;
}

void skls_final(struct skls *list) {
	struct sknd *i = list->head, *p = i;
	assert(i != NULL);
	while (i) {
		p = i;
		i = i->fwd[0];
		vm_free(p);
	}
}

int skls_equal(const struct skls *list, const struct skls *lhs) {
	const struct sknd *i = list->head;
	if (list == lhs)
		return 1;
	assert(i != NULL);
	while ((i = i->fwd[0]) != NULL) {
		if (!equal(&i->v, skfind(lhs, &i->k)))
			return 0;
	}
	return 1;
}

int skls_compare(const struct skls *list, const struct skls *lhs) {
	const struct sknd *i = list->head;
	int rv = 0;
	if (list == lhs)
		return 0;
	assert(i != NULL);
	while ((i = i->fwd[0]) != NULL)
		rv += compare(&i->v, skfind(lhs, &i->k));
	return rv;
}

struct variable *skls_get(struct skls *list, const struct variable *key) {
	struct sknd *x = skindex(list, key);
	x->k = *key;
	return &x->v;
}
