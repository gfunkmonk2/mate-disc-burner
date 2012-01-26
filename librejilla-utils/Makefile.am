
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
	$(REJILLA_GIO_CFLAGS)						\
	$(REJILLA_PL_PARSER_CFLAGS)					\
	$(REJILLA_GSTREAMER_CFLAGS)					\
	$(REJILLA_GSTREAMER_BASE_CFLAGS)				\
	$(REJILLA_GTK_CFLAGS)


lib_LTLIBRARIES = \
	librejilla-utils@REJILLA_LIBRARY_SUFFIX@.la

librejilla_utils@REJILLA_LIBRARY_SUFFIX@_la_LIBADD =					\
	$(REJILLA_GLIB_LIBS)					\
	$(REJILLA_GIO_LIBS)		\
	$(REJILLA_GSTREAMER_LIBS)	\
	$(REJILLA_GSTREAMER_BASE_LIBS)	\
	$(REJILLA_PL_PARSER_LIBS)	\
	$(REJILLA_GTK_LIBS)

librejilla_utils@REJILLA_LIBRARY_SUFFIX@_la_LDFLAGS =					\
	-version-info $(LIBREJILLA_LT_VERSION)			\
	-no-undefined
#	-export-symbols ./librejilla-utils.symbols

librejilla_utils@REJILLA_LIBRARY_SUFFIX@_la_SOURCES =		\
	rejilla-disc-message.c        \
	rejilla-disc-message.h        \
	rejilla-misc.c        \
	rejilla-misc.h        \
	rejilla-notify.c        \
	rejilla-notify.h        \
	rejilla-jacket-background.c        \
	rejilla-jacket-background.h        \
	rejilla-jacket-buffer.c        \
	rejilla-jacket-buffer.h        \
	rejilla-jacket-edit.c        \
	rejilla-jacket-edit.h        \
	rejilla-jacket-font.c        \
	rejilla-jacket-font.h        \
	rejilla-jacket-view.c        \
	rejilla-jacket-view.h        \
	rejilla-tool-color-picker.c        \
	rejilla-tool-color-picker.h        \
	rejilla-async-task-manager.c        \
	rejilla-async-task-manager.h        \
	rejilla-io.c        \
	rejilla-io.h        \
	rejilla-metadata.c        \
	rejilla-metadata.h        \
	rejilla-pk.c        \
	rejilla-pk.h

# EXTRA_DIST =			\
#	librejilla-utils.symbols

CLEANFILES =			\
	$(RECMARSHALFILES)

-include $(top_srcdir)/git.mk
