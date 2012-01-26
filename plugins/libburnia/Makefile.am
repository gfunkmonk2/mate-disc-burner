
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
	$(REJILLA_LIBISOFS_CFLAGS)			\
	$(REJILLA_LIBBURN_CFLAGS)			\
	$(REJILLA_GLIB_CFLAGS)

#libburn
libburndir = $(REJILLA_PLUGIN_DIRECTORY)
libburn_LTLIBRARIES = librejilla-libburn.la
librejilla_libburn_la_SOURCES = burn-libburn.c 		\
				burn-libburn-common.c	\
				burn-libburn-common.h  \
				burn-libburnia.h 
librejilla_libburn_la_LIBADD = $(REJILLA_GLIB_LIBS) $(REJILLA_LIBBURNIA_LIBS) ../../librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la
librejilla_libburn_la_LDFLAGS = -module -avoid-version

#libisofs (apparently libisofs needs one libburn function)
libisofsdir = $(REJILLA_PLUGIN_DIRECTORY)
libisofs_LTLIBRARIES = librejilla-libisofs.la
librejilla_libisofs_la_SOURCES = burn-libisofs.c                       \
	burn-libburn-common.c burn-libburn-common.h			\
	burn-libburnia.h 
librejilla_libisofs_la_LIBADD = $(REJILLA_GLIB_LIBS) $(REJILLA_LIBBURNIA_LIBS) ../../librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la
librejilla_libisofs_la_LDFLAGS = -module -avoid-version

-include $(top_srcdir)/git.mk
