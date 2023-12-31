/* 
 * Copyright (c) 1995-1994 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 */

#include <stddef.h>
#include <string/string.h>

#include "malloc_internal.h"

/* XX could be made smarter, so it doesn't always copy to a new block.  */
void *_realloc(void *buf, size_t new_size)
{
	vm_size_t *op;
	vm_size_t old_size;
	vm_size_t *np;

	if (buf == 0)
		return _malloc(new_size);

	op = (vm_size_t*)buf;
	old_size = *--op;

	new_size += sizeof(vm_size_t);
	malloc_lmm_begin();
	np = lmm_alloc(&malloc_lmm, new_size, 0);
	malloc_lmm_end();
	if (!np)
	    return NULL;

	memcpy(np, op, old_size < new_size ? old_size : new_size);

	malloc_lmm_begin();
	lmm_free(&malloc_lmm, op, old_size);
	malloc_lmm_end();
	
	*np++ = new_size;
	return np;
}

