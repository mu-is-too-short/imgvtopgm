/*
 * imgvtopnm.c
 *	Pilot Image Viewer to PGM/PBM converter.
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
MU_ID("$Mu: imgvtopgm/imgvtopnm.c 1.6 1999/05/09 08:15:49 $")

#include <stdlib.h>

#include <mine/ipdb.h>

/*
 * These tend to live in different places on different systems so I'll
 * just do it by hand and assume you have a proper libc.
 */
extern	int	optind;
extern	char	*optarg;
extern	int	getopt(int, char *const *, const char *);

static int
version(const char *me)
{
	printf("%s %s\n", me, VERSION);
	return EXIT_SUCCESS;
}

#define	OPTS	"hvprn:"
static char *usefmt    = "%s [-hv] [-{r|p}] [-n note] [in [out]]\n";
static char *explain[] = {
	"\tConvert a Pilot Image Viewer pdb file to a pgm or pbm file.",
	"\tGrayscale images are output in PGM format; monochrome images",
	"\tare output in PBM format.",
	"",
	"\t-h       Display this usage message and exit.",
	"\t-v       Display the version number and exit.",
	"\t-p       Force the output file to be in plain/text format.",
	"\t-r       Force the output file to be in raw/binary format.",
	"\t-n note  Write the note (if any) to note.",
	"",
	"\tA filename of \"-\" means the standard input or standard output.",
	NULL
};
static int
usage(const char *me, int ret)
{
	char	**s;

	printf(usefmt, me);
	for(s = &explain[0]; *s != NULL; ++s)
		printf("%s\n", *s);
	return ret;
}

static int
writeg16img(IPDB *pdb, int plain, FILE *fp)
{
	gray	*row;
	int	i;
	int	w = ipdb_width(pdb);
	int	h = ipdb_height(pdb);

	row = pgm_allocrow(w);
	pgm_writepgminit(fp, w, h, 255, plain);
	for(i = 0; i < h; ++i)
		pgm_writepgmrow(fp, ipdb_g16row(pdb, i, row), w, 255, plain);
	pgm_freerow(row);

	return 0;
}

static int
writegimg(IPDB *pdb, int plain, FILE *fp)
{
	gray	*row;
	int	i;
	int	w = ipdb_width(pdb);
	int	h = ipdb_height(pdb);

	row = pgm_allocrow(w);
	pgm_writepgminit(fp, w, h, 255, plain);
	for(i = 0; i < h; ++i)
		pgm_writepgmrow(fp, ipdb_grow(pdb, i, row), w, 255, plain);
	pgm_freerow(row);

	return 0;
}

static int
writemimg(IPDB *pdb, int plain, FILE *fp)
{
	bit	*row;
	int	i;
	int	w = ipdb_width(pdb);
	int	h = ipdb_height(pdb);

	row = pbm_allocrow(w);
	pbm_writepbminit(fp, w, h, plain);
	for(i = 0; i < h; ++i)
		pbm_writepbmrow(fp, ipdb_mrow(pdb, i, row), w, plain);
	pbm_freerow(row);

	return 0;
}

static int
writetxt(IPDB *pdb, char *name)
{
	char	*note = ipdb_text(pdb);
	FILE	*fp;

	if(name == NULL || note == NULL)
		return 0;
	if(strcmp(name, "-") == 0)
		fp = stdout;
	else if((fp = fopen(name, "w")) == NULL)
		return errno;
	fprintf(fp, "%s\n", note);
	if(fp != stdin)
		fclose(fp);
	return 0;
}

int
main(int argc, char **argv)
{
	char	*me, *note;
	int	o, status, plain;
	FILE	*in, *out;
	IPDB	*pdb = NULL;

	if((me = strrchr(*argv, '/')) == NULL)
		me = *argv;
	else
		++me;
	pgm_init(&argc, argv);
	plain = FALSE;
	note  = NULL;
	while((o = getopt(argc, argv, OPTS)) != EOF) {
		switch(o) {
		case 'h':	exit(usage(me, EXIT_SUCCESS));	break;
		case 'v':	exit(version(me));		break;
		case 'p':	plain = TRUE;			break;
		case 'r':	plain = FALSE;			break;
		case 'n':	note  = optarg;			break;
		default:	exit(usage(me, EXIT_FAILURE));	break;
		}
	}
	in  = argv[optind] == NULL ? stdin  : pm_openr(argv[optind++]);
	out = argv[optind] == NULL ? stdout : pm_openw(argv[optind]);

	if((pdb = ipdb_alloc(NULL)) == NULL)
		pm_error("%s.", ipdb_err(ENOMEM));
	if((status = ipdb_read(pdb, in)) != 0)
		pm_error("Image header read error: %s.", ipdb_err(status));
	if(ipdb_type(pdb) == IMG_MONO) {
		if((status = writemimg(pdb, plain, out)) != 0)
			pm_error("Could not write output: %s.",
				 ipdb_err(status));
	}
	else if(ipdb_type(pdb) == IMG_GRAY) {
		if((status = writegimg(pdb, plain, out)) != 0)
			pm_error("Could not write output: %s.",
				 ipdb_err(status));
	}
	else {
		if((status = writeg16img(pdb, plain, out)) != 0)
			pm_error("Could not write output: %s.",
				 ipdb_err(status));
	}
	if((status = writetxt(pdb, note)) != 0)
		pm_error("Could not write note: %s.", ipdb_err(status));
	pdb = ipdb_free(pdb);

	if(in != stdin)
		pm_close(in);
	if(out != stdout)
		pm_close(out);

	return EXIT_SUCCESS;
}
