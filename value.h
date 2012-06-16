#ifndef YMD_VALUE_H
#define YMD_VALUE_H

#include "memory.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

//-----------------------------------------------------------------------
// Type defines:
//-----------------------------------------------------------------------
#define T_NIL     0
#define T_INT     1
#define T_BOOL    2
#define T_EXT     3 // Naked pointer
#define T_KSTR    4 // Constant string
#define T_FUNC    5 // Closure
#define T_DYAY    6 // Dynamic array
#define T_HMAP    7 // Hash map
#define T_SKLS    8 // Skip list
#define T_MAND    9 // Managed data(from C/C++)
#define KNAX     10 // Flag value

#define DECL_TREF(v) \
	v(func, T_FUNC)  \
	v(kstr, T_KSTR)  \
	v(dyay, T_DYAY)  \
	v(hmap, T_HMAP)  \
	v(skls, T_SKLS)  \
	v(mand, T_MAND)

#define MAX_CHUNK_LEN 512

struct context;
struct mach;

typedef long long          ymd_int_t;
typedef unsigned long long ymd_uint_t;
typedef unsigned int       ymd_inst_t;
typedef int (*ymd_nafn_t)(struct context *);


struct variable {
	union {
		struct gc_node *ref;
		void *ext;
		ymd_int_t i;
	} value;
	unsigned char type;
};

// Byte Function
struct chunk {
	int ref; // Reference counter
	ymd_inst_t *inst; // Instructions
	int kinst; // Number of instructions
	struct kstr **kz; // Local constant strings
	struct kstr **lz; // Local variable mapping
	unsigned short kkz;
	unsigned short klz;
	unsigned short kargs; // Prototype number of arguments
};

struct func {
	GC_HEAD;
	struct kstr *proto; // Prototype description
	struct variable *bind; // Binded values
	struct dyay *argv;  // Arguments
	unsigned short is_c;
	unsigned short n_bind;
	union {
		ymd_nafn_t nafn;  // Native function
		struct chunk *core; // Byte code
	} u;
};

// Constant String:
struct kstr {
	GC_HEAD;
	int len;
	size_t hash;
	char land[1];
};

// Dynamic Array:
struct dyay {
	GC_HEAD;
	int count;
	int max;
	struct variable *elem;
};

// Hash Map:
// Key-value pair:
struct kvi {
	struct kvi *next;
	size_t hash;       // Fast hash value
	struct variable k;
	struct variable v;
	unsigned char flag;
};

struct hmap {
	GC_HEAD;
	int shift;
	struct kvi *item;
	struct kvi *free;
};

// Skip List:
struct sknd {
	struct variable k;
	struct variable v;
	unsigned short n;
	struct sknd *fwd[1]; // Forward list
};

struct skls {
	GC_HEAD;
	int count;
	unsigned short lv;
	struct sknd *head;
};

// Managed data (must be from C/C++)
typedef int (*ymd_final_t)(void *);
struct mand {
	GC_HEAD;
	int len; // land length
	const char *tt; // Type name
	ymd_final_t final; // Release function, call in deleted
	unsigned char land[1]; // Payload data
};

// A flag fake variable:
extern struct variable *knax;

#define DEFINE_REFCAST(name, tt) \
static inline struct name *name##_x(struct variable *var) { \
	assert(var->value.ref->type == tt); \
	return (struct name *)var->value.ref; \
} \
static inline const struct name *name##_k(const struct variable *var) { \
	assert(var->value.ref->type == tt); \
	return (const struct name *)var->value.ref; \
}
DECL_TREF(DEFINE_REFCAST)
#undef DEFINE_REFCAST

// Generic functions:
static inline int is_nil(const struct variable *v) {
	return v->type == T_NIL;
}
ymd_int_t int_of(const struct variable *var);
ymd_int_t bool_of(const struct variable *var);
#define DECL_REFOF(name, tt) \
struct name *name##_of(struct variable *var);
DECL_TREF(DECL_REFOF)
#undef DECL_REFOF

static inline void vset_nil(struct variable *v) {
	v->type = T_NIL;
	v->value.i = 0;
}
static inline void vset_int(struct variable *v, ymd_int_t i) {
	v->type = T_INT;
	v->value.i = i;
}
static inline void vset_bool(struct variable *v, ymd_int_t i) {
	v->type = T_BOOL;
	v->value.i = i;
}
static inline void vset_ext(struct variable *v, void *p) {
	v->type = T_EXT;
	v->value.ext = p;
}
#define DEFINE_SETTER(name, tt) \
static inline void vset_##name(struct variable *v, struct name *o) { \
	v->type = tt; \
	v->value.ref = gcx(o); \
}
DECL_TREF(DEFINE_SETTER)
#undef DEFINE_SETTER

// Generic comparing
int equals(const struct variable *lhs, const struct variable *rhs);
int compare(const struct variable *lhs, const struct variable *rhs);

// Constant string: `kstr`
struct kstr *kstr_new(const char *z, int n);
int kstr_equals(const struct kstr *kz, const struct kstr *rhs);
int kstr_compare(const struct kstr *kz, const struct kstr *rhs);
size_t kz_hash(const char *z, int n);
struct kvi *kz_index(struct hmap *map, const char *z, int n);

// Hash map: `hmap` functions:
struct hmap *hmap_new(int count);
void hmap_final(struct hmap *map);
int hmap_equals(const struct hmap *map, const struct hmap *rhs);
int hmap_compare(const struct hmap *map, const struct hmap *rhs);
struct variable *hmap_get(struct hmap *map, const struct variable *key);

// Skip list: `skls` functions:
struct skls *skls_new();
void skls_final(struct skls *list);
int skls_equals(const struct skls *list, const struct skls *rhs);
int skls_compare(const struct skls *list, const struct skls *rhs);
struct variable *skls_get(struct skls *list, const struct variable *key);

// Dynamic array: `dyay` functions:
struct dyay *dyay_new(int count);
void dyay_final(struct dyay *arr);
int dyay_equals(const struct dyay *arr, const struct dyay *rhs);
int dyay_compare(const struct dyay *arr, const struct dyay *rhs);
struct variable *dyay_get(struct dyay *arr, ymd_int_t i);
struct variable *dyay_add(struct dyay *arr);
struct variable *dyay_insert(struct dyay *arr, ymd_int_t i);

// Managed data: `mand` functions:
struct mand *mand_new(const void *data, int size, ymd_final_t final);
void mand_final(struct mand *pm);
int mand_equals(const struct mand *pm, const struct mand *rhs);
int mand_compare(const struct mand *pm, const struct mand *rhs);

// Closure functions:
struct func *func_new(ymd_nafn_t nafn);
const struct kstr *func_init(struct func *fn);
struct func *func_clone(struct func *fn);
void func_final(struct func *fn);
int func_emit(struct func *fn, ymd_inst_t inst);
int func_kz(struct func *fn, const char *z, int n);
int func_find_lz(struct func *fn, const char *z);
int func_add_lz(struct func *fn, const char *z);
int func_bind(struct func *fn, int i, const struct variable *var);
void func_shrink(struct func *fn);
int func_call(struct func *fn, int argc);
int func_main(struct func *fn, int argc, char *argv[]);
struct func *func_compile(FILE *fp);
void func_dump(struct func *fn, FILE *fp);
static inline int func_nlocal(const struct func *fn) {
	assert(!fn->is_c);
	return fn->u.core->klz - fn->n_bind;
}
static inline struct variable *func_bval(struct func *fn, int i) {
	struct variable nil; memset(&nil, 0, sizeof(nil));
	func_bind(fn, i, &nil);
	return fn->bind + i;
}

#endif // YMD_VALUE_H
