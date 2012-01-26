
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
	$(REJILLA_GLIB_CFLAGS)

#cdrdao
cdrdaodir = $(REJILLA_PLUGIN_DIRECTORY)
cdrdao_LTLIBRARIES = librejilla-cdrdao.la
librejilla_cdrdao_la_SOURCES = burn-cdrdao.c 
librejilla_cdrdao_la_LIBADD = $(REJILLA_GLIB_LIBS) ../../librejilla-media/librejilla-media@REJILLA_LIBRARY_SUFFIX@.la ../../librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la
librejilla_cdrdao_la_LDFLAGS = -module -avoid-version

-include $(top_srcdir)/git.mk
