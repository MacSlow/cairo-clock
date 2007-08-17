bin_PROGRAMS = cairo_clock
transform = s/_/-/g

cairo_clock_LDFLAGS = -Wl, --export-dynamic

cairo_clock_LDADD =	\
	$(GLIB_LIBS)	\
	$(GTK_LIBS)	\
	$(GTK_DEP_LIBS)		 

cairo_clock_CFLAGS =			\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include		\
	-DG_LOG_DOMAIN=\"CairoClock\"	\
	$(GLIB_CFLAGS)			\
	$(GTK_CFLAGS) $(GTK_DEP_CFLAGS)	\
	$(CLOCK_DEFINES)		\
	-D_XOPEN_SOURCE=600		\
	-DCAIROCLOCKLOCALEDIR=\""$(cairoclocklocaledir)"\"

cairo_clock_SOURCES = \
	cairo-clock.c
