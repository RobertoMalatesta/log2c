/************************************************************************/
/* Copyright (c) 1998-2014 Luc Van Oostenryck. All rights reserved.	*/
/*									*/
/* %@%LICENSE								*/
/*									*/
/************************************************************************/

#include "Prolog.h"
#include "pl-op.h"
#include "pl-init.h"			// For PL_init_ops()

struct op_type {
	short type;
	short prec;
};

struct operator {
	struct atom *operator;
	struct op_type type[3];
	struct operator *next;
};

#define OP_HASH_SIZE	256
#define OperatorHashValue(Op)	(((Op)->hash) % OP_HASH_SIZE)

static struct operator *operators[OP_HASH_SIZE];

static struct op_type *get_op_type(struct atom *name, int fix)
{
	struct operator *op;

	for (op = operators[OperatorHashValue(name)]; op; op = op->next) {
		if (op->operator!= name)
			continue;
		return &(op->type[fix - 1]);
	}

	return 0;
}

static struct operator *get_operator(struct atom *name)
{
	struct operator *op;

	for (op = operators[OperatorHashValue(name)]; op; op = op->next) {
		if (op->operator!= name)
			continue;
		return op;
	}

	return 0;
}

static int OperatorAtom2Type(struct atom *type)
{
	if (type == ATOM(_fx))
		return OP_FX;
	if (type == ATOM(_fy))
		return OP_FY;
	if (type == ATOM(_xf))
		return OP_XF;
	if (type == ATOM(_yf))
		return OP_YF;
	if (type == ATOM(_xfx))
		return OP_XFX;
	if (type == ATOM(_xfy))
		return OP_XFY;
	if (type == ATOM(_yfx))
		return OP_YFX;
	if (type == ATOM(_yfy))
		return OP_YFY;

	return 0;
}

static struct atom *OperatorType2Atom(int type)
{
	switch (type) {
	case OP_FX:
		return ATOM(_fx);
	case OP_FY:
		return ATOM(_fy);
	case OP_XF:
		return ATOM(_xf);
	case OP_YF:
		return ATOM(_yf);
	case OP_XFX:
		return ATOM(_xfx);
	case OP_XFY:
		return ATOM(_xfy);
	case OP_YFX:
		return ATOM(_yfx);
	case OP_YFY:
		return ATOM(_yfy);
	default:
		return 0;
	}
}

static void add_operator(int precedence, int type, struct atom *operator)
{
	struct operator *op;
	hash_t h;
	struct op_type *op_t;

	op = get_operator(operator);

	if (op == 0) {
		h = OperatorHashValue(operator);
		op = NEW(*op);
		op->operator= operator;
		op->type[0].prec = op->type[1].prec = op->type[2].prec = 0;
		op->next = operators[h];
		operators[h] = op;
	}

	op_t = &(op->type[(type >> 2) - 1]);
	op_t->type = type;
	op_t->prec = precedence;
}

int PL_is_op(int fix, struct atom *operator, int *type, int *prec)
{
	struct op_type *op_t;

	if (!(op_t = get_op_type(operator, fix)))
		return 0;

	if (op_t->prec <= 0)
		return 0;

	*type = op_t->type;
	*prec = op_t->prec;

	return 1;
}

int PL_can_be_op(struct atom *operator)
{
	return get_operator(operator) != 0;
}

int pl_op(union cell *precedence, union cell *type, union cell *operator)
{
	struct atom *op;
	struct atom *a_t;
	int prec, t;

	if (!(op = PL_get_atom(operator)) ||
	    !(a_t = PL_get_atom(type)) ||
	    !PL_get_intg(precedence, &prec) ||
	    prec < 0 || prec > 1200 || !(t = OperatorAtom2Type(a_t)))
		fail;

	add_operator(prec, t, op);

	succeed;
}

int pl_current_op(union cell *precedence, union cell *type, union cell *operator, enum control *ctrl)
{
	struct operator *op;
	int prec, fix;
	struct atom *name;
	struct {
		int hash;
		struct operator *op;
		int fix;

		struct atom *name;
		int prec;
		int type;
	}     *ctxt;
	int h;

	switch (GetCtrl(ctrl)) {
	struct atom *a_t;
	int t;

	case FIRST_CALL:
		if (!(name = PL_get_atom(operator))) {
			if (PL_is_var(operator))
				name = 0;
			else
				fail;
		}

		if (!PL_get_intg(precedence, &prec)) {
			if (PL_is_var(precedence))
				prec = 0;
			else
				fail;
		}

		if ((a_t = PL_get_atom(type))) {
			if (!(t = OperatorAtom2Type(a_t)))
				fail;
		} else if (PL_is_var(type))
			t = 0;
		else
			fail;

		ctxt = AllocCtxt(*ctxt);
		ctxt->name = name;
		ctxt->prec = prec;
		ctxt->type = t;
		h = OP_HASH_SIZE - 1;
		op = operators[h];
		fix = 0;
		break;
	case NEXT_CALL:
		ctxt = GetCtxt(ctrl);
		h = ctxt->hash;
		op = ctxt->op;
		fix = ctxt->fix;
		break;
	default:
		fail;
	}


	for (; h >= 0; op = operators[--h]) {
		for (; op; op = op->next, fix = 0) {
			if ((ctxt->name && ctxt->name != op->operator))
				continue;

			for (; fix < 3; fix++) {
				struct op_type *op_t = &op->type[fix];

				if (op_t->prec == 0)
					continue;
				if (ctxt->prec && ctxt->prec != op_t->prec)
					continue;
				if (ctxt->type && ctxt->type != op_t->type)
					continue;

				if (!PL_unify_atom(operator, op->operator) ||
				    !PL_unify_atom(type, OperatorType2Atom(op_t->type)) ||
				    !PL_unify_intg(precedence, op_t->prec)
					)
					fail;

				ctxt->hash = h;
				ctxt->op = op;
				ctxt->fix = ++fix;
				retry;
			}
		}
	}

	fail;
}

void PL_init_ops(void)
{
	add_operator(200, OP_FY, ATOM(backslash));
	add_operator(200, OP_FY, ATOM(minus));
	add_operator(200, OP_FY, ATOM(plus));
	add_operator(200, OP_XFX, ATOM(doublestar));
	add_operator(200, OP_XFY, ATOM(hat));
	add_operator(400, OP_YFX, ATOM(_mod));
	add_operator(400, OP_YFX, ATOM(_rem));
	add_operator(400, OP_YFX, ATOM(_xor));
	add_operator(400, OP_YFX, ATOM(div));
	add_operator(400, OP_YFX, ATOM(divide));
	add_operator(400, OP_YFX, ATOM(lshift));
	add_operator(400, OP_YFX, ATOM(rshift));
	add_operator(400, OP_YFX, ATOM(star));
	add_operator(500, OP_YFX, ATOM(and));
	add_operator(500, OP_YFX, ATOM(minus));
	add_operator(500, OP_YFX, ATOM(or));
	add_operator(500, OP_YFX, ATOM(plus));
	add_operator(600, OP_XFY, ATOM(module));
	add_operator(700, OP_XFX, ATOM(_is));
	add_operator(700, OP_XFX, ATOM(ar_equals));
	add_operator(700, OP_XFX, ATOM(ar_larger));
	add_operator(700, OP_XFX, ATOM(ar_larger_equal));
	add_operator(700, OP_XFX, ATOM(ar_not_equal));
	add_operator(700, OP_XFX, ATOM(ar_smaller));
	add_operator(700, OP_XFX, ATOM(ar_smaller_equal));
	add_operator(700, OP_XFX, ATOM(at_equals));
	add_operator(700, OP_XFX, ATOM(at_larger));
	add_operator(700, OP_XFX, ATOM(at_larger_equal));
	add_operator(700, OP_XFX, ATOM(at_not_equals));
	add_operator(700, OP_XFX, ATOM(at_smaller));
	add_operator(700, OP_XFX, ATOM(at_smaller_equal));
	add_operator(700, OP_XFX, ATOM(not_strick_equals));
	add_operator(700, OP_XFX, ATOM(not_unifiable));
	add_operator(700, OP_XFX, ATOM(strick_equals));
	add_operator(700, OP_XFX, ATOM(unify));
	add_operator(700, OP_XFX, ATOM(univ));
	add_operator(900, OP_FY, ATOM(not_provable));
	add_operator(900, OP_FY, ATOM(__2B_3E));
	add_operator(1000, OP_XFY, ATOM(comma));
	add_operator(1050, OP_XFY, ATOM(ifthen));
	add_operator(1050, OP_XFY, ATOM(softcut));
	add_operator(1100, OP_XFY, ATOM(semicolon));
	add_operator(1105, OP_XFY, ATOM(bar));
	add_operator(1200, OP_FX, ATOM(prove));
	add_operator(1200, OP_XFX, ATOM(grammar));
	add_operator(1200, OP_XFX, ATOM(prove));
	add_operator(1200, OP_XFX, ATOM(__3A_2B));
}
