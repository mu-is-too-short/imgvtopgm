/*
 * ipdb.h
 *	Image Viewer PDB file functions.
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
#ifndef	IMGVTOPGM_IPDB_H
#define	IMGVTOPGM_IPDB_H
MU_HID(imgvtopgm_ipdb_h, "$Mu: imgvtopgm/mine/ipdb.h 1.2 1999/05/09 08:02:08 $")

#include <stdio.h>
#include <errno.h>

#include <pgm.h>

/*
 * Extra error numbers, feed these (or errno values) to ipdb_err()
 * to get strings.
 */
#define	E_BADCOLORS	-1
#define	E_NOTIMAGE	-2
#define	E_IMAGETHERE	-3
#define	E_IMAGENOTTHERE	-4
#define	E_TEXTTHERE	-5
#define	E_NOTRECHDR	-6
#define	E_UNKNOWNRECHDR	-7
#define	E_TOOBIGG	-8
#define	E_TOOBIGM	-9
#define	E_LAST		-9

/*
 * The standard pdb header.
 */
typedef struct {
	char	name[32];		/* nul terminated	*/
	u2	flags;			/* 0			*/
	u2	version;		/* 0			*/
	u4	ctime;			/* mac time		*/
	u4	mtime;			/* mac time		*/
	u4	btime;			/* mac time		*/
	u4	mod_num;		/* 0			*/
	u4	app_info;		/* 0			*/
	u4	sort_info;		/* 0			*/
	u1	type[4];		/* vIMG			*/
	u1	id[4];			/* View			*/
	u4	uniq_seed;		/* 0			*/
	u4	next_rec;		/* 0			*/
	u2	num_recs;		/* 1			*/
} PDBHEAD;
#define	PDBHEAD_SIZE	(32 + 2*2 + 6*4 + 4 + 4 + 2*4 + 2)

/*
 * Between the pdb header and the image header we find some "mystery" bytes,
 * these are supposed to be eight byte record headers but sometimes there
 * are ten bytes.  Version zero files use eight bytes, version 1 files appear
 * to use ten bytes, files with attached notes (version 2?) use two sets of
 * eight bytes.  Note that this version isn't the same as the `version' field
 * in IMAGE, that version only indicates if the file is compressed or not.
 *
 * The first four bytes of each piece are a four byte offset to the start
 * of the corresponding image header or text record; the next three bytes
 * (40 6f 80) are some kind of magic (they are always the same); the next
 * byte is zero for image records and 1 for text records; any remaining
 * mystery bytes (zero or two) are always zero.
 */
typedef struct {
	u4	offset;		/* offset, from zero, to the image	*/
	u1	unknown[3];	/* 40 6f 80				*/
	u1	rec_type;	/* byte seven, TEXT_REC || IMG_REC	*/
	size_t	n_extra;	/* bytes in extra			*/
	u1	*extra;		/* extra unknown end bytes		*/
} RECHDR;
#define	IMG_REC		(u1)(0x00)
#define	TEXT_REC	(u1)(0x01)

/*
 * The image headers.
 */
typedef struct {
	RECHDR	*r;

	/*
	 * Whether the image was originally compressed.  Since compressed
	 * data can cross row boundaries we have to uncompress the whole
	 * thing during reads so `data' is always in the uncompressed
	 * (but packed) format.  I think we can just use the `version'
	 * field for this but a little extra paranoia is worth a couple
	 * of bytes. This is also set after a write to indicate if
	 * compression was used.
	 */
	int	compressed;

	/*
	 * The actual image header, this starts at `m->offset'.
	 */
	char	name[32];	/* nul terminated			*/
	u1	version;	/* 0 => uncompressed, 1 => compressed	*/
	u1	type;		/* GRAYSCALE || MONOCHROME		*/
	u1	reserved1[4];	/* zero					*/
	u1	note[4];	/* zero					*/
	u2	x_last;		/* zero					*/
	u2	y_last;		/* zero					*/
	u1	reserved2[4];	/* zero					*/
	u2	x_anchor;	/* 0xffff				*/
	u2	y_anchor;	/* 0xffff				*/
	u2	width;		/* pixels (must be 0 mod 16)		*/
	u2	height;		/* pixels				*/

	/*
	 * And finally, the actual image data.  We always store the
	 * image data as 4 pixels per byte uncompressed.  Any compression
	 * or decompression is done at I/O time.
	 */
	u1	*data;
} IMAGE;
#define	IMAGESIZE	(32 + 1 + 1 + 4 + 4 + 2*2 + 4 + 2*2 + 2*2)

/*
 * Image types for IMAGE.type.
 */
#define	IMG_GRAY16	((u1)2)
#define	IMG_GRAY	((u1)0)
#define	IMG_MONO	((u1)0xff)

/*
 * Compression constants for IMAGE.version.
 */
#define	IMG_COMPRESSED		((u1)0x01)
#define	IMG_UNCOMPRESSED	((u1)0x00)

/*
 * The notes record.  If this exists, it will follow the image record.
 */
typedef struct {
	RECHDR	*r;
	char	*data;		/* the actual text as a normal string	*/
} TEXT;

/*
 * One PDB file.  The `t' field will be NULL if there is no note.
 */
typedef struct {
	PDBHEAD	*p;
	IMAGE	*i;
	TEXT	*t;
} IPDB;

/*
 * Flags for ipdb_write().
 */
#define	IPDB_COMPMAYBE	0		/* compress if it does any good	*/
#define	IPDB_NOCOMPRESS	(1 << 1)	/* don't compress		*/
#define	IPDB_COMPRESS	(1 << 2)	/* compress			*/

#define	ipdb_width(pdb)		((pdb)->i->width)
#define	ipdb_height(pdb)	((pdb)->i->height)
#define	ipdb_text(pdb)		((pdb)->t == NULL ? NULL : (pdb)->t->data)
#define	ipdb_compressed(pdb)	((pdb)->i->compressed)
#define	ipdb_ctime(pdb)		((time_t)((pdb)->p->ctime - UNIXEPOCH))
#define	ipdb_mtime(pdb)		((time_t)((pdb)->p->mtime - UNIXEPOCH))
#define	ipdb_btime(pdb)		((time_t)((pdb)->p->btime - UNIXEPOCH))
#define	ipdb_iname(pdb)		((pdb)->i->name)
#define	ipdb_pname(pdb)		((pdb)->p->name)
#define	ipdb_version(pdb)	((pdb)->i->version)
#define	ipdb_type(pdb)		((pdb)->i->type)
#define	ipdb_xlast(pdb)		((pdb)->i->x_last)
#define	ipdb_ylast(pdb)		((pdb)->i->y_last)
#define	ipdb_xanchor(pdb)	((pdb)->i->x_anchor)
#define	ipdb_yanchor(pdb)	((pdb)->i->y_anchor)

CDECLS_BEGIN

extern	char	*ipdb_err(int error);
extern	int	 ipdb_read(IPDB *, FILE *);
extern	int	 ipdb_write(IPDB *, int, FILE *);
extern	int	 ipdb_insert_g16image(IPDB *, int, int, const gray *);
extern	int	 ipdb_insert_gimage(IPDB *, int, int, const gray *);
extern	int	 ipdb_insert_mimage(IPDB *, int, int, const bit *);
extern	int	 ipdb_insert_text(IPDB *, const char *);
extern	int	 ipdb_remove_image(IPDB *);
extern	int	 ipdb_remove_text(IPDB *);
extern	gray	*ipdb_g16row(IPDB *, int, gray *);
extern	gray	*ipdb_grow(IPDB *, int, gray *);
extern	bit	*ipdb_mrow(IPDB *, int, bit *);
extern	IPDB	*ipdb_free(IPDB *);
extern	IPDB	*ipdb_alloc(const char *);

CDECLS_END

#endif
