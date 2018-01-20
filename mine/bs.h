/*
 * bs.h
 *	Byte order mangling functions for ipdb.c.
 *
 * Copyright (C) 1997 Eric A. Howe
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
 *   Authors: Eric A. Howe (mu@trends.net)
 */
#ifndef	IMGVTOPGM_BS_H
#define	IMGVTOPGM_BS_H
MU_HID(imgvtopgm_bs_h, "$Mu: imgvtopgm/mine/bs.h 1.1 1998/12/16 18:25:12 $")

#include <stdio.h>

CDECLS_BEGIN
extern	size_t	fread_u2(u2 *, FILE *);
extern	size_t	fread_u4(u4 *, FILE *);
extern	size_t	fwrite_u2(u2, FILE *);
extern	size_t	fwrite_u4(u4, FILE *);
CDECLS_END

#endif
