
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

#growisofs
growisofsdir = $(REJILLA_PLUGIN_DIRECTORY)
growisofs_LTLIBRARIES = librejilla-growisofs.la
librejilla_growisofs_la_SOURCES = burn-growisofs.c \
	burn-growisofs-common.h 
librejilla_growisofs_la_LIBADD = $(REJILLA_GLIB_LIBS) ../../librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la
librejilla_growisofs_la_LDFLAGS = -module -avoid-version

#dvdrwformat
dvdrwformatdir = $(REJILLA_PLUGIN_DIRECTORY)
dvdrwformat_LTLIBRARIES = librejilla-dvdrwformat.la
librejilla_dvdrwformat_la_SOURCES = burn-dvd-rw-format.c 
librejilla_dvdrwformat_la_LIBADD = $(REJILLA_GLIB_LIBS) ../../librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la
librejilla_dvdrwformat_la_LDFLAGS = -module -avoid-version

-include $(top_srcdir)/git.mk
