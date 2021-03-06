#ifndef YMD_STATE_H
#define YMD_STATE_H

#include "memory.h"
#include "value.h"
#include "value_inl.h"
#include <string.h>
#include <setjmp.h>
#include <assert.h>

struct ymd_context;
struct ymd_mach;

// Constant string pool
struct kpool {
	struct gc_node **slot;
	int used;
	int shift;
};

struct ymd_mach {
	struct hmap *global;
	struct kpool kpool;
	struct gc_struct gc;
	// Internal memory function:
	void *(*zalloc)(struct ymd_mach *, void *, size_t);
	void (*free)(struct ymd_mach *, void *);
	void *cookie;
	ymd_int_t tick;
	struct ymd_context *curr;
	struct variable knil; // nil flag
};

struct ymd_mach *ymd_init();

void ymd_final(struct ymd_mach *vm);

struct ymd_context *ioslate(struct ymd_mach *vm);

static inline void ymd_log4gc(struct ymd_mach *vm, FILE *logf) {
	vm->gc.logf = logf;
}

// Mach functions:
static inline void *vm_zalloc(struct ymd_mach *vm, size_t size) {
	return vm->zalloc(vm, NULL, size);
}
static inline void *vm_realloc(struct ymd_mach *vm, void *p, size_t size) {
	return vm->zalloc(vm, p, size);
}
static inline void vm_free(struct ymd_mach *vm, void *p) {
	vm->free(vm, p);
}

#define UNUSED(useless) ((void)useless)

#define MAX_KPOOL_LEN 40
#define MAX_LOCAL     512
#define MAX_STACK     128
#define FUNC_ALIGN    128
#define GC_THESHOLD   10 * 1024

struct call_info {
	struct call_info *chain;
	struct func *run;
	int pc;
	struct variable *loc;
};

struct call_jmpbuf {
	struct call_jmpbuf *chain;
	int level;
	int panic;
	jmp_buf core; // ANSI-C jmp_buf
};

struct ymd_context {
	struct call_info *info;
	struct variable loc[MAX_LOCAL];
	struct variable stk[MAX_STACK];
	struct variable *top;
	struct call_jmpbuf *jpt; // Jump point
	struct ymd_mach *vm;
};

//-----------------------------------------------------------------------------
// For debug and testing:
// ----------------------------------------------------------------------------
struct call_info *vm_nearcall(struct ymd_context *l);

static inline const char *vm_file(const struct call_info *i) {
	assert(i != NULL);
	return i->run->u.core->file->land;
}

static inline int vm_line(const struct call_info *i) {
	assert(i != NULL);
	return i->run->u.core->line[i->pc - 1];
}

int vm_reached(struct ymd_mach *vm, const char *name);

//-----------------------------------------------------------------------------
// Call and run:
// ----------------------------------------------------------------------------
void ymd_panic(struct ymd_context *l, const char *fmt, ...);

void ymd_error(struct ymd_context *l);

int ymd_call(struct ymd_context *l, struct func *fn, int argc, int method);

// Internal protected call
int ymd_pcall(struct ymd_context *l, struct func *fn, int argc);

// External protected call
int ymd_xcall(struct ymd_context *l, int argc);

int ymd_ncall(struct ymd_context *l, struct func *fn, int nret, int narg);
int ymd_main(struct ymd_context *l, int argc, char *argv[]);

//-----------------------------------------------------------------------------
// Misc:
// ----------------------------------------------------------------------------
// Get/Put a container: hashmap/skiplist/array
struct variable *vm_put(struct ymd_mach *vm, struct variable *var,
                        const struct variable *key);

struct variable *vm_get(struct ymd_mach *vm, struct variable *var,
                        const struct variable *key);

// Get/Put global variable
struct variable *vm_putg(struct ymd_mach *vm, const char *field);

struct variable *vm_getg(struct ymd_mach *vm, const char *field);


// Define/Get object's member
struct variable *vm_def(struct ymd_mach *vm, void *o, const char *field);

struct variable *vm_mem(struct ymd_mach *vm, void *o, const char *field);

// String tool
struct kstr *vm_strcat(struct ymd_mach *vm, const struct kstr *lhs,
                       const struct kstr *rhs);

struct kstr *vm_format(struct ymd_mach *vm, const char *fmt, ...);

// Runtime
static inline struct func *ymd_called(struct ymd_context *l) {
	assert(l->info);
	assert(l->info->run);
	return l->info->run;
}

static inline struct dyay *ymd_argv(struct ymd_context *l) {
	return ymd_called(l)->argv;
}

static inline struct dyay *ymd_argv_chk(struct ymd_context *l, int need) {
	if (need > 0 && (!ymd_argv(l) || ymd_argv(l)->count < need))
		ymd_panic(l, "Bad argument, need > %d", need);
	return ymd_argv(l);
}

static inline struct variable *ymd_argv_get(struct ymd_context *l, int i) {
	struct dyay *argv = ymd_argv_chk(l, i + 1);
	return argv->elem + i;
}

static inline struct variable *ymd_bval(struct ymd_context *l, int i) {
	assert(i >= 0);
	assert(i < ymd_called(l)->n_bind);
	return ymd_called(l)->bind + i;
}

//-----------------------------------------------------------------------------
// Stack functions:
// ----------------------------------------------------------------------------
static inline struct variable *ymd_push(struct ymd_context *l) {
	if (l->top >= l->stk + MAX_STACK)
		ymd_panic(l, "Stack overflow!");
	++l->top;
	return l->top - 1;
}

static inline struct variable *ymd_top(struct ymd_context *l, int i) {
	if (l->top == l->stk)
		ymd_panic(l, "Stack empty!");
	if (i < 0 && 1 - i >= l->top - l->stk)
		ymd_panic(l, "Stack out of range");
	if (i > 0 &&     i >= l->top - l->stk)
		ymd_panic(l, "Stack out of range");
	return i < 0 ? l->stk + 1 - i : l->top - 1 - i;
}

static inline void ymd_move(struct ymd_context *l, int i) {
	struct variable k, *p = ymd_top(l, i);
	if (i == 0) return;
	k = *p;
	memmove(p, p + 1, (l->top - p - 1) * sizeof(k));
	*ymd_top(l, 0) = k;
}

static inline void ymd_pop(struct ymd_context *l, int n) {
	if (n > 0 && l->top == l->stk)
		ymd_panic(l, "Stack empty!");
	if (n > l->top - l->stk)
		ymd_panic(l, "Bad pop!");
	l->top -= n;
	memset(l->top, 0, sizeof(*l->top) * n);
}

// Adjust return variables
static inline int ymd_adjust(struct ymd_context *l, int adjust, int ret) {
	if (adjust < ret) {
		ymd_pop(l, ret - adjust);
		return ret;
	}
	if (adjust > ret) {
		int i = adjust - ret;
		while (i--)
			vset_nil(ymd_push(l));
	}
	return ret;
}

//-----------------------------------------------------------------------------
// Fake stack functions:
// ----------------------------------------------------------------------------
static inline void ymd_dyay(struct ymd_context *l, int k) {
	struct dyay *o = dyay_new(l->vm, k);
	vset_dyay(ymd_push(l), o); 
	gc_release(o);
}

static inline void ymd_add(struct ymd_context *l) {
	struct dyay *o = dyay_of(l, ymd_top(l, 1));
	*dyay_add(l->vm, o) = *ymd_top(l, 0);
	ymd_pop(l, 1);
}

static inline void ymd_hmap(struct ymd_context *l, int k) {
	struct hmap *o = hmap_new(l->vm, k);
	vset_hmap(ymd_push(l), o);
	gc_release(o);
}

static inline void ymd_skls(struct ymd_context *l) {
	struct skls *o = skls_new(l->vm);
	vset_skls(ymd_push(l), o);
	gc_release(o);
}

static inline void *ymd_mand(struct ymd_context *l, const char *tt,
                             size_t size, ymd_final_t final) {
	struct mand *o = mand_new(l->vm, size, final);
	o->tt = tt;
	vset_mand(ymd_push(l), o);
	gc_release(o);
	return o->land;
}

static inline void ymd_kstr(struct ymd_context *l, const char *z,
	                        int len) {
	struct kstr *o = kstr_fetch(l->vm, z, len);
	vset_kstr(ymd_push(l), o);
	gc_release(o);
}

void ymd_format(struct ymd_context *l, const char *fmt, ... );

static inline void ymd_int(struct ymd_context *l, ymd_int_t i) {
	vset_int(ymd_push(l), i);
}

static inline void ymd_ext(struct ymd_context *l, void *p) {
	vset_ext(ymd_push(l), p);
}

static inline void ymd_bool(struct ymd_context *l, int b) {
	vset_bool(ymd_push(l), b);
}

static inline void ymd_nil(struct ymd_context *l) {
	vset_nil(ymd_push(l));
}

static inline void ymd_nafn(struct ymd_context *l, ymd_nafn_t fn,
                            const char *name, int nbind) {
	struct func *o = func_new_c(l->vm, fn, name);
	o->n_bind = nbind;
	vset_func(ymd_push(l), o);
	gc_release(o);
}

static inline void ymd_func(struct ymd_context *l, struct chunk *blk,
                            const char *name, int nbind) {
	struct func *o = func_new(l->vm, blk, name);
	o->n_bind = nbind;
	vset_func(ymd_push(l), o);
	gc_release(o);
}

static inline struct func *ymd_naked(struct ymd_context *l,
	                                 struct chunk *blk) {
	struct func *o = func_new(l->vm, blk, NULL);
	vset_func(ymd_push(l), o);
	gc_release(o);
	return o;
}

static inline void ymd_getf(struct ymd_context *l) {
	struct variable *v = vm_get(l->vm, ymd_top(l, 1), ymd_top(l, 0));
	*ymd_top(l, 0) = *v;
}

static inline void ymd_putf(struct ymd_context *l) {
	struct variable *v = vm_put(l->vm, ymd_top(l, 2), ymd_top(l, 1));
	*v = *ymd_top(l, 0);
	ymd_pop(l, 2);
}

static inline void ymd_mem(struct ymd_context *l, const char *field) {
	struct variable *v;
	if (!is_ref(ymd_top(l, 0)))
		ymd_panic(l, "object must be hashmap or skiplist");
	v = vm_mem(l->vm, ymd_top(l, 0)->value.ref, field);
	*ymd_push(l) = *v;
}

static inline void ymd_def(struct ymd_context *l, const char *field) {
	if (!is_ref(ymd_top(l, 1)))
		ymd_panic(l, "object must be hashmap or skiplist");
	*vm_def(l->vm, ymd_top(l, 1)->value.ref, field) = *ymd_top(l, 0);
	ymd_pop(l, 1);
}

static inline void ymd_getg(struct ymd_context *l, const char *field) {
	*ymd_push(l) = *vm_getg(l->vm, field);
}

static inline void ymd_putg(struct ymd_context *l, const char *field) {
	*vm_putg(l->vm, field) = *ymd_top(l, 0);
	ymd_pop(l, 1);
}

static inline void ymd_bind(struct ymd_context *l, int i) {
	struct func *o = func_of(l, ymd_top(l, 1));
	*func_bval(l->vm, o, i) = *ymd_top(l, 0);
	ymd_pop(l, 1);
}

static inline void ymd_insert(struct ymd_context *l) {
	struct dyay *o = dyay_of(l, ymd_top(l, 2));
	ymd_int_t i = int_of(l, ymd_top(l, 1));
	*dyay_insert(l->vm, o, i) = *ymd_top(l, 0);
	ymd_pop(l, 2);
}

static inline void ymd_setmetatable(struct ymd_context *l) {
	struct mand *o = mand_of(l, ymd_top(l, 1));
	if (ymd_top(l, 0)->type != T_HMAP &&
		ymd_top(l, 0)->type != T_SKLS) ymd_panic(l, "Not metatable type!");
	mand_proto(o, ymd_top(l, 0)->value.ref);
	ymd_pop(l, 1);
}

#endif // YMD_STATE_H

