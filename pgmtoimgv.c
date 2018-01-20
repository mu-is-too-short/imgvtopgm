/*
 * pgmtoimgv.c
 *	PGM to Pilot Image Viewer pdb converter.
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
MU_ID("$Mu: imgvtopgm/pgmtoimgv.c 1.28 1999/05/09 08:27:51 $")

#include <stdlib.h>
#include <sys/stat.h>

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

#define	OPTS	"4hvt:n:cmu"
static char *usefmt = "%s [-hv] [-n note] [-t title] [-{c|m|u}] [in [out]]\n";
static char *explain[] = {
	"\tConvert a pgm file to a Pilot Image Viewer pdb file.",
	"",
	"\t-4        4 depth bitmap.",
	"\t-h        Display this usage message and exit.",
	"\t-v        Display the version number and exit.",
	"\t-c        Produce a compressed image.",
	"\t-m        Produce a compressed image only if it is smaller.",
	"\t-u        Produce an uncompressed image.",
	"\t-t title  Specify the pdb name, the default is unnamed.",
	"\t-n note   Read the image note from note.",
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
readimg(IPDB *pdb, FILE *fp, int depth)
{
	gray	*g = NULL;
	gray	maxval;
	int	w, h, fmt;
	int	status, i;

	pgm_readpgminit(fp, &w, &h, &maxval, &fmt);
	if((g = calloc(1, w*h*sizeof(gray))) == NULL)
		return ENOMEM;
	for(i = 0; i < h; ++i)
		pgm_readpgmrow(fp, &g[w*i], w, maxval, fmt);
	if (depth)
		status = ipdb_insert_g16image(pdb, w, h, g);
	else
		status = ipdb_insert_gimage(pdb, w, h, g);
	free(g);

	return status;
}

static int
readtxt(IPDB *pdb, char *notefile)
{
	struct stat	st;
	char		*s;
	FILE		*fp;
	int		n;

	if(notefile == NULL)
		return 0;
	memset((void *)&st, '\0', sizeof(st));
	if(stat(notefile, &st) != 0)
		return errno;
	fp = pm_openr(notefile);
	if((s = calloc(1, st.st_size + 1)) == NULL)
		return ENOMEM;
	if(fread(s, 1, st.st_size, fp) != (size_t)st.st_size)
		return EIO;
	pm_close(fp);

	for(n = strlen(s) - 1; n >= 0 && s[n] == '\n'; --n)
		s[n] = '\0';
	ipdb_insert_text(pdb, s);

	return 0;
}

int
main(int argc, char **argv)
{
	char	*me, *name, *notefile;
	int	i, status, comp, depth = 0;
	IPDB	*pdb;
	FILE	*in, *out;

	if((me = strrchr(*argv, '/')) == NULL)
		me = *argv;
	else
		++me;
	comp     = IPDB_COMPMAYBE;
	name     = NULL;
	notefile = NULL;
	pgm_init(&argc, argv);
	while((i = getopt(argc, argv, OPTS)) != EOF) {
		switch(i) {
		case '4':	depth = 4;			break;
		case 'h':	exit(usage(me, EXIT_SUCCESS));	break;
		case 'v':	exit(version(me));		break;
		case 't':	name     = optarg;		break;
		case 'c':	comp     = IPDB_COMPRESS;	break;
		case 'u':	comp     = IPDB_NOCOMPRESS;	break;
		case 'm':	comp     = IPDB_COMPMAYBE;	break;
		case 'n':	notefile = optarg;		break;
		default:	exit(usage(me, EXIT_FAILURE));	break;
		}
	}
	if(name == NULL)
		name = argv[optind] == NULL ? "unnamed" : argv[optind];

	in  = argv[optind] == NULL ? stdin  : pm_openr(argv[optind++]);
	out = argv[optind] == NULL ? stdout : pm_openw(argv[optind]);

	if((pdb = ipdb_alloc(name)) == NULL)
		pm_error("%s.", ipdb_err(ENOMEM));
	if((status = readimg(pdb, in, depth)) != 0)
		pm_error("PGM read error: %s.", ipdb_err(status));
	if((status = readtxt(pdb, notefile)) != 0)
		pm_error("Note read error: %s.", ipdb_err(status));
	if((status = ipdb_write(pdb, comp, out)) != 0)
		pm_error("PDB write error: %s.", ipdb_err(status));
	if(comp == IPDB_COMPMAYBE && !ipdb_compressed(pdb))
		pm_message("Image too complex to be compressed.");
	pdb = ipdb_free(pdb);

	if(in != stdin)
		pm_close(in);
	if(out != stdout)
		pm_close(out);

	return EXIT_SUCCESS;
}
