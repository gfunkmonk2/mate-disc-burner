
INCLUDES = \
	-I$(top_srcdir)							\
	-I$(top_srcdir)/librejilla-utils/				\
	-I$(top_builddir)/librejilla-utils/				\
	-I$(top_srcdir)/librejilla-media/				\
	-I$(top_builddir)/librejilla-media/				\
	-I$(top_srcdir)/librejilla-burn/				\
	-I$(top_builddir)/librejilla-burn/				\
	-DREJILLA_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 	\
	-DREJILLA_PREFIX=\"$(prefix)\"           			\
	-DREJILLA_SYSCONFDIR=\"$(sysconfdir)\"   			\
	-DREJILLA_DATADIR=\"$(datadir)/rejilla\"     	    		\
	-DREJILLA_LIBDIR=\"$(libdir)\"  	         		\
	$(WARN_CFLAGS)							\
	$(DISABLE_DEPRECATED)						\
	$(REJILLA_GLIB_CFLAGS)						\
	$(REJILLA_GIO_CFLAGS)						\
	$(REJILLA_GSTREAMER_CFLAGS)					\
	$(REJILLA_GSTREAMER_BASE_CFLAGS)				\
	$(REJILLA_GTK_CFLAGS)						\
	$(REJILLA_DBUS_CFLAGS)						\
	$(REJILLA_LIBXML_CFLAGS)					\
	$(REJILLA_LIBUNIQUE_CFLAGS) 					\
	$(REJILLA_SEARCH_CFLAGS)					\
	$(REJILLA_PL_PARSER_CFLAGS)					\
	$(REJILLA_SM_CFLAGS)

GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
RECMARSHALFILES = rejilla-marshal.h rejilla-marshal.c

rejilla-marshal.h: rejilla-marshal.list
	( $(GLIB_GENMARSHAL) --prefix=rejilla_marshal $(srcdir)/rejilla-marshal.list --header > rejilla-marshal.h )
rejilla-marshal.c: rejilla-marshal.h
	( $(GLIB_GENMARSHAL) --prefix=rejilla_marshal $(srcdir)/rejilla-marshal.list --body --header > rejilla-marshal.c )

bin_PROGRAMS = rejilla

rejilla_SOURCES = \
	rejilla-marshal.c	\
	rejilla-marshal.h	\
	main.c		\
	rejilla-utils.h		\
	rejilla-utils.c         \
	rejilla-player.c         \
	rejilla-player.h         \
	rejilla-eject-dialog.c		\
	rejilla-eject-dialog.h		\
	rejilla-disc.c         \
	rejilla-disc.h         \
	rejilla-audio-disc.c         \
	rejilla-audio-disc.h         \
	rejilla-project.c         \
	rejilla-project.h         \
	rejilla-project-type-chooser.c         \
	rejilla-project-type-chooser.h         \
	rejilla-player-bacon.c         \
	rejilla-player-bacon.h         \
	rejilla-song-properties.c         \
	rejilla-song-properties.h         \
	rejilla-uri-container.c         \
	rejilla-uri-container.h         \
	rejilla-project-manager.c         \
	rejilla-project-manager.h         \
	rejilla-layout.c         \
	rejilla-layout.h         \
	rejilla-file-chooser.c         \
	rejilla-file-chooser.h         \
	eggtreemultidnd.c         \
	eggtreemultidnd.h         \
	rejilla-multi-dnd.c         \
	rejilla-multi-dnd.h         \
	rejilla-layout-object.c         \
	rejilla-layout-object.h         \
	rejilla-plugin-manager-ui.c         \
	rejilla-plugin-manager-ui.h         \
	rejilla-pref.h         \
	rejilla-pref.c         \
	rejilla-plugin-option.h         \
	rejilla-plugin-option.c         \
	rejilla-split-dialog.h         \
	rejilla-split-dialog.c         \
	rejilla-time-button.h         \
	rejilla-time-button.c         \
	rejilla-preview.h         \
	rejilla-preview.c         \
	rejilla-data-disc.h         \
	rejilla-data-disc.c         \
	rejilla-file-filtered.h         \
	rejilla-file-filtered.c         \
	rejilla-filter-option.h         \
	rejilla-filter-option.c         \
	baobab-cell-renderer-progress.c         \
	baobab-cell-renderer-progress.h         \
	rejilla-multi-song-props.h         \
	rejilla-multi-song-props.c         \
	rejilla-rename.h         \
	rejilla-rename.c         \
	rejilla-video-disc.c         \
	rejilla-video-disc.h         \
	rejilla-video-tree-model.c         \
	rejilla-video-tree-model.h         \
	rejilla-app.c		\
	rejilla-app.h           \
	rejilla-project-name.c           \
	rejilla-project-name.h           \
	rejilla-project-parse.c           \
	rejilla-project-parse.h           \
	rejilla-xsession.c           \
	rejilla-xsession.h		\
	eggdesktopfile.c        \
	eggdesktopfile.h        \
	eggsmclient.c        \
	eggsmclient.h        \
	eggsmclient-private.h        \
	eggsmclient-xsmp.c        \
	rejilla-setting.h        \
	rejilla-setting.c        \
	rejilla-search-engine.h        \
	rejilla-search-engine.c        \
	rejilla-drive-settings.h        \
	rejilla-drive-settings.c	\
	rejilla-song-control.h        \
	rejilla-song-control.c        \
	rejilla-cli.c        \
	rejilla-cli.h

if BUILD_PLAYLIST
rejilla_SOURCES += \
	rejilla-playlist.c         \
	rejilla-playlist.h

endif

if BUILD_TRACKER
rejilla_SOURCES += \
		   rejilla-search-tracker.h	\
		   rejilla-search-tracker.c
endif

if BUILD_BEAGLE
rejilla_SOURCES += 				\
		   rejilla-search-beagle.h	\
		   rejilla-search-beagle.c
endif

if BUILD_SEARCH

rejilla_SOURCES += \
		   rejilla-mime-filter.c         \
		   rejilla-mime-filter.h         \
		   rejilla-search-entry.c         \
		   rejilla-search-entry.h         \
		   rejilla-search.h		\
		   rejilla-search.c
endif

rejilla_LDADD =						\
	$(top_builddir)/librejilla-media/librejilla-media@REJILLA_LIBRARY_SUFFIX@.la	\
	$(top_builddir)/librejilla-burn/librejilla-burn@REJILLA_LIBRARY_SUFFIX@.la	\
	$(top_builddir)/librejilla-utils/librejilla-utils@REJILLA_LIBRARY_SUFFIX@.la	\
	$(REJILLA_GLIB_LIBS)		\
	$(REJILLA_GTHREAD_LIBS)				\
	$(REJILLA_GIO_LIBS)		\
	$(REJILLA_GSTREAMER_LIBS)	\
	$(REJILLA_GSTREAMER_BASE_LIBS)	\
	$(REJILLA_GTK_LIBS)		\
	$(REJILLA_GMODULE_LIBS)		\
	$(REJILLA_DBUS_LIBS)		\
	$(REJILLA_LIBXML_LIBS)		\
	$(REJILLA_LIBUNIQUE_LIBS) 	\
	$(REJILLA_SEARCH_LIBS)		\
	$(REJILLA_PL_PARSER_LIBS)	\
	$(REJILLA_SM_LIBS)

EXTRA_DIST =			\
	rejilla-marshal.list

CLEANFILES =			\
	$(RECMARSHALFILES)


-include $(top_srcdir)/git.mk
