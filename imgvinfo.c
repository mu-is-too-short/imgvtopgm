/*
 * imgvinfo.c
 *	Pilot Image Viewer header dumper.
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
MU_ID("$Mu: imgvtopgm/imgvinfo.c 1.21 1999/05/09 07:58:02 $")

#include <time.h>
#include <stdlib.h>

#include <mine/ipdb.h>

/*
 * These tend to live in different places on different systems so I'll
 * just do it by hand and assume you have a proper libc.
 */
extern	int	optind;
extern	char	*optarg;
extern	int	getopt(int, char *const *, const char *);

/*
 * "Globals".  I would just use plain global variables but I have a
 * pathalogical problem with writable globals.
 */
typedef struct {
	char	*timefmt;
	int	utc;
	IPDB	*pdb;
} SLOP;

/*
 * Default time format (this is ISO8601 standard, see strftime(3) for
 * the available codes).
 */
#define	TIMEFMT	"%Y-%m-%d %H:%M:%S"

static int
version(const char *me)
{
	printf("%s %s\n", me, VERSION);
	return EXIT_SUCCESS;
}

#define	OPTS	"hvt:ul"
static char *usefmt    = "%s [-hv] [-t fmt] [-{u|l}] [in [...]]\n";
static char *explain[] = {
	"\tDump the header information for a Pilot Image Viewer pdb file.",
	"\tThis program was originally written to test some .pdb header",
	"\tparsing code but it may be a usefull little tool anyway.",
	"",
	"\t-h      Display this usage message and exit.",
	"\t-v      Display the version number and exit.",
	"\t-t fmt  Specify the time display format (see strftime(3)).",
	"\t-u      Display times in UTC.",
	"\t-l      Display times in the local timezone (default).",
	"",
	"\tThe default time format is \"" TIMEFMT "\" (ISO8601 standard).",
	"\tA file name of \"-\" means to read the standard input.",
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

static SLOP *
readit(char *name, SLOP *s)
{
	FILE	*fp  = NULL;
	IPDB	*pdb = NULL;
	int	status;

	if(name == NULL || strcmp(name, "-") == 0) {
		fp = stdin;
	}
	else if((fp = fopen(name, "r")) == NULL) {
		pm_message("cannot open %s: %s", name, strerror(errno));
		goto TheEnd;
	}
	if((pdb = ipdb_alloc(NULL)) == NULL) {
		pm_message("%s: %s", name, ipdb_err(ENOMEM));
		goto TheEnd;
	}
	if((status = ipdb_read(pdb, fp)) != 0) {
		pm_message("%s: %s", name, ipdb_err(status));
		pdb = ipdb_free(pdb);
		goto TheEnd;
	}

TheEnd:
	if(fp != NULL && fp != stdin)
		fclose(fp);
	s->pdb = pdb;
	return s;
}

static char *
ptime(SLOP *s, time_t t)
{
static	char	buf[1024];
	struct tm *(*tf)(const time_t *) = s->utc ? gmtime : localtime;

	strftime(buf, sizeof(buf) - 1, s->timefmt, tf(&t));
	return buf;
}

static char *
typestr(IPDB *pdb)
{
	char	*s;
	switch(ipdb_type(pdb)) {
	case IMG_GRAY16:s = "grayscale16";	break;
	case IMG_GRAY:	s = "grayscale";	break;
	case IMG_MONO:	s = "monochrome";	break;
	default:	s = "unknown";		break;
	}
	return s;
}

#define compstr(pdb)	(ipdb_compressed(pdb) ? "compressed" : "uncompressed")

static void
dumpit(SLOP *s, char *file)
{
	IPDB	*pdb    = s->pdb;
	char	*note   = NULL;

	if(pdb == NULL)
		return;

	printf("file:\t\t%s\n", file == NULL ? "-" : file);
	printf("name:\t\t%s (%s)\n", ipdb_iname(pdb), ipdb_pname(pdb));
	printf("created:\t%s\n", ptime(s, ipdb_ctime(pdb)));
	printf("last modified:\t%s\n", ptime(s, ipdb_mtime(pdb)));
	printf("last backed up:\t%s\n", ptime(s, ipdb_btime(pdb)));
	printf("version:\t%d (%s)\n", ipdb_version(pdb), compstr(pdb));
	printf("type:\t\t%d (%s)\n", ipdb_type(pdb), typestr(pdb));
	printf("last position:\t(%d, %d)\n", ipdb_xlast(pdb), ipdb_ylast(pdb));
	printf("anchor:\t\t(%d, %d)\n", ipdb_xanchor(pdb), ipdb_yanchor(pdb));
	printf("size:\t\t%dx%d\n", ipdb_width(pdb), ipdb_height(pdb));

	if((note = ipdb_text(pdb)) != NULL)
		printf("Note:\n%s\n", note);
	printf("\n");

	s->pdb = ipdb_free(pdb);
}

int
main(int argc, char **argv)
{
	char	*me;
	int	o;
	SLOP	s;

	memset((void *)&s, '\0', sizeof(s));
	if((me = strrchr(*argv, '/')) == NULL)
		me = *argv;
	else
		++me;
	s.timefmt = TIMEFMT;
	s.utc     = FALSE;
	pgm_init(&argc, argv);
	while((o = getopt(argc, argv, OPTS)) != EOF) {
		switch(o) {
		case 'h':	exit(usage(me, EXIT_SUCCESS));		break;
		case 'v':	exit(version(me));			break;
		case 't':	s.timefmt = optarg;			break;
		case 'u':	s.utc     = TRUE;			break;
		case 'l':	s.utc     = FALSE;			break;
		default:	exit(usage(me, EXIT_FAILURE));		break;
		}
	}

	do {
		dumpit(readit(argv[optind], &s), argv[optind]);
	} while(argv[++optind] != NULL);

	return EXIT_SUCCESS;
}
