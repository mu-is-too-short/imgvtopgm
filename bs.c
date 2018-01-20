/*
 * bs.c
 *	Byte mangling functions for ipdb.c.
 *
 * Copyright (C) 1997 Eric A. Howe
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Authors:  Eric A. Howe (mu@trends.net)
 */
#include <mine/slop.h>
MU_ID("$Mu: imgvtopgm/bs.c 1.10 1998/12/16 18:25:11 $")

#include <mine/bs.h>

#if !defined(MU_BIGENDIAN) && !defined(MU_LITTLEENDIAN) && !defined(MU_UNKNOWNENDIAN)
#	define	MU_UNKNOWNENDIAN
#endif

/**
 ** Big endian.
 **/
#if defined(MU_BIGENDIAN) || defined(MU_UNKNOWNENDIAN)
	static u1 *
	flip2_big(u2 x, u1 *b)
	{
		b[0] = ((u1 *)&x)[0];
		b[1] = ((u1 *)&x)[1];
		return b;
	}

	static u2
	unflip2_big(u1 *b)
	{
		u2	x;

		((u1 *)&x)[0] = b[0];
		((u1 *)&x)[1] = b[1];
		return x;
	}

	static u1 *
	flip4_big(u4 x, u1 *b)
	{
		b[0] = ((u1 *)&x)[0];
		b[1] = ((u1 *)&x)[1];
		b[2] = ((u1 *)&x)[2];
		b[3] = ((u1 *)&x)[3];
		return b;
	}

	static u4
	unflip4_big(u1 *b)
	{
		u4	x;

		((u1 *)&x)[0] = b[0];
		((u1 *)&x)[1] = b[1];
		((u1 *)&x)[2] = b[2];
		((u1 *)&x)[3] = b[3];
		return x;
	}
#endif

/**
 ** Little endian.
 **/
#if defined(MU_LITTLEENDIAN) || defined(MU_UNKNOWNENDIAN)
	static u1 *
	flip2_little(u2 x, u1 *b)
	{
		b[0] = ((u1 *)&x)[1];
		b[1] = ((u1 *)&x)[0];
		return b;
	}

	static u2
	unflip2_little(u1 *b)
	{
		u2	x;

		((u1 *)&x)[0] = b[1];
		((u1 *)&x)[1] = b[0];
		return x;
	}

	static u1 *
	flip4_little(u4 x, u1 *b)
	{
		b[0] = ((u1 *)&x)[3];
		b[1] = ((u1 *)&x)[2];
		b[2] = ((u1 *)&x)[1];
		b[3] = ((u1 *)&x)[0];
		return b;
	}

	static u4
	unflip4_little(u1 *b)
	{
		u4	x;

		((u1 *)&x)[0] = b[3];
		((u1 *)&x)[1] = b[2];
		((u1 *)&x)[2] = b[1];
		((u1 *)&x)[3] = b[0];
		return x;
	}
#endif

#if defined(MU_UNKNOWNENDIAN)
	static u1 *flip2_unknown(u2, u1 *);
	static u1 *flip4_unknown(u4, u1 *);
	static u2  unflip2_unknown(u1 *);
	static u4  unflip4_unknown(u1 *);

	static u1 *(*flip2)(u2, u1 *) = flip2_unknown;
	static u1 *(*flip4)(u4, u1 *) = flip4_unknown;
	static u2  (*unflip2)(u1 *)   = unflip2_unknown;
	static u4  (*unflip4)(u1 *)   = unflip4_unknown;

	static void
	fix(void)
	{
		union {
			u1 c[4];
			u4 i;
		} u;
		u.i = 0x01020304;
		if(u.c[0] == 4
		&& u.c[1] == 3
		&& u.c[2] == 2
		&& u.c[3] == 1) {
			flip2   = flip2_little;
			unflip2 = unflip2_little;
			flip4   = flip4_little;
			unflip4 = unflip4_little;
		}
		else {
			flip2   = flip2_big;
			unflip2 = unflip2_big;
			flip4   = flip4_big;
			unflip4 = unflip4_big;
		}
	}

	static u1 *
	flip2_unknown(u2 x, u1 *b)
	{
		fix();
		return flip2(x, b);
	}

	static u2
	unflip2_unknown(u1 *b)
	{
		fix();
		return unflip2(b);
	}

	static u1 *
	flip4_unknown(u4 x, u1 *b)
	{
		fix();
		return flip4(x, b);
	}

	static u4
	unflip4_unknown(u1 *b)
	{
		fix();
		return unflip4(b);
	}
#endif

#if defined(MU_BIGENDIAN)
#	define	flip2	flip2_big
#	define	flip4	flip4_big
#	define	unflip2	unflip2_big
#	define	unflip4	unflip4_big
#elif defined(MU_LITTLEENDIAN)
#	define	flip2	flip2_little
#	define	flip4	flip4_little
#	define	unflip2	unflip2_little
#	define	unflip4	unflip4_little
#endif

size_t
fread_u2(u2 *x, FILE *fp)
{
	u1	b[2];
	size_t	len;

	if((len = fread(b, 1, 2, fp)) != 2)
		return len;
	*x = unflip2(b);
	return 2;
}

size_t
fread_u4(u4 *x, FILE *fp)
{
	u1	b[4];
	size_t	len;

	if((len = fread(b, 1, 4, fp)) != 4)
		return len;
	*x = unflip4(b);
	return 4;
}

size_t
fwrite_u2(u2 x, FILE *fp)
{
	u1	b[2];
	return fwrite(flip2(x, b), 1, 2, fp);
}

size_t
fwrite_u4(u4 x, FILE *fp)
{
	u1	b[4];
	return fwrite(flip4(x, b), 1, 4, fp);
}
