#include "value.h"
#include "state.h"
#include "memory.h"
#include <stdio.h>

extern int do_compile(FILE *fp, struct func *fn);

int func_call(struct func *fn, int argc) {
	int rv;
	struct call_info scope;

	scope.pc = 0;
	scope.run = fn;
	scope.chain = ioslate()->info;
	ioslate()->info = &scope;
	if (fn->nafn) {
		rv = (*fn->nafn)(ioslate());
		goto ret;
	}

ret:
	ioslate()->info = ioslate()->info->chain;
	return rv;
}

struct func *func_compile(FILE *fp) {
	struct func *fn = func_new(NULL);
	int rv = do_compile(fp, fn);
	return rv < 0 ? NULL : fn;
}