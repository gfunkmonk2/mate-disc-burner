module_flags = -avoid-version -module

INCLUDES =					\
	-I$(top_srcdir)/librejilla-utils/	\
	-I$(top_builddir)/librejilla-utils/	\
	-I$(top_srcdir)/librejilla-media/	\
	-I$(top_builddir)/librejilla-media/	\
	-I$(top_srcdir)/librejilla-burn/	\
	-I$(top_builddir)/librejilla-burn/	\
	-I$(top_srcdir)/src/			\
	-I$(top_builddir)/src/			\
	$(CAJA_EXTENSION_CFLAGS)		\
	$(WARN_CFLAGS)							\
	$(WERROR)				\
	-D_FILE_OFFSET_BITS=64			\
	-D_GNU_SOURCE				\
	-D_LARGEFILE64_SOURCE			\
	-D_REENTRANT				\
	-D_POSIX_PTHREAD_SEMANTICS		\
	-DPREFIX=\"$(prefix)\"			\
	-DBINDIR=\""$(bindir)"\"		\
	-DLIBEXECDIR=\""$(libexecdir)"\"	\
	-DSHAREDIR=\""$(datadir)"\"		\
	-DDATADIR=\""$(datadir)/rejilla"\"	\
	$(DISABLE_DEPRECATED)

@INTLTOOL_DESKTOP_RULE@

desktopdir=$(datadir)/applications
desktop_in_files = rejilla-caja.desktop.in

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

EXTRA_DIST = $(desktop_in_files)
CLEANFILES = $(desktop_DATA)
DISTCLEANFILES = $(NULL)

### Caja Extension
caja_extensiondir = $(CAJADIR)
caja_extension_LTLIBRARIES = libcaja-rejilla-extension.la

libcaja_rejilla_extension_la_SOURCES = 	\
	caja-burn-bar.c			\
	caja-burn-bar.h			\
	caja-burn-extension.c			\
	$(top_srcdir)/src/rejilla-project-name.c                       \
	$(top_srcdir)/src/rejilla-project-name.h	\
	$(top_srcdir)/src/rejilla-drive-settings.h	\
	$(top_srcdir)/src/rejilla-drive-settings.c

libcaja_rejilla_extension_la_LIBADD =	\
	$(CAJA_EXTENSION_LIBS)		\
	../librejilla-utils/librejilla-utils@REJILLA_LIBRARY_SUFFIX@.la	\
	../librejilla-media/librejilla-media@REJILLA_LIBRARY_SUFFIX@.la	\
	../librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la
	
libcaja_rejilla_extension_la_LDFLAGS = $(module_flags)

-include $(top_srcdir)/git.mk
