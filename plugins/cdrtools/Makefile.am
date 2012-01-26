
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

#cdrecord
cdrecorddir = $(REJILLA_PLUGIN_DIRECTORY)
cdrecord_LTLIBRARIES = librejilla-cdrecord.la
librejilla_cdrecord_la_SOURCES = burn-cdrecord.c \
	burn-cdrtools.h 
librejilla_cdrecord_la_LIBADD = $(REJILLA_GLIB_LIBS) ../../librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la
librejilla_cdrecord_la_LDFLAGS = -module -avoid-version

#mkisofs
mkisofsdir = $(REJILLA_PLUGIN_DIRECTORY)
mkisofs_LTLIBRARIES = librejilla-mkisofs.la
librejilla_mkisofs_la_SOURCES = burn-mkisofs.c \
	burn-cdrtools.h 
librejilla_mkisofs_la_LIBADD = $(REJILLA_GLIB_LIBS) ../../librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la
librejilla_mkisofs_la_LDFLAGS = -module -avoid-version

#readcd
readcddir = $(REJILLA_PLUGIN_DIRECTORY)
readcd_LTLIBRARIES = librejilla-readcd.la
librejilla_readcd_la_SOURCES = burn-readcd.c \
	burn-cdrtools.h 
librejilla_readcd_la_LIBADD = $(REJILLA_GLIB_LIBS) ../../librejilla-media/librejilla-media@REJILLA_LIBRARY_SUFFIX@.la ../../librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la
librejilla_readcd_la_LDFLAGS = -module -avoid-version

#cdda2wav
cdda2wavdir = $(REJILLA_PLUGIN_DIRECTORY)
cdda2wav_LTLIBRARIES = librejilla-cdda2wav.la
librejilla_cdda2wav_la_SOURCES = burn-cdda2wav.c burn-cdrtools.h 
librejilla_cdda2wav_la_LIBADD = $(REJILLA_GLIB_LIBS) ../../librejilla-media/librejilla-media@REJILLA_LIBRARY_SUFFIX@.la ../../librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la
librejilla_cdda2wav_la_LDFLAGS = -module -avoid-version

-include $(top_srcdir)/git.mk
