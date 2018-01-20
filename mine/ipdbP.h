/*
 * ipdbP.h
 *	Private things for ipdb*.c.
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
#ifndef	IMGVTOPGM_IPDBP_H
#define	IMGVTOPGM_IPDBP_H
MU_HID(imgvtopgm_ipdbP_h, "$Mu: imgvtopgm/mine/ipdbP.h 1.2 1999/05/09 08:02:22 $")

#include <mine/bs.h>

/*
 * Pixel setting macros.
 */
#define	setg16pixel(b,v,o)	((b) |= ((v) << (4 - 4*(o))))
#define	getg16pixel(b,o)	(((b) >> (4 - 4*(o))) & 0x0f)
#define	setgpixel(b,v,o)	((b) |= ((v) << (6 - 2*(o))))
#define	getgpixel(b,o)		(((b) >> (6 - 2*(o))) & 0x03)
#define	setmpixel(b,v,o)	((b) |= ((v) << (7 - (o))))
#define	getmpixel(b,o)		(((b) >> (7 - (o))) & 0x01)

/*
 * Pixels/byte.
 */
#define	img_ppb(i) (			\
	(i)->type == IMG_GRAY   ? 4 :	\
	(i)->type == IMG_GRAY16 ? 2 :	\
	8				\
)

/*
 * Size (in bytes) of an image's data.
 */
#define	img_size(i)	(size_t)((i)->width/img_ppb(i)*(i)->height)

/*
 * Return the start of row `r'.
 */
#define	img_row(i, r)	(&(i)->data[(r)*(i)->width/img_ppb(i)])

/*
 * Only use four bytes of these.
 */
#define	IPDB_vIMG	"vIMG"
#define	IPDB_View	"View"

/*
 * Only use three bytes of this.
 */
#define	IPDB_MYST	"\x40\x6f\x80"

CDECLS_BEGIN

extern	IPDB	*ipdb_clear(IPDB *);

extern	PDBHEAD	*pdbhead_alloc(const char *);
extern	PDBHEAD	*pdbhead_free(PDBHEAD *);

extern	RECHDR	*rechdr_alloc(int, u4);
extern	RECHDR	*rechdr_free(RECHDR *);

extern	IMAGE	*image_alloc(const char *, int, int, int);
extern	IMAGE	*image_free(IMAGE *);

extern	TEXT	*text_alloc(const char *);
extern	TEXT	*text_free(TEXT *);

CDECLS_END

#endif
