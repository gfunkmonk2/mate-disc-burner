
INCLUDES = \
	-I$(top_srcdir)						\
	-I$(top_srcdir)/librejilla-media/					\
	-I$(top_builddir)/librejilla-media/			\
	-I$(top_srcdir)/librejilla-burn				\
	-I$(top_builddir)/librejilla-burn/				\
	-DREJILLA_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 	\
	-DREJILLA_PREFIX=\"$(prefix)\"           		\
	-DREJILLA_SYSCONFDIR=\"$(sysconfdir)\"   		\
	-DREJILLA_DATADIR=\"$(datadir)/rejilla\"     	    	\
	-DREJILLA_LIBDIR=\"$(libdir)\"  	         	\
	$(WARN_CFLAGS)							\
	$(DISABLE_DEPRECATED)					\
	$(REJILLA_GLIB_CFLAGS)					\
	$(REJILLA_GIO_CFLAGS)

#local-track
local_trackdir = $(REJILLA_PLUGIN_DIRECTORY)
local_track_LTLIBRARIES = librejilla-local-track.la

librejilla_local_track_la_SOURCES = burn-local-image.c 
librejilla_local_track_la_LIBADD = $(REJILLA_GLIB_LIBS) $(REJILLA_GIO_LIBS) ../../librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la
librejilla_local_track_la_LDFLAGS = -module -avoid-version

#burn-uri
burn_uridir = $(REJILLA_PLUGIN_DIRECTORY)
burn_uri_LTLIBRARIES = librejilla-burn-uri.la

librejilla_burn_uri_la_SOURCES = burn-uri.c 
librejilla_burn_uri_la_LIBADD = $(REJILLA_GLIB_LIBS) $(REJILLA_GIO_LIBS) ../../librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la
librejilla_burn_uri_la_LDFLAGS = -module -avoid-version

-include $(top_srcdir)/git.mk
