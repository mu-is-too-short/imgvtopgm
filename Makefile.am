# @(#)$Mu: imgvtopgm/Makefile.am 1.5 1998/12/16 19:19:34 $
##
## Makefile.am
##	automake file for imgvtopgm.
##
## Copyright (C) 1998 Eric A. Howe
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
##
##   Authors:	Eric A. Howe (mu@trends.net)
##

bin_PROGRAMS = imgvinfo imgvtopnm pbmtoimgv pgmtoimgv
bin_SCRIPTS  = imgvview ppmimgvquant

imgvinfo_SOURCES  = imgvinfo.c  ipdb_r.c ipdb_c.c bs.c
imgvtopnm_SOURCES = imgvtopnm.c ipdb_r.c ipdb_c.c bs.c
pbmtoimgv_SOURCES = pbmtoimgv.c ipdb_w.c ipdb_c.c bs.c
pgmtoimgv_SOURCES = pgmtoimgv.c ipdb_w.c ipdb_c.c bs.c

imgvinfo_LDADD  = @PGM_LIBS@ @PBM_LIBS@
imgvtopnm_LDADD = @PGM_LIBS@ @PBM_LIBS@
pbmtoimgv_LDADD = @PGM_LIBS@ @PBM_LIBS@
pgmtoimgv_LDADD = @PGM_LIBS@ @PBM_LIBS@

man_MANS = \
	imgvinfo.1 \
	imgvtopnm.1 \
	imgvview.1 \
	pbmtoimgv.1 \
	pgmtoimgv.1 \
	ppmimgvquant.1

MAINTAINERCLEANFILES = aclocal.m4 Makefile.in

DISTCLEANFILES = \
	imgvinfo.1 \
	imgvtopnm.1 \
	imgvview.1 \
	pbmtoimgv.1 \
	pgmtoimgv.1 \
	ppmimgvquant.1 \
	imgvview \
	ppmimgvquant

INCLUDES = -I.
