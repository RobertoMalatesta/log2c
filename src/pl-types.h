/****************************************************************/
/* Copyright (c) 1998 Luc Van Oostenryck. All rights reserved.	*/
/*								*/
/****************************************************************/

#ifndef PL_TYPES_H_
#define PL_TYPES_H_

#include <sys/types.h>

typedef unsigned long hash_t;
typedef long intg_t;
typedef double flt_t;


typedef enum { ref_tag, var_tag, ato_tag,
               fun_tag, int_tag, flt_tag
             } tag_t;

typedef enum { FAIL, SUCCEED, RETRY } foreign_t;
typedef	enum { FIRST_CALL, NEXT_CALL } ctrl_t, *control_t;


#ifdef	WORDS_BIGENDIAN

typedef	struct { tag_t tag:3;  int  val:29; } _val_t;
typedef struct { tag_t tag:3; uint uval:29; } uval_t;

#else	// WORDS_BIGENDIAN

typedef	struct {  int  val:29; tag_t tag:3; } _val_t;
typedef struct { uint uval:29; tag_t tag:3; } uval_t;

#endif	// WORDS_BIGENDIAN


typedef union  cell_t  cell_t, *term_t, *tr_t;
union cell_t { ulong   val;
	       _val_t  tag_val;
	       uval_t  tag_uval;
               cell_t *celp;
             };

typedef struct atom__t atom__t, *atom_t;
struct atom__t { cell_t  atom;
	         const char *name;
	         hash_t  hash;
	         atom_t  next;
               };

typedef struct fun__t fun__t, *fun_t, *functor_t;
struct fun__t  { atom_t  functor;
                 long    arity;
	         fun_t   next;
               };


typedef union pl_stack_t pl_stack_t;
union pl_stack_t { cell_t  *celp;
	  	   tr_t    *tr;
                   pl_stack_t *stk;
                   void    *cod;
                   long     intg;
		   cell_t   cell;
		   ctrl_t   ctrl;
                 };

typedef struct { tr_t   *trail;
                 cell_t *global;
               } mark_t;

typedef	void *predicate_t, *pred_t;

typedef struct jmp__t jmp__t;
struct jmp__t { atom_t  functor;
		long	arity;
	        void   *pred;
	        jmp__t *next;
              };
typedef struct  { jmp__t **tab;
		  int      size;
		} jmp_table;

typedef struct { const char *file;
		 atom_t      module;
                 jmp_table   pub;
                 jmp_table   all;
               } module_t;

typedef struct mods_t modules_t;
struct mods_t  { atom_t	     name;
                 module_t   *module;
                 modules_t  *next;
               };

typedef struct { int   type;		// int_tag | flt_tag
                 union { intg_t	intg;
                 	 flt_t  flt;
                 	 ulong  w[sizeof(double)/sizeof(ulong)];
                       } val;
               } pl_number_t;

typedef	int	bool;
#define	FALSE	0
#define	TRUE	1
#define	EOS	'\0'


#include <sys/time.h>

#ifndef TIME_OF_DAY
typedef struct
{ double utime;
  double stime;
} time__t;
#endif

#ifndef	STRUCT_PL_STREAM
#define STRUCT_PL_STREAM
typedef struct pl_stream_t pl_stream_t, *pl_stream;
#endif

// extern pl_stream Stdin, Stdout, Stderr;
// extern pl_stream_t Stdin__, Stdout__, Stderr__;

#endif	// PL_TYPES_H_
