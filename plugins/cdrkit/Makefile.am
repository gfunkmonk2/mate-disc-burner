
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

#wodim
wodimdir = $(REJILLA_PLUGIN_DIRECTORY)
wodim_LTLIBRARIES = librejilla-wodim.la
librejilla_wodim_la_SOURCES = burn-wodim.c \
	burn-cdrkit.h 
librejilla_wodim_la_LIBADD = $(REJILLA_GLIB_LIBS) ../../librejilla-media/librejilla-media@REJILLA_LIBRARY_SUFFIX@.la ../../librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la
librejilla_wodim_la_LDFLAGS = -module -avoid-version

#genisoimage
genisoimagedir = $(REJILLA_PLUGIN_DIRECTORY)
genisoimage_LTLIBRARIES = librejilla-genisoimage.la
librejilla_genisoimage_la_SOURCES = burn-genisoimage.c \
	burn-cdrkit.h 
librejilla_genisoimage_la_LIBADD = $(REJILLA_GLIB_LIBS) ../../librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la
librejilla_genisoimage_la_LDFLAGS = -module -avoid-version

#readom
readomdir = $(REJILLA_PLUGIN_DIRECTORY)
readom_LTLIBRARIES = librejilla-readom.la
librejilla_readom_la_SOURCES = burn-readom.c \
	burn-cdrkit.h 
librejilla_readom_la_LIBADD = $(REJILLA_GLIB_LIBS) ../../librejilla-media/librejilla-media@REJILLA_LIBRARY_SUFFIX@.la ../../librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la
librejilla_readom_la_LDFLAGS = -module -avoid-version

-include $(top_srcdir)/git.mk
