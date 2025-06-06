/* Object size checking support macros.
   Copyright (C) 2004-2024 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

In addition to the permissions in the GNU General Public License, the
Free Software Foundation gives you unlimited permission to link the
compiled version of this file into combinations with other programs,
and to distribute those combinations without any restriction coming
from the use of this file.  (The General Public License restrictions
do apply in other respects; for example, they cover modification of
the file, and distribution when not linked into a combine
executable.)

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */


#ifndef _SSP_H
#define _SSP_H 1

#if _FORTIFY_SOURCE > 0 && __OPTIMIZE__ > 0 \
    && defined __GNUC__ \
    && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)) \
    && !defined __cplusplus
# if _FORTIFY_SOURCE == 1
#  define __SSP_FORTIFY_LEVEL 1
# elif _FORTIFY_SOURCE > 1
#  define __SSP_FORTIFY_LEVEL 2
# endif
#endif

#if __SSP_FORTIFY_LEVEL > 0
# include <stddef.h>
# define __ssp_bos(ptr) __builtin_object_size (ptr, __SSP_FORTIFY_LEVEL > 1)
# define __ssp_bos0(ptr) __builtin_object_size (ptr, 0)

# define __SSP_REDIRECT(name, proto, alias) \
  name proto __asm__ (__SSP_ASMNAME (#alias))
# define __SSP_ASMNAME(cname)  __SSP_ASMNAME2 (__USER_LABEL_PREFIX__, cname)
# define __SSP_ASMNAME2(prefix, cname) __SSP_ASMNAME3 (prefix) cname
# define __SSP_ASMNAME3(prefix) #prefix

# undef __SSP_HAVE_VSNPRINTF

extern void __chk_fail (void) __attribute__((__noreturn__));
#endif

#endif /* _SSP_H */
