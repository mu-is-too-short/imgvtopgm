dnl
dnl AC_PROG_CC replacement.
dnl
dnl The -DMU_AUTOKLUDGE business is an interesting study in kludging around
dnl code that is trying to be smarter than it really is.  If CFLAGS doesn't
dnl have anything in it, AC_PROG_CC will put things in it for us; in
dnl particular, the dreaded debug flag, -g, will appear under various
dnl circumstances.  I don't want debug information it be around by default,
dnl I want optimizations turned on and debug turned off unless someone asks
dnl for different settings via --disable-optimize and/or --enable-debug.
dnl The problem is that I don't know what compiler optimization flags to
dnl use until the compiler is known; hence, the debug/optimization settings
dnl cannot be dealt with until AC_PROG_CC has done its thing.  To keep
dnl AC_PROG_CC from polluting CFLAGS with debug settings we have to make sure
dnl that something is in CFLAGS before AC_PROG_CC executes.  I have chosen
dnl to slip a -DMU_AUTOKLUDGE into CFLAGS since it should be completely
dnl harmless and it is easy to take out later.  This whole thing is just
dnl a way to let me use -O2, -xO2, or plain -O depending on what works.
dnl
dnl This macro runs AC_PROG_CC (with a little trick) and AC_ISC_POSIX.
dnl After that, we check for --enable-debug and --enable-optimize switches
dnl (and their --disable counterparts of course) to pick up debug and/or
dnl optimization flags for the C compiler.
dnl
AC_DEFUN(MU_PROG_CC, [
	CFLAGS="${CFLAGS:--DMU_AUTOKLUDGE}"
	AC_PROG_CC
	CFLAGS=`echo $CFLAGS | sed 's/-DMU_AUTOKLUDGE//'`

	dnl
	dnl We must call AC_ISC_POSIX before AC_TRY_COMPILE or we
	dnl get some warnings.
	dnl
	AC_ISC_POSIX

	dnl
	dnl Check for debug options.
	dnl
	mu_debug=${DEBUGFLAG:--g}
	AC_ARG_ENABLE(debug,
	[  --enable-debug          turn on debugging [default=no]], [
		if test "x${enable_debug}" != "xyes"; then
			mu_debug=""
		fi
	], [
		mu_debug=""
	])
	if test "x${mu_debug}" != "x"; then
		dnl
		dnl We could end up with multiple -g's in here but I really
		dnl don't care, if the compiler ends up caring then too bad,
		dnl so there!
		dnl
		CFLAGS="$mu_debug $CFLAGS"
	fi

	dnl
	dnl Check for various optimization settings.
	dnl
	mu_opt="yes"
	AC_ARG_ENABLE(optimize,
	[  --enable-optimize       turn on optimization [default=yes]],
		if test "x$enable_optimize" = "xno"; then
			mu_opt="no"
		fi
	)
	if test "x${mu_opt}" != "xyes"; then
		OPTFLAG=""
		mu_opt_works="no"
	elif test "x${OPTFLAG}" != "x"; then
		dnl
		dnl They've specified a specific optimization flag so we'll be
		dnl nice and use it (besides, the docs say we will and matching
		dnl the docs is generally a good idea).
		dnl
		CFLAGS="$OPTFLAG $CFLAGS"
		mu_opt_works="yes"
	else
		dnl
		dnl We try -O2 before -xO2 since -O2 will usually work and gcc
		dnl seems to just warn that "language O2 is unknown" with -xO2
		dnl but the compile doesn't fail, sigh.  Hopefully Sun's
		dnl compiler will fail with -O2 so that we can pick up -xO2.
		dnl
		mu_cflags="$CFLAGS"
		mu_opt_works="no"
		for OPTFLAG in -O2 -xO2 -O ""; do
			if test $mu_opt_works != "yes"; then
				CFLAGS="$OPTFLAG $mu_cflags"
				AC_TRY_COMPILE([],[],
					mu_opt_works=yes,
					mu_opt_works=no
				)
			fi
		done
	fi

	dnl
	dnl If we've got an optimization flag but no debug flag, then
	dnl we want to strip the final binaries.
	dnl
	if test "x${mu_opt_works}" != "x"; then
		if test "x${mu_debug}" = "x"; then
			LDFLAGS="${STRIPFLAG:--s} $LDFLAGS"
		fi
	fi
])

dnl
dnl MU_FIND_PNM
dnl -----------
dnl
dnl Find netpbm libraries and headers
dnl Put include directory in pnm_includes,
dnl put library directory in pnm_libraries,
dnl and add appropriate flags to X_CFLAGS and PNM_LIBS.
dnl
dnl Based on DDD's Motif finding code.
dnl
AC_DEFUN(MU_FIND_PNM, [
	pnm_includes=
	pnm_libraries=
	AC_ARG_WITH(pnm-includes, [  --with-pnm-includes=DIR        netpbm include files are in DIR], pnm_includes="$withval")
	AC_ARG_WITH(pnm-libraries, [  --with-pnm-libraries=DIR       netpbm libraries are in DIR], pnm_libraries="$withval")

	AC_MSG_CHECKING(for netpbm)

	##
	## Headers first.
	##

	if test "$pnm_includes" = ""; then
		AC_CACHE_VAL(mu_cv_pnm_includes, [
			mu_pnm_save_LIBS="$LIBS"
			mu_pnm_save_CFLAGS="$CFLAGS"
			mu_pnm_save_CPPFLAGS="$CPPFLAGS"
			mu_pnm_save_LDFLAGS="$LDFLAGS"

			CFLAGS="$X_CFLAGS $CFLAGS"
			CPPFLAGS="$X_CFLAGS $CPPFLAGS"
			LDFLAGS="$X_LDFLAGS $LDFLAGS"

			AC_TRY_COMPILE([#include <pbm.h>], [int a;], [
				##
				## Headers are in the standard include path.
				##
				mu_cv_pnm_includes=
			], [
				##
				## pbm.h is not in the standard search path.
				## Locate it and put its directory in
				## `pnm_includes'.  We look in all kinds
				## of places.
				##
				for dir in "${prefix}/include" \
						/usr/include \
						/usr/local/include \
						/usr/include/Motif* \
						/usr/include/X11* \
						/usr/dt/include \
						/usr/openwin/include \ \
						/usr/dt/*/include \
						/opt/*/include \
						"${prefix}"/*/include \
						/usr/*/include \
						/usr/local/*/include \
						"${prefix}"/include/* \
						/usr/include/* \
						/usr/local/include/*; do
					if test -f "$dir/pbm.h"; then
						mu_cv_pnm_includes="$dir"
						break
					fi
				done
				if test "$mu_cv_pnm_includes" = ""; then
					mu_cv_pnm_includes=no
				fi
			])

			LIBS="$mu_pnm_save_LIBS"
			CFLAGS="$mu_pnm_save_CFLAGS"
			CPPFLAGS="$mu_pnm_save_CPPFLAGS"
			LDFLAGS="$mu_pnm_save_LDFLAGS"
		])
		pnm_includes="$mu_cv_pnm_includes"
	fi

	##
	## And now for the libraries.
	##
	if test "$pnm_libraries" = ""; then
		AC_CACHE_VAL(mu_cv_pnm_libraries, [
			mu_pnm_save_LIBS="$LIBS"
			mu_pnm_save_CFLAGS="$CFLAGS"
			mu_pnm_save_CPPFLAGS="$CPPFLAGS"
			mu_pnm_save_LDFLAGS="$LDFLAGS"

			LIBS="-lpbm $LIBS"
			CFLAGS="$X_CFLAGS $CFLAGS"
			CPPFLAGS="$X_CFLAGS $CPPFLAGS"
			LDFLAGS="$X_LDFLAGS $LDFLAGS"

			AC_TRY_LINK([
				#include <pbm.h>
			], [pbm_init((int *)0, (char **)0);], [
				##
				## libpbm is in the standard search path.
				##
				mu_cv_pnm_libraries=
			], [
				##
				## libpbm.a is not in the standard search path.
				## Locate it and put its directory in
				## `pnm_libraries'.
				##
				for dir in "${prefix}/lib" \
						/usr/lib \
						/usr/local/lib \
						/usr/lib/Motif* \
						/usr/lib/X11* \
						/usr/dt/lib \
						/usr/openwin/lib \
						/usr/dt/*/lib \
						/opt/*/lib \
						/usr/lesstif*/lib \
						/usr/lib/Lesstif* \
						"${prefix}"/*/lib \
						/usr/*/lib \
						/usr/local/*/lib \
						"${prefix}"/lib/* \
						/usr/lib/* \
						/usr/local/lib/*; do
					if test -d "$dir" \
					&& test "`ls $dir/libpbm.* 2>/dev/null`" != ""; then
						mu_cv_pnm_libraries="$dir"
						break
					fi
				done
				if test "$mu_cv_pnm_libraries" = ""; then
					mu_cv_pnm_libraries=no
				fi
			])
			LIBS="$mu_pnm_save_LIBS"
			CFLAGS="$mu_pnm_save_CFLAGS"
			CPPFLAGS="$mu_pnm_save_CPPFLAGS"
			LDFLAGS="$mu_pnm_save_LDFLAGS"
		])
		pnm_libraries="$mu_cv_pnm_libraries"
	fi

	##
	## Add netpbm definitions to X flags.
	##

	if test "$pnm_includes" != "" \
	&& test "$pnm_includes" != "$x_includes" \
	&& test "$pnm_includes" != "no"; then
		X_CFLAGS="-I$pnm_includes $X_CFLAGS"
	fi

	PNM_LIBS=""
	if test "$pnm_libraries" != "" \
	&& test "$pnm_libraries" != "$x_libraries" \
	&& test "$pnm_libraries" != "no"; then
		PNM_LIBS="-L$pnm_libraries"
	fi

	pnm_libraries_result="$pnm_libraries"
	pnm_includes_result="$pnm_includes"

	if test "$pnm_libraries_result" = ""; then
		pnm_libraries_result="in default path"
	elif test "$pnm_libraries_result" = "no"; then
		pnm_libraries_result="(none)"
	fi
	if test "$pnm_includes_result" = ""; then
		pnm_includes_result="in default path"
	elif test "$pnm_includes_result" = "no"; then
		pnm_includes_result="(none)"
	fi
	AC_MSG_RESULT([libraries $pnm_libraries_result, headers $pnm_includes_result])
])dnl
