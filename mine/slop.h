/*
 * slop.h
 *	Misc. slop.
 *
 * Copyright (C) 1996  Eric A. Howe
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Authors:  Eric A. Howe (mu@trends.net)
 */
#ifndef	IMGVTOPGM_SLOP_H
#define	IMGVTOPGM_SLOP_H

/*
 * A tricky way to get a string in a compiled file without polluting the
 * global name space.  The little mess will also keep 'gcc -Wall' quiet
 * (thus allowing you to use '-Werror' to be a bastard _and_ get your
 * version strings in the compiled file).  Note that we prefix a "@(#)"
 * on the version strings so that what(1) (from SCCS) can find the
 * version strings too.  Yes, this takes an extra four bytes (!!!!!)
 * to fool the compiler but who cares about four stupid bytes when you're
 * already imbedding a whole goddamn useless string?
 *
 * You can also get away with casting things to void with gcc but I don't
 * know what other compilers think of that so I'll just stick with this
 * trick (which fools all of the compilers that I have used).
 *
 * Use MU_ID() for C files and MU_HID() for headers.
 */
#define	MU_IDBASE(s,x) \
		static char	Mu_iD_##s[]     = "@(#)" x; \
		static int	Mu_iD_trick_##s = sizeof(Mu_iD_trick_##s) \
						+ sizeof(Mu_iD_##s);
#if defined(NORCS)
#	define	MU_ID(x)
#else
#	define	MU_ID(x)	MU_IDBASE(SoUrCeFiLe, x)
#endif

/*
 * Almost never want these but they can be handy when you're debugging things.
 */
#if !defined(DEBUG)
#	define	NOHRCS
#endif
#if defined(NOHRCS)
#	define	MU_HID(s, x)
#else
#	define	MU_HID(s, x)	MU_IDBASE(s, x)
#endif

MU_HID(imgvtopgm_slop_h, "$Mu: imgvtopgm/mine/slop.h 1.1 1998/12/16 18:25:12 $")

/*
 * Types of known size.  These should work on 32 and 64 bit unix machines
 * (`int' is still 32 bits on the Alpha, I don't know about the UltraSPARC but
 * I'd assume it is the same).
 */
typedef unsigned char	u1;
typedef unsigned short	u2;
typedef unsigned int	u4;

/*
 * Pilot time is Mac time (seconds since 00:00 UTC 1904.01.01)
 */
typedef u4 pilot_time_t;

/*
 * The unix epoch in Mac time (the Mac epoch is 00:00 UTC 1904.01.01).
 * The 17 is the number of leap years.
 */
#define	UNIXEPOCH	(pilot_time_t)((66*365+17)*24*3600)

#define	TRUE	1
#define	FALSE	0

#if defined(__cplusplus)
#	define	CDECLS_BEGIN	extern "C" {
#	define	CDECLS_END	};
#else
#	define	CDECLS_BEGIN
#	define	CDECLS_END
#endif

#endif
