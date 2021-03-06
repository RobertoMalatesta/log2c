/************************************************************************/
/* Copyright (c) 1998-2014 Luc Van Oostenryck. All rights reserved.	*/
/*									*/
/* %@%LICENSE								*/
/*									*/
/************************************************************************/

#if	defined(__i386__)
#include "pl-cfg-i386.h"
#elif	defined(__x86_64__)
#include "pl-cfg-x86_64.h"
#endif

#ifndef PL_CONFIG_H__
#define PL_CONFIG_H__

//#define TIME_OF_DAY
//#define DEBUG_STACKS

#define	WORDS_LITTLEENDIAN
#define	CELL_SIZE	__SIZEOF_LONG__	// in bytes
#define	CELL_BITS	(CELL_SIZE*8)	// in bits

#define TAG_BITS	3
#define TAG_POS		(CELL_BITS-TAG_BITS)
#define TAG_MASK	(((1UL<<TAG_BITS)-1)<<TAG_POS)
#define VAL_MASK	(~TAG_MASK)
#define GC_BITS		2

#define PL_MAX_TAG_INT	(VAL_MASK>>1)
#define PL_MAX_INT	PL_MAX_TAG_INT
#define PL_MIN_TAG_INT	(-(PL_MAX_TAG_INT+1))
#define PL_MIN_INT	(-(PL_MAX_INT+1))

#define PL_MAX_RING_BUF	4

#define	PL_MAX_ARGS	256

// FIXME : should be done by configure & C°
#define PL_HOME		"/home/luc/log"
#define PL_VERSION	809
#define CC		"gcc"
#define C_LD_FLAGS	""
#define C_LIBS		""
#define C_STATIC_LIBS	""

#define HAVE_REMOVE
#define HAVE_REALPATH
#define HAVE_ASPRINTF
#define HAVE_PIPE

#endif
