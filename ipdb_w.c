/*
 * ipdb_w.c
 *	Pilot Image Viewer PDB writing functions.
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
MU_ID("$Mu: imgvtopgm/ipdb_w.c 1.19 1999/05/09 08:01:47 $")

#include <mine/ipdb.h>
#include <mine/ipdbP.h>

static int
pdbhead_write(PDBHEAD *p, FILE *fp)
{
	if(fwrite(   p->name, 1, 32, fp) != 32
	|| fwrite_u2(p->flags,       fp) !=  2
	|| fwrite_u2(p->version,     fp) !=  2
	|| fwrite_u4(p->ctime,       fp) !=  4
	|| fwrite_u4(p->mtime,       fp) !=  4
	|| fwrite_u4(p->btime,       fp) !=  4
	|| fwrite_u4(p->mod_num,     fp) !=  4
	|| fwrite_u4(p->app_info,    fp) !=  4
	|| fwrite_u4(p->sort_info,   fp) !=  4
	|| fwrite(   p->type, 1, 4,  fp) !=  4
	|| fwrite(   p->id,   1, 4,  fp) !=  4
	|| fwrite_u4(p->uniq_seed,   fp) !=  4
	|| fwrite_u4(p->next_rec,    fp) !=  4
	|| fwrite_u2(p->num_recs,    fp) !=  2)
		return EIO;
	return 0;
}

static int
rechdr_write(RECHDR *r, FILE *fp)
{
	if(r == NULL)
		return 0;
	if(fwrite_u4(r->offset,       fp) != 4
	|| fwrite(r->unknown,   1, 3, fp) != 3
	|| fwrite(&r->rec_type, 1, 1, fp) != 1)
		return EIO;
	if(r->n_extra != 0
	&& fwrite(r->extra, 1, r->n_extra, fp) != r->n_extra)
		return EIO;

	return 0;
}

static int
image_write(IMAGE *i, u1 *data, size_t n, FILE *fp)
{
	if(fwrite(   i->name,      1, 32, fp) != 32
	|| fwrite(  &i->version,   1,  1, fp) != 1
	|| fwrite(  &i->type,      1,  1, fp) != 1
	|| fwrite(   i->reserved1, 1,  4, fp) != 4
	|| fwrite(   i->note,      1,  4, fp) != 4
	|| fwrite_u2(i->x_last,           fp) != 2
	|| fwrite_u2(i->y_last,           fp) != 2
	|| fwrite(   i->reserved2, 1,  4, fp) != 4
	|| fwrite_u2(i->x_anchor,         fp) != 2
	|| fwrite_u2(i->y_anchor,         fp) != 2
	|| fwrite_u2(i->width,            fp) != 2
	|| fwrite_u2(i->height,           fp) != 2
	|| fwrite(   data,         1,  n, fp) != n)
		return EIO;
	return 0;
}

static int
text_write(TEXT *t, FILE *fp)
{
	size_t	len;

	if(t == NULL)
		return 0;
	len = strlen(t->data) + 1;
	if(fwrite(t->data, 1, len, fp) != len)
		return EIO;
	return 0;
}

typedef struct {
	unsigned	match;
	u1		buf[128];
	int		mode;
	size_t		len;
	size_t		used;
	u1		*p;
} RLE;
#define	MODE_MATCH	0
#define	MODE_LIT	1
#define	MODE_NONE	2

#define	reset(r) {		\
	(r)->match = 0xffff;	\
	(r)->mode  = MODE_NONE;	\
	(r)->len   = 0;		\
}

static void
put_match(RLE *r, size_t n)
{
	*r->p++  = 0x80 + n - 1;
	*r->p++  = r->match;
	r->used += 2;
	reset(r);
}

static void
put_lit(RLE *r, size_t n)
{
	*r->p++  = n - 1;
	r->p     = (u1 *)memcpy(r->p, r->buf, n) + n;
	r->used += n + 1;
	reset(r);
}

static size_t
compress(u1 *in, size_t n_in, u1 *out)
{
static	void (*put[])(RLE *, size_t) = {put_match, put_lit};
	RLE	r;
	size_t	i;

	memset((void *)&r, '\0', sizeof(r));
	r.p = out;
	reset(&r);

	for(i = 0; i < n_in; ++i, ++in) {
		if(*in == r.match) {
			if(r.mode == MODE_LIT && r.len > 1) {
				put_lit(&r, r.len - 1);
				++r.len;
				r.match = *in;
			}
			r.mode = MODE_MATCH;
			++r.len;
		}
		else {
			if(r.mode == MODE_MATCH)
				put_match(&r, r.len);
			r.mode         = MODE_LIT;
			r.match        = *in;
			r.buf[r.len++] = *in;
		}
		if(r.len == 128)
			put[r.mode](&r, r.len);
	}
	if(r.len != 0)
		put[r.mode](&r, r.len);

	return r.used;
}

int
ipdb_write(IPDB *pdb, int comp, FILE *fp)
{
	int	status;
	int	n;
	u1	*data;
	RECHDR	*ir, *tr;

	if(pdb->i == NULL)
		return E_IMAGENOTTHERE;

	n    = img_size(pdb->i);
	data = pdb->i->data;
	ir   = pdb->i->r;
	tr   = pdb->t == NULL ? NULL : pdb->t->r;

	if(comp != IPDB_NOCOMPRESS) {
		/*
		 * Allocate for the worst case.
		 */
		int	sz = (3*n + 2)/2;
		if((data = malloc(sz)) == NULL)
			return ENOMEM;
		sz = compress(pdb->i->data, n, data);
		if(comp == IPDB_COMPMAYBE && sz >= n) {
			free(data);
			data = pdb->i->data;
		}
		else {
			pdb->i->compressed = TRUE;
			if (pdb->i->type == IMG_GRAY16)
				pdb->i->version    = 9;
			else
				pdb->i->version    = 1;
			if(pdb->t != NULL)
				pdb->t->r->offset -= n - sz;
			n = sz;
		}
	}

	/*
	 * Cool, I've found a non-trivial use for casting something
	 * to `void'.
	 */
	(void)((status = pdbhead_write(pdb->p, fp)) != 0
	    || (status = rechdr_write(ir, fp)) != 0
	    || (status = rechdr_write(tr, fp)) != 0
	    || (status = image_write(pdb->i, data, n, fp)) != 0
	    || (status = text_write(pdb->t, fp)) != 0);
	if(data != pdb->i->data)
		free(data);
	return status;
}

static int
g16pack(const gray *g, u1 *p, int w)
{
	int	off, i;
	u1	*seg;

	for(i = off = 0, seg = p; i < w; ++i, ++g) {
		switch(*g) {
		case 0xff:	setg16pixel(*seg, 0x00, off);	break;
		case 0xee:	setg16pixel(*seg, 0x01, off);	break;
		case 0xdd:	setg16pixel(*seg, 0x02, off);	break;
		case 0xcc:	setg16pixel(*seg, 0x03, off);	break;
		case 0xbb:	setg16pixel(*seg, 0x04, off);	break;
		case 0xaa:	setg16pixel(*seg, 0x05, off);	break;
		case 0x99:	setg16pixel(*seg, 0x06, off);	break;
		case 0x88:	setg16pixel(*seg, 0x07, off);	break;
		case 0x77:	setg16pixel(*seg, 0x08, off);	break;
		case 0x66:	setg16pixel(*seg, 0x09, off);	break;
		case 0x55:	setg16pixel(*seg, 0x0a, off);	break;
		case 0x44:	setg16pixel(*seg, 0x0b, off);	break;
		case 0x33:	setg16pixel(*seg, 0x0c, off);	break;
		case 0x22:	setg16pixel(*seg, 0x0d, off);	break;
		case 0x11:	setg16pixel(*seg, 0x0e, off);	break;
		case 0x00:	setg16pixel(*seg, 0x0f, off);	break;
		default:	return E_BADCOLORS;
		}
		if(++off == 2)
			++seg, off = 0;
	}
	return w/2;
}

static int
gpack(const gray *g, u1 *p, int w)
{
	int	off, i;
	u1	*seg;

	for(i = off = 0, seg = p; i < w; ++i, ++g) {
		switch(*g) {
		case 0xff:	setgpixel(*seg, 0x00, off);	break;
		case 0xaa:	setgpixel(*seg, 0x01, off);	break;
		case 0x55:	setgpixel(*seg, 0x02, off);	break;
		case 0x00:	setgpixel(*seg, 0x03, off);	break;
		default:	return E_BADCOLORS;
		}
		if(++off == 4)
			++seg, off = 0;
	}
	return w/4;
}

static int
mpack(const bit *b, u1 *p, int w)
{
	int	off, i;
	u1	*seg;

	for(i = off = 0, seg = p; i < w; ++i, ++b) {
		setmpixel(*seg, *b != 0, off);
		if(++off == 8)
			++seg, off = 0;
	}

	return w/8;
}

static int
adjust_dims(int w, int h, int *aw, int *ah)
{
	*aw = w;
	*ah = h;
	if(*aw % 16 != 0)
		*aw += 16 - (*aw % 16);
	if(*aw < 160)
		*aw = 160;
	if(*ah < 160)
		*ah = 160;
	return w == *aw && h == *ah;
}

/*
 * You can only allocate 64k chunks of memory on the pilot and that
 * supplies an image size limit.
 */
#define	MAX_SIZE(t)	((1 << 16)*((t) == IMG_GRAY ? 4 : 8))
#define	MAX_ERROR(t)	((t) == IMG_GRAY ? E_TOOBIGG : E_TOOBIGM)

static int
image_insert_init(IPDB *pdb, int uw, int uh, int type)
{
	char	*name = pdb->p->name;
	int	w, h;

	if(pdb->p->num_recs != 0)
		return E_IMAGETHERE;

	adjust_dims(uw, uh, &w, &h);
	if(w*h > MAX_SIZE(type))
		return MAX_ERROR(type);
	if((pdb->i = image_alloc(name, type, w, h)) == NULL)
		return ENOMEM;
	pdb->p->num_recs = 1;

	return 0;
}

int
ipdb_insert_g16image(IPDB *pdb, int w, int h, const gray *g)
{
	int	i, len;
	int	incr;
	u1	*p;

	if((i = image_insert_init(pdb, w, h, IMG_GRAY16)) != 0)
		return i;
	incr = ipdb_width(pdb)/2;
	for(i = 0, p = pdb->i->data; i < h; ++i, p += incr, g += w)
		if((len = g16pack(g, p, w)) < 0)
			return len;
	return 0;
}

int
ipdb_insert_gimage(IPDB *pdb, int w, int h, const gray *g)
{
	int	i, len;
	int	incr;
	u1	*p;

	if((i = image_insert_init(pdb, w, h, IMG_GRAY)) != 0)
		return i;
	incr = ipdb_width(pdb)/4;
	for(i = 0, p = pdb->i->data; i < h; ++i, p += incr, g += w)
		if((len = gpack(g, p, w)) < 0)
			return len;
	return 0;
}

int
ipdb_insert_mimage(IPDB *pdb, int w, int h, const bit *b)
{
	int	i, len;
	int	incr;
	u1	*p;

	if((i = image_insert_init(pdb, w, h, IMG_MONO)) != 0)
		return i;
	incr = ipdb_width(pdb)/8;
	for(i = 0, p = pdb->i->data; i < h; ++i, p += incr, b += w)
		if((len = mpack(b, p, w)) < 0)
			return len;
	return 0;
}

int
ipdb_insert_text(IPDB *pdb, const char *s)
{
	if(pdb->i == NULL)
		return E_IMAGENOTTHERE;
	if(pdb->p->num_recs == 2)
		return E_TEXTTHERE;

	if((pdb->t = text_alloc(s)) == NULL)
		return ENOMEM;
	pdb->p->num_recs = 2;

	pdb->i->r->offset += 8;
	pdb->t->r->offset  = pdb->i->r->offset + IMAGESIZE + img_size(pdb->i);

	return 0;
}
