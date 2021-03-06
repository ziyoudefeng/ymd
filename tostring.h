#ifndef YMD_TOSTRING_H
#define YMD_TOSTRING_H

#include <string.h>
#include <assert.h>

struct variable;

// Format context
#define FMTX_STATIC_MAX 260
struct fmtx {
	char kbuf[FMTX_STATIC_MAX];
	char *dy;
	int last;
	int max;
};
#define FMTX_INIT { {0}, NULL, 0, FMTX_STATIC_MAX }

void fmtx_final(struct fmtx *self);

static inline void *fmtx_buf(struct fmtx *self) {
	return !self->dy ? self->kbuf : self->dy;
}

static inline int fmtx_remain(struct fmtx *self) {
	return self->max - self->last;
}

static inline void *fmtx_last(struct fmtx *self) {
	return fmtx_buf(self) + self->last;
}

static inline void *fmtx_advance(struct fmtx *self, int k) {
	assert(self->last + k < self->max);
	self->last += k;
	return fmtx_last(self);
}

static inline void *fmtx_add(struct fmtx *self) {
	self->last += strlen(fmtx_last(self));
	return fmtx_buf(self);
}

void fmtx_need(struct fmtx *self, int n);

static inline const void *fmtx_append(struct fmtx *self,
                                      const void *src, int n) {
	fmtx_need(self, n);
	memcpy(fmtx_last(self), src, n);
	self->last += n;
	return fmtx_buf(self);
}

// format a variable to formated string:
const char *tostring(struct fmtx *ctx, const struct variable *var);

#endif // YMD_TOSTRING_H

