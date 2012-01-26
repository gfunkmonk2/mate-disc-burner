INCLUDES = \
	-I$(top_srcdir)							\
	-I$(top_builddir)						\
	-I$(top_srcdir)/librejilla-utils/				\
	-I$(top_builddir)/librejilla-utils/				\
	-I$(top_srcdir)/librejilla-media/				\
	-I$(top_builddir)/librejilla-media/				\
	-DREJILLA_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 	\
	-DREJILLA_PREFIX=\"$(prefix)\"           			\
	-DREJILLA_SYSCONFDIR=\"$(sysconfdir)\"   			\
	-DREJILLA_DATADIR=\"$(datadir)/rejilla\"     	    		\
	-DREJILLA_LIBDIR=\"$(libdir)\"  	         		\
	-DREJILLA_PLUGIN_DIRECTORY=\"$(REJILLA_PLUGIN_DIRECTORY)\"	\
	$(WARN_CFLAGS)							\
	$(DISABLE_DEPRECATED)						\
	$(REJILLA_GMODULE_CFLAGS)					\
	$(REJILLA_GLIB_CFLAGS)						\
	$(REJILLA_GIO_CFLAGS)						\
	$(REJILLA_GTK_CFLAGS)						\
	$(REJILLA_GSTREAMER_CFLAGS)					\
	$(REJILLA_CANBERRA_CFLAGS)

GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
RECMARSHALFILES = librejilla-marshal.h librejilla-marshal.c

librejilla-marshal.h: librejilla-marshal.list
	( $(GLIB_GENMARSHAL) --prefix=rejilla_marshal $(srcdir)/librejilla-marshal.list --header > librejilla-marshal.h )
librejilla-marshal.c: librejilla-marshal.h
	( $(GLIB_GENMARSHAL) --prefix=rejilla_marshal $(srcdir)/librejilla-marshal.list --body --header > librejilla-marshal.c )

headerdir=$(includedir)/rejilla@REJILLA_LIBRARY_SUFFIX@
header_HEADERS =	rejilla-tags.h						\
		rejilla-enums.h						\
		rejilla-error.h						\
		rejilla-status.h					\
		rejilla-burn-lib.h					\
		rejilla-track-type.h					\
		rejilla-track.h						\
		rejilla-track-data.h                 			\
		rejilla-track-data-cfg.h				\
		rejilla-track-disc.h                 			\
		rejilla-track-stream.h                 			\
		rejilla-track-stream-cfg.h             			\
		rejilla-track-image.h					\
		rejilla-track-image-cfg.h				\
		rejilla-session.h                 			\
		rejilla-session-span.h					\
		rejilla-session-cfg.h					\
		rejilla-burn.h						\
		rejilla-burn-options.h        			        \
		rejilla-burn-dialog.h					\
		rejilla-tool-dialog.h					\
		rejilla-sum-dialog.h					\
		rejilla-blank-dialog.h

lib_LTLIBRARIES = \
	librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la

librejilla_burn@REJILLA_LIBRARY_SUFFIX@_la_LIBADD =					\
	../librejilla-media/librejilla-media@REJILLA_LIBRARY_SUFFIX@.la			\
	../librejilla-utils/librejilla-utils@REJILLA_LIBRARY_SUFFIX@.la			\
	$(REJILLA_GLIB_LIBS)					\
	$(REJILLA_GTHREAD_LIBS)					\
	$(REJILLA_GMODULE_LIBS)					\
	$(REJILLA_GIO_LIBS)					\
	$(REJILLA_GTK_LIBS)					\
	$(REJILLA_GSTREAMER_LIBS)				\
	$(REJILLA_CANBERRA_LIBS)

librejilla_burn@REJILLA_LIBRARY_SUFFIX@_la_LDFLAGS =					\
	-version-info $(LIBREJILLA_LT_VERSION)			\
	-no-undefined						\
	-export-dynamic
#	-export-symbols ./librejilla-burn.symbols

librejilla_burn@REJILLA_LIBRARY_SUFFIX@_la_SOURCES = 		\
	librejilla-marshal.c		\
	librejilla-marshal.h		\
	rejilla-tool-dialog.c         \
	rejilla-tool-dialog.h         \
	rejilla-sum-dialog.c         \
	rejilla-sum-dialog.h         \
	rejilla-blank-dialog.c         \
	rejilla-blank-dialog.h         \
	rejilla-burn.c			\
	rejilla-burn.h			\
	rejilla-xfer.c			\
	rejilla-xfer.h			\
	burn-basics.h                 \
	burn-caps.h                 \
	burn-dbus.h                 \
	burn-debug.h                 \
	burn-image-format.h                 \
	burn-job.h                 \
	burn-mkisofs-base.h                 \
	burn-plugin-manager.h                 \
	burn-process.h                 \
	rejilla-session.h                 \
	burn-task.h                 \
	burn-task-ctx.h                 \
	burn-task-item.h                 \
	rejilla-track.h                 \
	rejilla-session.c                 \
	rejilla-track.c                 \
	burn-basics.c                 \
	burn-caps.c                 \
	burn-dbus.c                 \
	burn-debug.c                 \
	burn-image-format.c                 \
	burn-job.c                 \
	burn-mkisofs-base.c                 \
	burn-plugin.c                 \
	burn-plugin-manager.c                 \
	burn-process.c                 \
	burn-task.c                 \
	burn-task-ctx.c                 \
	burn-task-item.c                 \
	rejilla-burn-dialog.c                 \
	rejilla-burn-dialog.h                 \
	rejilla-burn-options.c                 \
	rejilla-burn-options.h                 \
	rejilla-dest-selection.c                 \
	rejilla-dest-selection.h                 \
	rejilla-drive-properties.c                 \
	rejilla-drive-properties.h                 \
	rejilla-image-properties.c                 \
	rejilla-image-properties.h                 \
	rejilla-image-type-chooser.c                 \
	rejilla-image-type-chooser.h                 \
	rejilla-medium-properties.c                 \
	rejilla-medium-properties.h                 \
	rejilla-progress.c                 \
	rejilla-progress.h                 \
	rejilla-session-cfg.c                 \
	rejilla-session-cfg.h                 \
	rejilla-src-image.c                 \
	rejilla-src-image.h                 \
	rejilla-src-selection.c                 \
	rejilla-src-selection.h                 \
	rejilla-tray.c                 \
	rejilla-tray.h                 \
	rejilla-burn-lib.h                 \
	rejilla-error.h                 \
	rejilla-enums.h                 \
	rejilla-tags.h                 \
	rejilla-cover.c                 \
	rejilla-cover.h                 \
	rejilla-track-disc.c                 \
	rejilla-track-disc.h                 \
	rejilla-track-stream.c                 \
	rejilla-track-stream.h                 \
	rejilla-track-image.c                 \
	rejilla-track-image.h                 \
	rejilla-track-data.c                 \
	rejilla-track-data.h                 \
	rejilla-track-image-cfg.c                 \
	rejilla-track-image-cfg.h                 \
	rejilla-caps-plugin.c                 \
	rejilla-caps-burn.c                 \
	rejilla-caps-session.c                 \
	rejilla-caps-burn.h                 \
	rejilla-plugin.h                 \
	rejilla-plugin-information.h                 \
	rejilla-plugin-registration.h                 \
	rejilla-track-type.c                 \
	rejilla-track-type.h                 \
	rejilla-track-type-private.h                 \
	rejilla-status.c                 \
	rejilla-status.h                 \
	rejilla-status-dialog.c                 \
	rejilla-status-dialog.h                 \
	rejilla-session-helper.h                 \
	rejilla-data-project.c                 \
	rejilla-data-project.h                 \
	rejilla-data-session.c                 \
	rejilla-data-session.h                 \
	rejilla-data-vfs.c                 \
	rejilla-data-vfs.h                 \
	rejilla-file-node.c                 \
	rejilla-file-node.h                 \
	rejilla-data-tree-model.c                 \
	rejilla-data-tree-model.h                 \
	rejilla-track-data-cfg.c                 \
	rejilla-track-data-cfg.h                 \
	rejilla-filtered-uri.c                 \
	rejilla-filtered-uri.h                 \
	rejilla-track-stream-cfg.c                 \
	rejilla-track-stream-cfg.h                 \
	rejilla-tool-dialog-private.h                 \
	rejilla-video-options.h                 \
	rejilla-video-options.c                 \
	rejilla-session-span.h                 \
	rejilla-session-span.c                 \
	rejilla-plugin-private.h                 

if BUILD_INOTIFY
librejilla_burn@REJILLA_LIBRARY_SUFFIX@_la_SOURCES += rejilla-file-monitor.c rejilla-file-monitor.h
endif

if HAVE_APP_INDICATOR
librejilla_burn@REJILLA_LIBRARY_SUFFIX@_la_SOURCES += rejilla-app-indicator.h rejilla-app-indicator.c
librejilla_burn@REJILLA_LIBRARY_SUFFIX@_la_LIBADD += @APP_INDICATOR_LIBS@
INCLUDES += -DHAVE_APP_INDICATOR @APP_INDICATOR_CFLAGS@
endif

EXTRA_DIST =			\
	librejilla-marshal.list
#	librejilla-burn.symbols

if HAVE_INTROSPECTION
girdir = $(INTROSPECTION_GIRDIR)
gir_DATA = RejillaBurn-@REJILLA_VERSION@.gir

typelibsdir = $(INTROSPECTION_TYPELIBDIR)
typelibs_DATA = RejillaBurn-@REJILLA_VERSION@.typelib

RejillaBurn-@REJILLA_VERSION@.gir: librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la $(INTROSPECTION_SCANNER)
	$(INTROSPECTION_SCANNER) -v --namespace=RejillaBurn --nsversion=1.0 \
	    -I$(top_srcdir)/librejilla-media	\
	    -I$(top_builddir)/librejilla-media	\
	    -I$(top_srcdir)/librejilla-burn		\
	    -I$(top_builddir)/librejilla-burn	\
            --include=Gtk-@GTK_API_VERSION@ \
            --add-include-path=$(top_builddir)/librejilla-media	\
	    --include=RejillaMedia-@REJILLA_VERSION@ \
	    --library=rejilla-burn@REJILLA_LIBRARY_SUFFIX@ \
	    --libtool="$(LIBTOOL)" \
            --output $@ \
            --pkg=gtk+-@GTK_API_VERSION@  \
	    --strip-prefix=Rejilla	\
            $(top_srcdir)/librejilla-burn/rejilla-tags.h						\
            $(top_srcdir)/librejilla-burn/rejilla-enums.h						\
            $(top_srcdir)/librejilla-burn/rejilla-error.h						\
            $(top_srcdir)/librejilla-burn/rejilla-status.h					\
            $(top_builddir)/librejilla-burn/rejilla-burn-lib.h					\
            $(top_srcdir)/librejilla-burn/rejilla-track-type.h					\
            $(top_srcdir)/librejilla-burn/rejilla-track.h						\
            $(top_srcdir)/librejilla-burn/rejilla-track-data.h                 			\
            $(top_srcdir)/librejilla-burn/rejilla-track-data-cfg.h				\
            $(top_srcdir)/librejilla-burn/rejilla-track-disc.h                 			\
            $(top_srcdir)/librejilla-burn/rejilla-track-stream.h                 			\
            $(top_srcdir)/librejilla-burn/rejilla-track-stream-cfg.h             			\
            $(top_srcdir)/librejilla-burn/rejilla-track-image.h					\
            $(top_srcdir)/librejilla-burn/rejilla-track-image-cfg.h				\
            $(top_srcdir)/librejilla-burn/rejilla-session.h                 			\
            $(top_srcdir)/librejilla-burn/rejilla-session-span.h					\
            $(top_srcdir)/librejilla-burn/rejilla-session-cfg.h					\
            $(top_srcdir)/librejilla-burn/rejilla-burn.h						\
            $(top_srcdir)/librejilla-burn/rejilla-burn-options.h        			        \
            $(top_srcdir)/librejilla-burn/rejilla-burn-dialog.h					\
            $(top_srcdir)/librejilla-burn/rejilla-tool-dialog.h					\
	    $(top_srcdir)/librejilla-burn/rejilla-sum-dialog.h					\
	    $(top_srcdir)/librejilla-burn/*.c	    

RejillaBurn-@REJILLA_VERSION@.typelib: $(gir_DATA) $(INTROSPECTION_COMPILER)
	$(INTROSPECTION_COMPILER) $(top_builddir)/librejilla-burn/$(gir_DATA) --shared-library=librejilla-burn@REJILLA_LIBRARY_SUFFIX@ -o $(typelibs_DATA) --includedir=$(top_builddir)/librejilla-media
endif

CLEANFILES =			\
	$(RECMARSHALFILES)  \
	$(gir_DATA)         \
	$(typelibs_DATA)


-include $(top_srcdir)/git.mk
