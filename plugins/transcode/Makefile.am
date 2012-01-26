
INCLUDES = \
	-I$(top_srcdir)					\
	-I$(top_srcdir)/librejilla-media/					\
	-I$(top_builddir)/librejilla-media/		\
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
	$(REJILLA_GSTREAMER_CFLAGS)

transcodedir = $(REJILLA_PLUGIN_DIRECTORY)
transcode_LTLIBRARIES = librejilla-transcode.la

librejilla_transcode_la_SOURCES = burn-transcode.c burn-normalize.h 
librejilla_transcode_la_LIBADD = $(REJILLA_GLIB_LIBS) $(REJILLA_GSTREAMER_LIBS) ../../librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la
librejilla_transcode_la_LDFLAGS = -module -avoid-version

normalizedir = $(REJILLA_PLUGIN_DIRECTORY)
normalize_LTLIBRARIES = librejilla-normalize.la

librejilla_normalize_la_SOURCES = burn-normalize.c burn-normalize.h
librejilla_normalize_la_LIBADD = $(REJILLA_GLIB_LIBS) $(REJILLA_GSTREAMER_LIBS) ../../librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la
librejilla_normalize_la_LDFLAGS = -module -avoid-version

vobdir = $(REJILLA_PLUGIN_DIRECTORY)
vob_LTLIBRARIES = librejilla-vob.la

librejilla_vob_la_SOURCES = burn-vob.c 
librejilla_vob_la_LIBADD = $(REJILLA_GLIB_LIBS) $(REJILLA_GSTREAMER_LIBS) ../../librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la
librejilla_vob_la_LDFLAGS = -module -avoid-version

-include $(top_srcdir)/git.mk
