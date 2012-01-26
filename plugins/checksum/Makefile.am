
INCLUDES = \
	-I$(top_srcdir)							\
	-I$(top_srcdir)/librejilla-media/					\
	-I$(top_builddir)/librejilla-media/				\
	-I$(top_srcdir)/librejilla-burn				\
	-I$(top_builddir)/librejilla-burn/				\
	-DREJILLA_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 	\
	-DREJILLA_PREFIX=\"$(prefix)\"           		\
	-DREJILLA_SYSCONFDIR=\"$(sysconfdir)\"   		\
	-DREJILLA_DATADIR=\"$(datadir)/rejilla\"     	    	\
	-DREJILLA_LIBDIR=\"$(libdir)\"  	         	\
	$(WARN_CFLAGS)							\
	$(DISABLE_DEPRECATED)				\
	$(REJILLA_GLIB_CFLAGS)				\
	$(REJILLA_GTK_CFLAGS)

checksumdir = $(REJILLA_PLUGIN_DIRECTORY)
checksum_LTLIBRARIES = librejilla-checksum.la
librejilla_checksum_la_SOURCES = burn-checksum-image.c	

librejilla_checksum_la_LDFLAGS = -module -avoid-version
librejilla_checksum_la_LIBADD = $(REJILLA_GLIB_LIBS) ../../librejilla-media/librejilla-media@REJILLA_LIBRARY_SUFFIX@.la ../../librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la

checksumfiledir = $(REJILLA_PLUGIN_DIRECTORY)
checksumfile_LTLIBRARIES = librejilla-checksum-file.la
librejilla_checksum_file_la_SOURCES = burn-checksum-files.c	\
				      burn-volume-read.c  \
				      burn-volume-read.h

librejilla_checksum_file_la_LDFLAGS = -module -avoid-version
librejilla_checksum_file_la_LIBADD = $(REJILLA_GLIB_LIBS) $(REJILLA_GTK_LIBS) ../../librejilla-media/librejilla-media@REJILLA_LIBRARY_SUFFIX@.la ../../librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la

-include $(top_srcdir)/git.mk
