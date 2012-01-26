
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
	$(REJILLA_LIBXML_CFLAGS)

dvdauthordir = $(REJILLA_PLUGIN_DIRECTORY)
dvdauthor_LTLIBRARIES = librejilla-dvdauthor.la

librejilla_dvdauthor_la_SOURCES = burn-dvdauthor.c 
librejilla_dvdauthor_la_LIBADD = $(REJILLA_GLIB_LIBS) $(REJILLA_LIBXML_LIBS) ../../librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la
librejilla_dvdauthor_la_LDFLAGS = -module -avoid-version


-include $(top_srcdir)/git.mk
