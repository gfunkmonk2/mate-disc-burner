
INCLUDES = \
	-I$(top_srcdir)							\
	-I$(top_builddir)						\
	-DREJILLA_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 	\
	-DREJILLA_PREFIX=\"$(prefix)\"           			\
	-DREJILLA_SYSCONFDIR=\"$(sysconfdir)\"   			\
	-DREJILLA_DATADIR=\"$(datadir)/rejilla\"     	    		\
	-DREJILLA_LIBDIR=\"$(libdir)\"  	         		\
	$(WARN_CFLAGS)							\
	$(DISABLE_DEPRECATED)						\
	$(REJILLA_GLIB_CFLAGS)						\
	$(REJILLA_GMODULE_EXPORT_CFLAGS)					\
	$(REJILLA_GIO_CFLAGS)						\
	$(REJILLA_GTK_CFLAGS)

GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
RECMARSHALFILES = librejilla-marshal.h librejilla-marshal.c

librejilla-marshal.h: librejilla-marshal.list
	( $(GLIB_GENMARSHAL) --prefix=rejilla_marshal $(srcdir)/librejilla-marshal.list --header > librejilla-marshal.h )
librejilla-marshal.c: librejilla-marshal.h
	( $(GLIB_GENMARSHAL) --prefix=rejilla_marshal $(srcdir)/librejilla-marshal.list --body --header > librejilla-marshal.c )

headerdir=$(includedir)/rejilla@REJILLA_LIBRARY_SUFFIX@
header_HEADERS =	rejilla-drive.h			\
		rejilla-medium-monitor.h		\
		rejilla-media.h			\
		rejilla-units.h			\
		rejilla-medium.h			\
		rejilla-volume.h		\
		rejilla-medium-selection.h	\
		rejilla-drive-selection.h

lib_LTLIBRARIES = 				\
	librejilla-media@REJILLA_LIBRARY_SUFFIX@.la

librejilla_media@REJILLA_LIBRARY_SUFFIX@_la_LIBADD =                                     \
	$(REJILLA_GLIB_LIBS)                                     \
	$(REJILLA_GMODULE_EXPORT_LIBS)                                     \
	$(REJILLA_GTHREAD_LIBS)                                     \
	$(REJILLA_GIO_LIBS)                                     \
	$(REJILLA_GTK_LIBS)                                     \
	$(REJILLA_SCSI_LIBS)

librejilla_media@REJILLA_LIBRARY_SUFFIX@_la_LDFLAGS =					\
	-version-info $(LIBREJILLA_LT_VERSION)			\
	-no-undefined
	$(AM_LDFLAGS)
#	-export-symbols ./librejilla-media.symbols

librejilla_media@REJILLA_LIBRARY_SUFFIX@_la_SOURCES =		\
	librejilla-marshal.c		\
	librejilla-marshal.h		\
	rejilla-drive.h			\
	rejilla-medium-monitor.h		\
	rejilla-media.h			\
	rejilla-units.h			\
	rejilla-medium.h			\
	rejilla-volume.h		\
	rejilla-medium-selection.h	\
	scsi-base.h         \
	scsi-command.h         \
	scsi-error.h         \
	scsi-get-configuration.c         \
	scsi-get-configuration.h         \
	scsi-mmc1.h         \
	scsi-mmc2.h         \
	scsi-opcodes.h         \
	scsi-read-disc-info.c         \
	scsi-read-disc-info.h         \
	scsi-read-toc-pma-atip.c         \
	scsi-read-toc-pma-atip.h         \
	scsi-sense-data.c         \
	scsi-sense-data.h         \
	scsi-utils.h         \
	scsi-q-subchannel.h         \
	scsi-error.c         \
	scsi-read-track-information.c         \
	scsi-read-track-information.h         \
	scsi-get-performance.c         \
	scsi-get-performance.h         \
	scsi-mmc3.h         \
	scsi-mode-pages.h         \
	scsi-mode-sense.c         \
	scsi-status-page.h         \
	scsi-spc1.h         \
	scsi-read-capacity.c         \
	scsi-read-capacity.h         \
	scsi-read-disc-structure.c         \
	scsi-read-disc-structure.h         \
	scsi-dvd-structures.h         \
	scsi-read-format-capacities.c         \
	scsi-read-format-capacities.h         \
	scsi-read-cd.h	\
	scsi-read-cd.c	\
	scsi-device.h         \
	scsi-mech-status.c         \
	scsi-mech-status.h         \
	scsi-write-page.h         \
	scsi-mode-select.c         \
	scsi-read10.c         \
	scsi-sbc.h		\
	scsi-test-unit-ready.c           \
	rejilla-media.c           \
	rejilla-medium-monitor.c         \
	burn-susp.c         \
	burn-susp.h         \
	burn-iso-field.c         \
	burn-iso-field.h         \
	burn-iso9660.c         \
	burn-iso9660.h         \
	burn-volume-source.c         \
	burn-volume-source.h         \
	burn-volume.c         \
	burn-volume.h         \
	rejilla-medium.c         \
	rejilla-volume.c         \
	rejilla-drive.c         \
	rejilla-medium-selection.c         \
	rejilla-units.c         \
	rejilla-drive-selection.c         \
	rejilla-drive-selection.h         \
	rejilla-media-private.h         \
	rejilla-medium-selection-priv.h         \
	rejilla-gio-operation.h         \
	rejilla-gio-operation.c         \
	scsi-inquiry.c         \
	scsi-prevent-allow-medium-removal.c         \
	scsi-inquiry.h         \
	rejilla-drive-priv.h

# FreeBSD's SCSI CAM interface
if HAVE_CAM_LIB_H
librejilla_media@REJILLA_LIBRARY_SUFFIX@_la_SOURCES += scsi-cam.c
endif

# Linux's SCSI CAM interface
if HAVE_SG_IO_HDR_T
librejilla_media@REJILLA_LIBRARY_SUFFIX@_la_SOURCES += scsi-sg.c
endif

# NetBSD's scsi(4)
if HAVE_SCSIIO_H
librejilla_media@REJILLA_LIBRARY_SUFFIX@_la_SOURCES += scsi-netbsd.c
endif

# Solaris's USCSI interface
if HAVE_USCSI_H
librejilla_media@REJILLA_LIBRARY_SUFFIX@_la_SOURCES += scsi-uscsi.c
endif

if HAVE_INTROSPECTION
girdir = $(INTROSPECTION_GIRDIR)
gir_DATA = RejillaMedia-@REJILLA_VERSION@.gir

typelibsdir = $(INTROSPECTION_TYPELIBDIR)
typelibs_DATA = RejillaMedia-@REJILLA_VERSION@.typelib

RejillaMedia-@REJILLA_VERSION@.gir: librejilla-media@REJILLA_LIBRARY_SUFFIX@.la $(INTROSPECTION_SCANNER)
	$(INTROSPECTION_SCANNER) -v --namespace=RejillaMedia --nsversion=1.0 \
            --include=Gtk-@GTK_API_VERSION@ \
	    -I$(top_srcdir)/librejilla-media	\
	    --library=rejilla-media@REJILLA_LIBRARY_SUFFIX@ \
	    --libtool="$(LIBTOOL)" \
            --output $@ \
            --pkg=gtk+-@GTK_API_VERSION@  \
	    --strip-prefix=Rejilla	\
            $(top_srcdir)/librejilla-media/rejilla-drive.h			\
	    $(top_srcdir)/librejilla-media/rejilla-medium-monitor.h		\
	    $(top_builddir)/librejilla-media/rejilla-media.h			\
	    $(top_srcdir)/librejilla-media/rejilla-units.h			\
	    $(top_srcdir)/librejilla-media/rejilla-medium.h			\
	    $(top_srcdir)/librejilla-media/rejilla-volume.h		\
	    $(top_srcdir)/librejilla-media/rejilla-medium-selection.h	\
	    $(top_srcdir)/librejilla-media/rejilla-drive-selection.h	\
            $(top_srcdir)/librejilla-media/*.c	    

RejillaMedia-@REJILLA_VERSION@.typelib: $(gir_DATA) $(INTROSPECTION_COMPILER)
	$(INTROSPECTION_COMPILER) $(top_builddir)/librejilla-media/$(gir_DATA) --shared-library=librejilla-media@REJILLA_LIBRARY_SUFFIX@ -o $(typelibs_DATA)
endif

EXTRA_DIST =			\
	librejilla-marshal.list
#	librejilla-media.symbols

CLEANFILES =			\
	$(RECMARSHALFILES)  \
	$(gir_DATA)         \
	$(typelibs_DATA)

-include $(top_srcdir)/git.mk
