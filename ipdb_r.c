/*
 * ipdb_r.c
 *	Pilot Image Viewer PDB reading functions.
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
MU_ID("$Mu: imgvtopgm/ipdb_r.c 1.13 1999/05/09 08:00:47 $")

#include <mine/ipdb.h>
#include <mine/ipdbP.h>

/*
 * This will *always* free `buffer'.
 *
 * The compression scheme used is a simple RLE; the control codes,
 * CODE, are one byte and have the following meanings:
 *
 *	CODE >  0x80	Insert (CODE + 1 - 0x80) copies of the next byte.
 *	CODE <= 0x80	Insert the next (CODE + 1) literal bytes.
 *
 * Compressed pieces can (and do) cross row boundaries.
 */
static u1 *
decompress(u1 *buffer, int bytes)
{
	int	got, put;
	u1	*in, *out, *data;

	if((data = calloc(1, bytes)) == NULL) {
		free(buffer);
		return NULL;
	}
	in  = buffer;
	out = data;
	while(bytes > 0) {
		if(*in > 0x80) {
			put = *in++ + 1 - 0x80;
			memset(out, *in, put);
			got = 1;
		}
		else {
			put = *in++ + 1;
			memcpy(out, in, put);
			got = put;
		}
		in    += got;
		out   += put;
		bytes -= put;
	}
	free(buffer);
	return data;
}

#define	UNKNOWN_OFFSET	(u4)-1
static int
image_read_data(IMAGE *i, u4 end_offset, FILE *fp)
{
	size_t	data_size = 0;
	u1	*buffer;

	if(end_offset == UNKNOWN_OFFSET) {
		/*
		 * Read until EOF. Some of them have an extra zero byte
		 * dangling off the end.  I originally thought this was
		 * an empty note record (even though there was no record
		 * header for it); however, the release notes for Image
		 * Compression Manager 1.1 on http://www.pilotgear.com
		 * note this extra byte as a bug in Image Compression
		 * Manager 1.0 which 1.1 fixes.  We'll just blindly read
		 * this extra byte and ignore it by paying attention to
		 * the image dimensions.
		 */
		if((buffer = calloc(1, img_size(i))) == NULL)
			return ENOMEM;
		if((data_size = fread(buffer, 1, img_size(i), fp)) <= 0)
			return EIO;
	}
	else {
		/*
		 * Read to the indicated offset.
		 */
		data_size = end_offset - ftell(fp) + 1;
		if((buffer = calloc(1, data_size)) == NULL)
			return ENOMEM;
		if(fread(buffer, 1, data_size, fp) != data_size)
			return EIO;
	}

	/*
	 * Compressed data can cross row boundaries so we decompress
	 * the data here to avoid messiness in the row access functions.
	 */
	if(data_size != img_size(i)) {
		if((i->data = decompress(buffer, img_size(i))) == NULL)
			return ENOMEM;
		i->compressed = TRUE;
	}
	else {
		i->compressed = FALSE;
		i->data       = buffer;
	}

	return 0;
}

static int
image_read(IMAGE *i, u4 end_offset, FILE *fp)
{
	if(i == NULL)
		return 0;

	i->r->offset = (u4)ftell(fp);
	if(fread(    i->name,      1, 32, fp) != 32
	|| fread(   &i->version,   1,  1, fp) != 1
	|| fread(   &i->type,      1,  1, fp) != 1
	|| fread(    i->reserved1, 1,  4, fp) != 4
	|| fread(    i->note,      1,  4, fp) != 4
	|| fread_u2(&i->x_last,           fp) != 2
	|| fread_u2(&i->y_last,           fp) != 2
	|| fread(    i->reserved2, 1,  4, fp) != 4
	|| fread_u2(&i->x_anchor,         fp) != 2
	|| fread_u2(&i->y_anchor,         fp) != 2
	|| fread_u2(&i->width,            fp) != 2
	|| fread_u2(&i->height,           fp) != 2
	|| image_read_data(i, end_offset, fp) != 0)
		return EIO;
	return 0;
}

static int
text_read(TEXT *t, FILE *fp)
{
#	define	BUFSZ	128
	char	*s;
	char	buf[BUFSZ];
	int	used, alloced, len;

	if(t == NULL)
		return 0;

	t->r->offset = (u4)ftell(fp);

	/*
	 * What a pain in the ass!  Why the hell isn't there a length
	 * attached to the text record?  I suppose the designer wasn't
	 * concerned about non-seekable (i.e. pipes) input streams.
	 * Perhaps I'm being a little harsh, the lack of a length probably
	 * isn't much of an issue on the Pilot.
	 */
	used    =
	alloced = 0;
	s       = NULL;
	while((len = fread(buf, 1, sizeof(buf), fp)) != 0) {
		if(buf[len - 1] == '\0')
			--len;
		if(used + len > alloced) {
			char	*tmp;

			if((tmp = realloc(s, alloced += 2*BUFSZ)) == NULL) {
				free(s);
				return ENOMEM;
			}
			s = tmp;
		}
		memcpy(s + used, buf, len);
		used += len;
	}
	if((t->data = calloc(1, used + 1)) == NULL) {
		free(s);
		return ENOMEM;
	}
	memcpy(t->data, s, used);
	free(s);

	return 0;
#	undef	BUFSZ
}

static int
pdbhead_read(PDBHEAD *p, FILE *fp)
{
	if(fread(    p->name, 1, 32, fp) != 32
	|| fread_u2(&p->flags,       fp) !=  2
	|| fread_u2(&p->version,     fp) !=  2
	|| fread_u4(&p->ctime,       fp) !=  4
	|| fread_u4(&p->mtime,       fp) !=  4
	|| fread_u4(&p->btime,       fp) !=  4
	|| fread_u4(&p->mod_num,     fp) !=  4
	|| fread_u4(&p->app_info,    fp) !=  4
	|| fread_u4(&p->sort_info,   fp) !=  4
	|| fread(    p->type, 1, 4,  fp) !=  4
	|| fread(    p->id,   1, 4,  fp) !=  4
	|| fread_u4(&p->uniq_seed,   fp) !=  4
	|| fread_u4(&p->next_rec,    fp) !=  4
	|| fread_u2(&p->num_recs,    fp) !=  2)
		return EIO;
	if(memcmp(p->type, IPDB_vIMG, 4) != 0
	|| memcmp(p->id,   IPDB_View, 4) != 0)
		return E_NOTIMAGE;

	return 0;
}

static int
rechdr_read(RECHDR *r, FILE *fp)
{
	off_t	len;

	if(fread_u4(&r->offset, fp) != 4)
		return EIO;
	len = (off_t)r->offset - ftell(fp);
	switch(len) {
	case 4:
	case 12:
		/*
		 * Version zero (eight bytes of record header) or version
		 * two with a note (two chunks of eight record header bytes).
		 */
		if(fread(&r->unknown[0], 1, 3, fp) != 3
		|| fread(&r->rec_type,   1, 1, fp) != 1)
			return EIO;
		r->n_extra = 0;
		r->extra   = NULL;
		break;
	case 6:
		/*
		 * Version one (ten bytes of record header).
		 */
		if(fread(&r->unknown[0], 1, 3, fp) != 3
		|| fread(&r->rec_type,   1, 1, fp) != 1)
			return EIO;
		r->n_extra = 2;
		if((r->extra = malloc(r->n_extra)) == NULL)
			return ENOMEM;
		if(fread(r->extra, 1, r->n_extra, fp) != r->n_extra)
			return EIO;
		break;
	default:
		/*
		 * hmmm.... I'll assume this is the record header
		 * for a text record.
		 */
		if((fread(&r->unknown[0], 1, 3, fp) != 3)
		|| (fread(&r->rec_type,   1, 1, fp) != 1))
			return EIO;
		r->n_extra = 0;
		r->extra   = NULL;
		break;
	}

	if((r->rec_type != IMG_REC && r->rec_type != TEXT_REC)
	|| memcmp(r->unknown, IPDB_MYST, 3) != 0)
		return E_NOTRECHDR;

	return 0;
}

int
ipdb_read(IPDB *pdb, FILE *fp)
{
	int	status;
	u4	offset;

	ipdb_clear(pdb);

	if((pdb->p = pdbhead_alloc(NULL)) == NULL)
		return ENOMEM;
	if((status = pdbhead_read(pdb->p, fp)) != 0)
		return status;

	if((pdb->i = image_alloc(pdb->p->name, IMG_GRAY, 0, 0)) == NULL)
		return ENOMEM;
	if((status = rechdr_read(pdb->i->r, fp)) != 0)
		return status;

	if(pdb->p->num_recs > 1) {
		if((pdb->t = text_alloc(NULL)) == NULL)
			return ENOMEM;
		if((status = rechdr_read(pdb->t->r, fp)) != 0)
			return status;
	}

	offset = pdb->t == NULL ? UNKNOWN_OFFSET : pdb->t->r->offset - 1;
	if((status = image_read(pdb->i, offset, fp)) != 0)
		return status;
	if(pdb->t != NULL
	&& (status = text_read(pdb->t, fp)) != 0)
		return status;

	return 0;
}

static gray *
g16unpack(const u1 *p, gray *g, int w)
{
static	gray	pal[] = {0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88,
			 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00};
	u1	*seg;
	int	i;

	for(i = 0, seg = (u1 *)p; i < w; i += 2, ++seg) {
		g[i + 0] = pal[getg16pixel(*seg, 0)];
		g[i + 1] = pal[getg16pixel(*seg, 1)];
	}
	return g;
}

static gray *
gunpack(const u1 *p, gray *g, int w)
{
static	gray	pal[] = {0xff, 0xaa, 0x55, 0x00};
	u1	*seg;
	int	i;

	for(i = 0, seg = (u1 *)p; i < w; i += 4, ++seg) {
		g[i + 0] = pal[getgpixel(*seg, 0)];
		g[i + 1] = pal[getgpixel(*seg, 1)];
		g[i + 2] = pal[getgpixel(*seg, 2)];
		g[i + 3] = pal[getgpixel(*seg, 3)];
	}
	return g;
}

static u1 *
munpack(const u1 *p, bit *b, int w)
{
static	bit	pal[] = {0x00, 0x01};
	u1	*seg;
	int	i;

	for(i = 0, seg = (u1 *)p; i < w; i += 8, ++seg) {
		b[i + 0] = pal[getmpixel(*seg, 0)];
		b[i + 1] = pal[getmpixel(*seg, 1)];
		b[i + 2] = pal[getmpixel(*seg, 2)];
		b[i + 3] = pal[getmpixel(*seg, 3)];
		b[i + 4] = pal[getmpixel(*seg, 4)];
		b[i + 5] = pal[getmpixel(*seg, 5)];
		b[i + 6] = pal[getmpixel(*seg, 6)];
		b[i + 7] = pal[getmpixel(*seg, 7)];
	}
	return b;
}

gray *
ipdb_g16row(IPDB *pdb, int row, gray *buffer)
{
	return g16unpack(img_row(pdb->i, row), buffer, ipdb_width(pdb));
}

gray *
ipdb_grow(IPDB *pdb, int row, gray *buffer)
{
	return gunpack(img_row(pdb->i, row), buffer, ipdb_width(pdb));
}

bit *
ipdb_mrow(IPDB *pdb, int row, bit *buffer)
{
	return munpack(img_row(pdb->i, row), buffer, ipdb_width(pdb));
}

/*
 * There's no point in fiddling with pdb->t->r->offset here since we
 * never know what it really should be until write-time anyway.
 */
int
ipdb_remove_image(IPDB *pdb)
{
	if(pdb->i == NULL)
		return 0;
	pdb->i = image_free(pdb->i);
	--pdb->p->num_recs;
	return 0;
}

int
ipdb_remove_text(IPDB *pdb)
{
	if(pdb->t == NULL)
		return 0;
	pdb->t = text_free(pdb->t);
	if(pdb->i != NULL)
		pdb->i->r->offset -= 8;
	--pdb->p->num_recs;
	return 0;
}
