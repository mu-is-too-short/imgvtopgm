/*
 * ipdb_c.c
 *	Functions common to Pilot Image Viewer PDB reading and writing.
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
MU_ID("$Mu: imgvtopgm/ipdb_c.c 1.14 1998/12/16 18:25:11 $")

#include <time.h>

#include <mine/ipdb.h>
#include <mine/ipdbP.h>

static char *errors[] = {
	/* E_BADCOLORS		*/
	"Invalid palette, only {0x00, 0x55, 0xAA, 0xFF} allowed.",

	/* E_NOTIMAGE		*/
	"Not an image file.",

	/* E_IMAGETHERE		*/
	"Image record already present, logic error.",

	/* E_IMAGENOTTHERE	*/
	"Image record required before text record, logic error.",

	/* E_TEXTTHERE		*/
	"Text record already present, logic error.",

	/* E_NOTRECHDR		*/
	"Invalid record header encountered.",

	/* E_UNKNOWNRECHDR	*/
	"Unknown record header.",

	/* E_TOOBIGG		*/
	"Image too big, maximum size approx. 640*400 gray pixels.",

	/* E_TOOBIGM		*/
	"Image too big, maximum size approx. 640*800 monochrome pixels.",
};
char *
ipdb_err(int e)
{
	if(e < 0)
		return e >= E_LAST ? errors[-e - 1] : "unknown error";
	return strerror(e);
}

/**
 ** Death.
 **/

RECHDR *
rechdr_free(RECHDR *r)
{
	if(r == NULL)
		return r;
	free(r->extra);
	free(r);
	return NULL;
}

IMAGE *
image_free(IMAGE *i)
{
	if(i == NULL)
		return i;
	i->r = rechdr_free(i->r);
	free(i->data);
	free(i);
	return NULL;
}

TEXT *
text_free(TEXT *t)
{
	if(t == NULL)
		return t;
	t->r = rechdr_free(t->r);
	free(t->data);
	free(t);
	return NULL;
}

PDBHEAD *
pdbhead_free(PDBHEAD *p)
{
	free(p);
	return NULL;
}

IPDB *
ipdb_clear(IPDB *pdb)
{
	if(pdb == NULL)
		return pdb;
	pdb->i = image_free(pdb->i);
	pdb->t = text_free(pdb->t);
	pdb->p = pdbhead_free(pdb->p);
	return pdb;
}

IPDB *
ipdb_free(IPDB *pdb)
{
	free(ipdb_clear(pdb));
	return NULL;
}

/**
 ** Birth.
 **/

PDBHEAD *
pdbhead_alloc(const char *name)
{
	PDBHEAD	*p;

	if((p = calloc(1, sizeof(PDBHEAD))) == NULL)
		return NULL;

	strncpy(p->name, name == NULL ? "unnamed" : name, 31);

	/*
	 * All of the Image Viewer pdb files that I've come across have
	 * 3510939142U (1997.08.16 14:38:22 UTC) here.  I don't know where
	 * this bizarre date comes from but the real date works fine so
	 * I'm using it.
	 */
	p->ctime =
	p->mtime = (pilot_time_t)time(NULL) + UNIXEPOCH;

	memcpy(p->type, IPDB_vIMG, sizeof(p->type));
	memcpy(p->id,   IPDB_View, sizeof(p->id));

	return p;
}

/*
 * We never produce the `extra' bytes (we only read them from a file)
 * so there is no point allocating them here.
 */
RECHDR *
rechdr_alloc(int type, u4 offset)
{
	RECHDR	*r;

	if((r = calloc(1, sizeof(RECHDR))) == NULL)
		return r;
	r->offset   = offset;
	r->rec_type = (u1)(0xff & type);
	memcpy(r->unknown, IPDB_MYST, sizeof(r->unknown));

	return r;
}

/*
 * The offset will be patched up as needed elsewhere.
 */
#define	IMGOFFSET	(PDBHEAD_SIZE + 8)
IMAGE *
image_alloc(const char *name, int type, int w, int h)
{
	IMAGE	*i;

	if((i = calloc(1, sizeof(IMAGE))) == NULL
	|| (i->r = rechdr_alloc(IMG_REC, IMGOFFSET)) == NULL
	|| (w != 0 && h != 0 && (i->data = calloc(1, w*h)) == NULL))
		return image_free(i);
	strncpy(i->name, name == NULL ? "unnamed" : name, 31);
	i->type     = type;
	i->x_anchor =
	i->y_anchor = 0xffff;
	i->width    = w;
	i->height   = h;

	return i;
}

TEXT *
text_alloc(const char *s)
{
	TEXT	*t;

	/*
	 * The offset will be patched up later on when we know what it
	 * should be.
	 */
	if((t    = calloc(1, sizeof(TEXT))) == NULL
	|| (t->r = rechdr_alloc(TEXT_REC, 0)) == NULL)
		return text_free(t);
	if(s == NULL)
		return t;
	if((t->data = calloc(1, strlen(s) + 1)) == NULL)
		return text_free(t);
	strcpy(t->data, s);

	return t;
}

IPDB *
ipdb_alloc(const char *name)
{
	IPDB	*pdb;

	if((pdb = calloc(1, sizeof(IPDB))) == NULL)
		return ipdb_free(pdb);
	if(name == NULL)
		return pdb;
	if((pdb->p = pdbhead_alloc(name)) == NULL)
		return ipdb_free(pdb);
	return pdb;
}
