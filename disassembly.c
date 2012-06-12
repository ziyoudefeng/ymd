#include "disassembly.h"
#include <assert.h>

static const char *kz_test_op[] = {
	"eq", "ne", "gt", "ge", "lt", "le",
};

static const char *kz_logc_op[] = {
	"not", "and", "or",
};

static const char *kz_shift_op[] = {
	"left", "right:l", "right:a",
};

static const char *address(const struct func *fn, uint_t inst,
                           char *buf, size_t n) {
	switch (asm_flag(inst)) {
	case F_INT:
		snprintf(buf, n, "[imm]:%u", asm_param(inst));
		break;
	case F_PARTAL:
		snprintf(buf, n, "[partal]:%04x", asm_param(inst));
		break;
	case F_ZSTR:
		snprintf(buf, n, "[str]:%d:\"%s\"", fn->kz[asm_param(inst)]->len,
				 fn->kz[asm_param(inst)]->land);
		break;
	case F_LOCAL:
		snprintf(buf, n, "[local]:<%d>", asm_param(inst));
		break;
	case F_BOOL:
		snprintf(buf, n, asm_param(inst) == 0 ? "false" : "true");
		break;
	case F_NIL:
		snprintf(buf, n, "nil");
		break;
	case F_OFF:
		snprintf(buf, n, "[global]:@%s", fn->kz[asm_param(inst)]->land);
		break;
	default:
		assert(0);
		return "<N/A>";
	}
	return buf;
}

static const char *jmp(uint_t inst, char *buf, size_t n) {
	switch (asm_flag(inst)) {
	case F_FORWARD:
		snprintf(buf, n, "+%d", asm_param(inst));
		break;
	case F_BACKWARD:
		snprintf(buf, n, "-%d", asm_param(inst));
		break;
	default:
		assert(0);
		return "<N/A>";
	}
	return buf;
}

int dis_inst(FILE *fp, const struct func *fn, uint_t inst) {
	char buf[128];
	int rv;
#define BUF buf, sizeof(buf)
	switch (asm_op(inst)) {
	case I_PANIC:
		rv = fprintf(fp, "panic");
		break;
	case I_CLOSURE:
		rv = fprintf(fp, "closure");
		break;
	case I_STORE:
		rv = fprintf(fp, "store %s", address(fn, inst, BUF));
		break;  
	case I_RET:
		rv = fprintf(fp, "ret %d", asm_param(inst));
		break;
	case I_JNE:
		rv = fprintf(fp, "jne %s", jmp(inst, BUF));
		break;
	case I_JMP:
		rv = fprintf(fp, "jmp %s", jmp(inst, BUF));
		break;
	case I_FOREACH:
		rv = fprintf(fp, "foreach %s", jmp(inst, BUF));
		break;
	case I_SETF:
		rv = fprintf(fp, "setf %d", asm_param(inst));
		break;
	case I_PUSH:
		rv = fprintf(fp, "push %s", address(fn, inst, BUF));
		break;
	case I_TEST:
		rv = fprintf(fp, "test <%s>", kz_test_op[asm_flag(inst)]);
		break;
	case I_LOGC:
		rv = fprintf(fp, "logc <%s>", kz_logc_op[asm_flag(inst)]);
		break;
	case I_INDEX:
		rv = fprintf(fp, "index");
		break;
	case I_TYPEOF:
		rv = fprintf(fp, "typeof");
		break;
	case I_INV:
		rv = fprintf(fp, "inv");
		break;
	case I_MUL:
		rv = fprintf(fp, "mul");
		break;
	case I_DIV:
		rv = fprintf(fp, "div");
		break;
	case I_ADD:
		rv = fprintf(fp, "add");
		break;
	case I_SUB:
		rv = fprintf(fp, "sub");
		break;
	case I_MOD:
		rv = fprintf(fp, "mod");
		break;
	case I_POW:
		rv = fprintf(fp, "pow");
		break;
	case I_ANDB:
		rv = fprintf(fp, "andb");
		break;
	case I_ORB:
		rv = fprintf(fp, "orb");
		break;
	case I_XORB:
		rv = fprintf(fp, "xorb");
		break;
	case I_INVB:
		rv = fprintf(fp, "invb");
		break;
	case I_SHIFT:
		rv = fprintf(fp, "shift [%s]", kz_shift_op[asm_param(inst)]);
		break;
	case I_CALL:
		rv = fprintf(fp, "call %d", asm_param(inst));
		break;
	case I_NEWMAP:
		rv = fprintf(fp, "newmap %d", asm_param(inst));
		break;
	case I_NEWSKL:
		rv = fprintf(fp, "newskl %d", asm_param(inst));
		break;
	case I_NEWDYA:
		rv = fprintf(fp, "newdya %d", asm_param(inst));
		break;
	case I_BIND:
		rv = fprintf(fp, "bind %d", asm_param(inst));
		break;
	case I_LOAD:
		rv = fprintf(fp, "load [func]:<%d>", asm_param(inst));
		break;
	default:
		assert(0);
		break;
	}
#undef BUF
	return rv;
}

int dis_func(FILE *fp, const struct func *fn) {
	int i, count = 0;
	for (i = 0; i < fn->n_inst; ++i) {
		fprintf(fp, "[%03d] ", i);
		count += dis_inst(fp, fn, fn->inst[i]);
		fprintf(fp, "\n");
	}
	return count;
}
