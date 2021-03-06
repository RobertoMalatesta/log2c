/************************************************************************/
/* Copyright (c) 1998-2014 Luc Van Oostenryck. All rights reserved.	*/
/*									*/
/* %@%LICENSE								*/
/*									*/
/************************************************************************/

#include "Prolog.h"
#include "pl-io.h"
#include "pl-stream.h"
#include "pl-option.h"
#include "pl-fli.h"
#include "pl-atom.h"
#include "pl-ctype.h"
#include "pl-os.h"
#include "pl-status.h"
#include "pl-init.h"			// For init/exit_io

#define	MAX_FILES	(OPEN_MAX/2)

struct pl_file {
	struct atom *file;
	struct atom *alias;
	struct stream *S;
};

// INV : A closed struct pl_file *have the components file, alias and S == 0

static int max_files;
static struct pl_file plfiles[MAX_FILES] = {
	{ATOM(_user), ATOM(_user__input), 0},
	{ATOM(_user), ATOM(_user__output), 0},
	{0, ATOM(_user__error), 0}
};

static struct pl_file *Finput = &(plfiles[0]);
static struct pl_file *Foutput = &(plfiles[1]);

void PL_init_io(void)
{
	plfiles[0].S = Stdin;
	plfiles[1].S = Stdout;
	plfiles[2].S = Stderr;
	pl_init_stream();
	max_files = 3;
}

/****************************************************************/
// Function for alias handling

static void AddAlias(struct pl_file *f, struct atom *alias)
{
	f->alias = alias;
}

static struct pl_file *GetAliasStream(struct atom *alias)
{
	int i;
	struct pl_file *f = 0;

	for (i = 0; i < max_files; i++) {
		if (plfiles[i].alias == alias) {
			f = &plfiles[i];
			break;
		}
	}
	return f;
}

static int TestAlias(struct atom *alias)
{
	int i;

	for (i = 0; i < max_files; i++) {
		if (plfiles[i].alias == alias) {
			PL_warn("Stream Name '%s' already in use", alias->name);
			return 0;
		}
	}
	return 1;
}

static void DelStreamAliases(struct pl_file *f)
{
	f->alias = 0;
}

/****************************************************************/
static void CloseStream(struct pl_file *f)
{
	if (f->S)
		switch (f - plfiles) {
		case 0:
			Sclearerr(f->S);
			break;
		case 1:
		case 2:
			Sflush(f->S);
			break;
		default:
			Sclose(f->S);
			f->S = 0;
			DelStreamAliases(f);
			f->file = 0;
			break;
		}

	if (f == &plfiles[max_files - 1])
		max_files--;
}

void PL_exit_io(void)
{
	int n;

	for (n = 0; n < max_files; n++) {
		if (!plfiles[n].file || !plfiles[n].S)
			continue;

		Sclose(plfiles[n].S);
	}
}

static struct pl_file *openStream(union cell *file, enum stream_mode mode, int flags)
{
	struct stream *S;
	struct atom *name;
	struct functor *f;
	enum stream_type type;
	struct pl_file *fp;

	if ((name = PL_get_atom(file))) {
		if (mode == SM_READ) {
			if (name == ATOM(_user) || name == ATOM(_user__input)) {
				return &plfiles[0];
			}
		} else {
			if (name == ATOM(_user) || name == ATOM(_user__output)) {
				return &plfiles[1];
			}
			if (name == ATOM(_user__error)) {
				return &plfiles[2];
			}
		}
		type = ST_FILE;
	} else if (PL_get_functor(file, &f) && f == FUN(_pipe, 1)) {
		type = ST_PIPE;
	} else
		PL_warning("Illegal stream specification");

	if (!(flags & SF_OPEN)) {	/* see/1, tell/1, append/1 */
		for (fp = plfiles; fp < plfiles + max_files; fp++) {
			if (fp->file == name && StreamType(fp->S) == type) {
				if (StreamMode(fp->S) == mode) {
					CloseStream(fp);
				} else
					goto OK;
			}
		}
	}

	if (type == ST_PIPE) {
		if (!(S = Sopen_pipe(name->name, mode, flags))) {
			if (PL__status.file_err) {
				PL_warning("Cannot open pipe %s: %s", name->name, PL_OsError());
			}
			fail;
		}
	} else				// type == ST_FILE
	{
		char *fn = PL_ExpandFile(name->name, 0);

		if (!fn)
			fail;

		if (!(S = Sopen_file(fn, mode, flags))) {
			if (PL__status.file_err) {
				PL_warn("Cannot open %s", name->name);
			}
			fail;
		}
	}

	for (fp = plfiles; fp < plfiles + max_files && fp->S; fp++) ;

	if (fp == plfiles + MAX_FILES)
		PL_warning("Cannot handle more than %d open files", MAX_FILES);
	if (fp == plfiles + max_files)
		max_files++;

	fp->file = name;
	fp->alias = 0;
	fp->S = S;

OK:
	return fp;
}

static struct pl_file *GetStream(union cell *spec, enum stream_mode mode)
{
	struct pl_file *f = 0;
	int n;
	struct atom *alias;

	if (PL_get_intg(spec, &n)) {
		if (n < max_files && plfiles[n].S)
			f = &plfiles[n];
	} else if ((alias = PL_get_atom(spec))) {
		if (alias == ATOM(_user)) {
			if (mode & SM_READ)
				f = Finput;
			else
				f = Foutput;
		} else {
			f = GetAliasStream(alias);
		}
	}

	if (!f)
		PL_warning("Illegal I/O stream specification");

	switch (mode) {
	case SM_READ:
		if (StreamMode(f->S) != SM_READ)
			PL_warning("Stream was not open for reading");
		break;
	case SM_APPEND:
	case SM_UPDATE:
	case SM_WRITE:
		if (StreamMode(f->S) == SM_READ)
			PL_warning("Stream was not open for writing");
		break;
	case SM_ANY:
		break;
	}

	return f;
}

static int unifyStreamMode(union cell *mode, struct stream *S)
{
	enum stream_mode m = StreamMode(S);
	struct atom *a;

	switch (m) {
	case SM_READ:
		a = ATOM(_read);
		break;
	case SM_WRITE:
		a = ATOM(_write);
		break;
	case SM_APPEND:
		a = ATOM(_append);
		break;
	case SM_UPDATE:
		a = ATOM(_update);
		break;
	default:
		fail;
	}

	return PL_unify_atom(mode, a);
}

/**********************************************************************/
/* see, tell and C°                                                   */
/**********************************************************************/

int pl_see(union cell *s)
{
	struct pl_file *f;

	if ((f = openStream(s, SM_READ, 0))) {
		Finput = f;
		succeed;
	} else
		fail;
}

int pl_tell(union cell *s)
{
	struct pl_file *f;

	if ((f = openStream(s, SM_WRITE, 0))) {
		Foutput = f;
		succeed;
	} else
		fail;
}

int pl_append(union cell *s)
{
	struct pl_file *f;

	if ((f = openStream(s, SM_APPEND, 0))) {
		Foutput = f;
		succeed;
	} else
		fail;
}

int pl_seeing(union cell *s)
{
	return PL_unify_atom(s, Finput->file);
}					// FIXME : if pipe ???

int pl_telling(union cell *s)
{
	return PL_unify_atom(s, Foutput->file);
}					// FIXME : if pipe ???

int pl_seen(void)
{
	CloseStream(Finput);
	Finput = &plfiles[0];
	return 1;
}

int pl_told(void)
{
	CloseStream(Foutput);
	Foutput = &plfiles[1];
	return 1;
}

/**********************************************************************/
/* open and C°                                                        */
/**********************************************************************/

static struct atom *opt_type;
static struct atom *opt_alias;
static struct atom *opt_eof_action;
static int opt_reposition;		// Don't care
static struct pl_option_spec spec_open4[] = {
	{ATOM(_alias), OPT_ATOM, {.atom = &opt_alias}},
	{ATOM(_eof__action), OPT_ATOM, {.atom = &opt_eof_action}},
	{ATOM(_reposition), OPT_BOOL, {.bool = &opt_reposition}},
	{ATOM(_type), OPT_ATOM, {.atom = &opt_type}},
	{0, 0, {0}}
};

int pl_open4(union cell *srcdest, union cell *mode, union cell *stream, union cell *options)
{
	int s_flags = 0;
	struct atom *m;
	struct pl_file *f;
	enum stream_mode s_mode;

// init default
	opt_type = ATOM(_text);
	opt_alias = 0;
	opt_eof_action = ATOM(_eof__code);

// get options
	if (options) {
		if (!PL_scan_options(options, spec_open4))
			PL_warning("open/4 : Illegal option list");
	}
// get mode
	if (!(m = PL_get_atom(mode)))
		PL_warning("open/4 : Illegal mode specification");
	if (m == ATOM(_read))
		s_mode = SM_READ;
	else if (m == ATOM(_write))
		s_mode = SM_WRITE;
	else if (m == ATOM(_append))
		s_mode = SM_APPEND;
	else if (m == ATOM(_update))
		s_mode = SM_UPDATE;
	else
		PL_warning("open/4 : Illegal mode specification");

// get text/binary stream type
	if (opt_type == ATOM(_binary))
		s_flags |= SF_BINARY;
	else {
		if (opt_type != ATOM(_text))
			PL_warn("open/4 : Illegal type specification");
		// default : OK
	}

// get eof_action flags
	if (opt_eof_action == ATOM(_error))
		s_flags |= SF_EOF_ERR;
	else if (opt_eof_action == ATOM(_reset))
		s_flags |= SF_EOF_RESET;
	else if (opt_eof_action != ATOM(_eof__code))
		PL_warn("open/4 : Illegal eof_action specification");

// open the stream
	if (!(f = openStream(srcdest, s_mode, s_flags)))
		fail;

// Set alias if needed
	if (opt_alias) {
		if (!TestAlias(opt_alias))
			fail;

		AddAlias(f, opt_alias);
		return PL_unify_atom(stream, opt_alias);
	} else
		return PL_unify_intg(stream, f - plfiles);
}

int pl_open3(union cell *srcdest, union cell *mode, union cell *stream)
{
	return pl_open4(srcdest, mode, stream, 0);
}

// FIXME : open_null_stream ???

int pl_close(union cell *stream)
{
	struct pl_file *f;

	if (!(f = GetStream(stream, SM_ANY)))
		fail;

	CloseStream(f);
	if (f == Finput) {
		Finput = &plfiles[0];
	} else if (f == Foutput) {
		Foutput = &plfiles[1];
	}

	succeed;
}

int pl_current_stream(union cell *file, union cell *mode, union cell *stream, enum control *ctrl)
{
	int *ctxt, n;

	switch (GetCtrl(ctrl)) {
	case FIRST_CALL:
		ctxt = AllocCtxt(*ctxt);
		n = 0;
		break;
	case NEXT_CALL:
		ctxt = GetCtxt(ctrl);
		n = *ctxt;
		break;
	default:
		fail;
	}

	for (; n < max_files; n++) {
		struct mark m;

		if (!plfiles[n].S)
			continue;

		Mark(m);
		if (!(PL_unify_atom(file, plfiles[n].file) &&
		      unifyStreamMode(mode, plfiles[n].S) && PL_unify_intg(stream, n))) {
			Undo(m);
		} else {
			*ctxt = n + 1;
			retry;
		}
	}

	fail;
}

/**********************************************************************/
/* switching between implicit and explicit stream                     */
/**********************************************************************/

int pl_set_input(union cell *s)
{
	struct pl_file *f;

	if (!(f = GetStream(s, SM_READ)))
		fail;

	Finput = f;
	succeed;
}

int pl_set_output(union cell *s)
{
	struct pl_file *f;

	if (!(f = GetStream(s, SM_WRITE)))
		fail;

	Foutput = f;
	succeed;
}

int pl_current_input(union cell *s)
{
	return PL_unify_intg(s, Finput - plfiles);
}

int pl_current_output(union cell *s)
{
	return PL_unify_intg(s, Foutput - plfiles);
}

/**********************************************************************/
/* Stream status                                                      */
/**********************************************************************/

// FIXME : wait_for_input

/**********************************************************************/
/* Primitive Character I/O                                            */
/**********************************************************************/

#define OutputStream(s)	\
	({ struct pl_file *f; \
	   if (!(f=GetStream(s,SM_WRITE)) || !f->S) fail; \
	   f->S; \
	})
#define InputStream(s)	\
	({ struct pl_file *f; \
	   if (!(f=GetStream(s,SM_READ)) || !f->S) fail; \
	   f->S; \
	})
#define IOStream(s)	\
	({ struct pl_file *f; \
	   if (!(f=GetStream(s,SM_ANY)) || !f->S) fail; \
	   S=f->S; \
	})

struct stream *PL_Output_Stream(union cell *s)
{
	return OutputStream(s);
}

struct stream *PL_Input_Stream(union cell *s)
{
	return InputStream(s);
}

struct stream *PL_OutStream(void)
{
	return Foutput->S;
}

struct stream *PL_InStream(void)
{
	return Finput->S;
}

static int Put(union cell *t, struct stream *S)
{
	int c;
	struct atom *a;

	if (PL_get_intg(t, &c)) {
		c = (unsigned char)c;
	}				// FIXME : 0 <= c <= 256 ??
	else if ((a = PL_get_atom(t))) {
		c = a->name[0];
	}				// FIXME : 'a' must be single character
	else
		fail;

	Sputc(S, c);
	succeed;
}

int pl_put(union cell *c)
{
	return Put(c, Foutput->S);
}

int pl_put2(union cell *s, union cell *c)
{
	struct stream *S = OutputStream(s);
	return Put(c, S);
}

int pl_nl(void)
{
	Sputc(Foutput->S, '\n');
	return 1;
}

int pl_nl1(union cell *s)
{
	struct stream *S = OutputStream(s);
	Sputc(S, '\n');
	return 1;
}

int pl_tab(union cell *N)
{
	long n;
	if (!PL_eval_(N, &n) || n < 0)
		fail;
	for (; n--;)
		Sputc(Foutput->S, ' ');
	return 1;
}

int pl_tab2(union cell *s, union cell *N)
{
	long n;
	struct stream *S;

	if (!PL_eval_(N, &n) || n < 0)
		fail;
	S = OutputStream(s);
	for (; n--;)
		Sputc(S, ' ');
	return 1;
}

int pl_flush(void)
{
	return !Sflush(Foutput->S);
}

int pl_flush_output(union cell *s)
{
	struct stream *S = OutputStream(s);
	return !Sflush(S);
}

static int Get0(struct stream *S)
{
	int c = Sgetc(S);
	return c;
}

int pl_get0(union cell *c)
{
	return PL_unify_intg(c, Get0(Finput->S));
}

int pl_get02(union cell *s, union cell *c)
{
	struct stream *S = InputStream(s);
	return PL_unify_intg(c, Get0(S));
}

int pl_get_single_char(union cell *c)
{
	return PL_unify_intg(c, PL_GetSingleChar());
}

static int Get(struct stream *S)
{
	int c;
	do
		c = Get0(S);
	while (isSpace(c));
	return c;
}

int pl_get(union cell *c)
{
	return PL_unify_intg(c, Get(Finput->S));
}

int pl_get2(union cell *s, union cell *c)
{
	struct stream *S = InputStream(s);
	return PL_unify_intg(c, Get(S));
}

int pl_peek_byte(union cell *c)
{
	return PL_unify_intg(c, Speekc(Finput->S));
}

int pl_peek_byte2(union cell *s, union cell *c)
{
	struct stream *S = InputStream(s);
	return PL_unify_intg(c, Speekc(S));
}

#define Skip(C,S,M)	\
	{ int __c, __r; \
	  if (!PL_get_intg(C,&__c)) \
	    PL_warning( M " : instantiation fault"); \
          __c&=0xff;	/* FIXME : test + msg if not ascii ??? */ \
	  while ((__r=Get0(S))!=__c && __r!=EOF) ; \
	}

int pl_skip(union cell *c)
{
	Skip(c, Finput->S, "skip/1");
	succeed;
}

int pl_skip2(union cell *s, union cell *c)
{
	struct stream *S = InputStream(s);
	Skip(c, S, "skip/2");
	succeed;
}

int pl_getline(union cell *line)
{
	static char buf[1024];
	int c;
	do
		c = Sgetc(Stdin);
	while (isSpace(c) || c == -1);
	buf[0] = c;
	if (!gets(buf + 1))
		buf[1] = '\0';

	return PL_unify_atom_chars(line, buf);
}

// FIXME : at_end_of_stream

int pl_stream_position(union cell *s, union cell *old, union cell *new)
{
	long o_char_no, char_no, line_no, col_no;
	long told;
	struct stream *S;
	union cell *pos;
	struct stream_pos *p;

	S = IOStream(s);		// FIXME : check return value
	told = Stell(S);

	if ((p = Sget_pos(S))) {
		char_no = p->char_no;
		line_no = p->line_no;
		col_no = p->col_no;
		if (told != char_no)	// --> FIXME : problem in S_updatepos ???
			fprintf(stderr,
				"\n[ Warning : position of stream don't correspond with internal position ] : told = %ld, char_no = %ld\n",
				told, char_no);

		pos = HP;
		HP[0].val = __fun(FUN(str_pos, 3));
		HP[1].val = __intg(char_no);
		HP[2].val = __intg(line_no);
		HP[3].val = __intg(col_no);
		HP += 4;
	} else {
		if (told < 0)
			PL_warning("Stream doesn't maintain position");
		char_no = told;

		pos = HP;
		HP[0].val = __fun(FUN(str_pos, 3));
		HP[1].val = __intg(char_no);
#if 1
		HP[2].val = __var();
		HP[3].val = __var();
#else
		HP[2].val = __intg(0);
		HP[3].val = __intg(0);
#endif
		HP += 4;
	}
	if (!pl_unify(old, pos))
		fail;

	new = deref(new);
	if (new == deref(old))
		succeed;
	o_char_no = char_no;
	if (!isfun(FUN(str_pos, 3), new) ||
	    !PL_get_long(new + 1, &char_no) ||
	    !PL_get_long(new + 2, &line_no) || !PL_get_long(new + 3, &col_no)) {
//     if (!isfun(FUN(str_pos,3),new))
//       fprintf(stderr, "functor is not '$stream_position'\n");
//     if (!PL_get_long(new+1, &char_no))
//       fprintf(stderr, "char_no is not a long\n");
//     if (!PL_get_long(new+2, &line_no))
//       fprintf(stderr, "line_no is not a long\n");
//     if (!PL_get_long(new+3, &col_no))
//       fprintf(stderr, "col_no is not a long\n");
//     pl_report(new);
		PL_warning("stream_position/3: Invalid position specifier");
	}

	if (char_no != o_char_no && Sseek(S, char_no, SF_SEEK_SET) < 0)
		PL_warning("Failed to set stream position");

	if (p) {
		p->char_no = char_no;
		p->line_no = line_no;
		p->col_no = col_no;
	}
	succeed;
}

int pl_set_stream_position(union cell *s, union cell *new)
{
	long char_no, line_no, col_no;
	struct stream *S;
	struct stream_pos *p;

	S = IOStream(s);		// FIXME : check return value
	Stell(S);
	p = Sget_pos(S);

	new = deref(new);

	if (!isfun(FUN(str_pos, 3), new) ||
	    !PL_get_long(new + 1, &char_no) ||
	    !PL_get_long(new + 2, &line_no) || !PL_get_long(new + 3, &col_no)) {
		PL_warning("stream_position/3: Invalid position specifier");
	}

	if (Sseek(S, char_no, SF_SEEK_SET) < 0) {
		PL_warning("Failed to set stream position");
	}

	if (p) {
		p->char_no = char_no;
		p->line_no = line_no;
		p->col_no = col_no;
	}
	succeed;
}

int pl_line_count(union cell *s, union cell *cnt)
{
	struct stream *S = IOStream(s);
	struct stream_pos *p;

	if ((p = Sget_pos(S)))
		return PL_unify_intg(cnt, p->line_no);
	else
		PL_warning("Stream doesn't maintain position");
}

int pl_line_position(union cell *s, union cell *cnt)
{
	struct stream *S = IOStream(s);
	struct stream_pos *p;

	if ((p = Sget_pos(S)))
		return PL_unify_intg(cnt, p->col_no);
	else
		PL_warning("Stream doesn't maintain position");
}

int pl_character_count(union cell *s, union cell *cnt)
{
	struct stream *S = IOStream(s);
	struct stream_pos *p;

	if ((p = Sget_pos(S)))
		return PL_unify_intg(cnt, p->char_no);
	else
		PL_warning("Stream doesn't maintain position");
}

/**********************************************************************/

enum stream_property {
	file_name,
	mode,
	in_out,
	alias,
	position,
	eof,
	action,
	repos,
	type,

	last_prop
};

static union cell *GetProp(int n, enum stream_property p)
{
	union cell *t = 0;

	switch (p) {
		struct atom *a;

	case file_name:
		if (StreamType(plfiles[n].S) != ST_FILE || !plfiles[n].file)
			break;
		t = HP;
		HP[0].val = __fun(FUN(_file__name, 1));
		HP[1].val = __atom(plfiles[n].file);
		HP += 2;
		break;

	case mode:
		switch (StreamMode(plfiles[n].S)) {
		case SM_READ:
			a = ATOM(_read);
			break;
		case SM_WRITE:
			a = ATOM(_write);
			break;
		case SM_APPEND:
			a = ATOM(_append);
			break;
		case SM_UPDATE:
			a = ATOM(_update);
			break;
		default:
			return 0;	// impossible error
		}
		t = HP;
		HP[0].val = __fun(FUN(_mode, 1));
		HP[1].val = __atom(a);
		HP += 2;
		break;

	case in_out:
		if (StreamMode(plfiles[n].S) == SM_READ)
			t = new_atom(ATOM(_input));
		else
			t = new_atom(ATOM(_output));
		break;

	case alias:			// FIXME : a stream can have several aliases
		if (!plfiles[n].alias)
			break;
		t = HP;
		HP[0].val = __fun(FUN(_alias, 1));
		HP[1].val = __atom(plfiles[n].alias);
		HP += 2;
		break;

	case position:
		{
			struct stream_pos *pos;

			if (!(pos = Sget_pos(plfiles[n].S)))
				break;
			t = HP;
			HP[0].val = __fun(FUN(_position, 1));
			HP[1].val = __fun(FUN(str_pos, 3));
			HP[2].val = __intg(pos->char_no);
			HP[3].val = __intg(pos->line_no);
			HP[4].val = __intg(pos->col_no);
			HP += 5;
			break;
		}

	case eof:
		{
			Sflag_t flags = StreamFlags(plfiles[n].S);

			t = HP;
			HP[0].val = __fun(FUN(_end__of__stream, 1));
			if (flags & SF_EOF2)
				HP[1].val = __atom(ATOM(_past));
			else if (flags & SF_EOF)
				HP[1].val = __atom(ATOM(_at));
			else
				HP[1].val = __atom(ATOM(_not));
			HP += 2;
			break;
		}

	case action:
		{
			Sflag_t flags = StreamFlags(plfiles[n].S);

			t = HP;
			HP[0].val = __fun(FUN(_eof__action, 1));
			if (flags & SF_EOF_ERR)
				HP[1].val = __atom(ATOM(_error));
			else if (flags & SF_EOF_RESET)
				HP[1].val = __atom(ATOM(_reset));
			else
				HP[1].val = __atom(ATOM(_eof__code));
			HP += 2;
			break;
		}

	case repos:
		{
			struct stream_pos *pos = Sget_pos(plfiles[n].S);

			t = HP;
			HP[0].val = __fun(FUN(_reposition, 1));
			if (pos)
				HP[1].val = __atom(ATOM(_true));
			else
				HP[1].val = __atom(ATOM(_false));
			HP += 2;
			break;
		}

	case type:
		{
			Sflag_t flag = StreamFlags(plfiles[n].S);

			t = HP;
			HP[0].val = __fun(FUN(_type, 1));
			if (flag & SF_BINARY)
				HP[1].val = __atom(ATOM(_binary));
			else
				HP[1].val = __atom(ATOM(_text));
			HP += 2;
			break;
		}

	default:
		break;
	}

	return t;
}

int pl_stream_property(union cell *stream, union cell *prop, enum control *ctrl)
{
	enum type { str, pro, all };
	struct {
		int n;
		enum stream_property p;
		enum type type;
	}     *ctxt;
	struct pl_file *file;
	int n;
	enum stream_property p;

	Deref(stream);
	Deref(prop);

	switch (GetCtrl(ctrl)) {
	case FIRST_CALL:
		if (!is_var(stream)) {
			if ((file = GetStream(stream, SM_ANY))) {
				if (!(file->S))
					fail;
				ctxt = AllocCtxt(*ctxt);
				ctxt->n = n = file - plfiles;
				ctxt->p = p = 0;
				ctxt->type = pro;
				goto loop_pro;
			} else
				fail;
		} else			// stream is a var
		{
			ctxt = AllocCtxt(*ctxt);
			ctxt->n = n = 0;
			ctxt->p = p = 0;
			if (is_var(prop)) {
				ctxt->type = all;
				goto loop_all;
			} else {
				ctxt->type = str;
				goto loop_all;	// FIXME
			}
		}

	case NEXT_CALL:
		ctxt = GetCtxt(ctrl);
		n = ctxt->n;
		p = ctxt->p;
		switch (ctxt->type) {
		case pro:
			goto loop_pro;
		case str:
			goto loop_all;	// FIXME
		case all:
			goto loop_all;
		default:
			fail;		// Impossible error
		}
		break;
	default:
		fail;
	}
	fail;

loop_all:
	for (; n < max_files; n++) {
		struct mark m;
		union cell *t;

		if (!plfiles[n].S)
			continue;

		Mark(m);

		for (; p < last_prop; p++) {
			t = GetProp(n, p);
			if (!t)
				continue;

			if (!pl_unify(t, prop)) {
				Undo(m);
				continue;
			}

			if (ctxt->type == all) {
				ctxt->n = n;
				ctxt->p = p + 1;
			} else		// ctxt->type == str
			{
				ctxt->n = n + 1;
				ctxt->p = p;
			}
			PL_unify_intg(stream, n);	// always succeed
			retry;
		}
		p = 0;
	}
	fail;

loop_pro:
	for (; p < last_prop; p++) {
		struct mark m;
		union cell *t;

		Mark(m);
		t = GetProp(n, p);
		if (!t)
			continue;

		if (!pl_unify(t, prop)) {
			Undo(m);
			continue;
		}
		// ctxt->n=n;
		ctxt->p = p + 1;
		retry;
	}
	fail;
}
