AM_CPPFLAGS = -I$(top_srcdir)/src/lib -I$(top_builddir)/src/lib
AM_CPPFLAGS += $(BOOST_INCLUDES)

AM_CXXFLAGS = $(BUNDY_CXXFLAGS)

if USE_STATIC_LINK
AM_LDFLAGS = -static
endif

CLEANFILES = *.gcno *.gcda

noinst_PROGRAMS = rdata_reader_bench rrset_render_bench

rdata_reader_bench_SOURCES = rdata_reader_bench.cc
rdata_reader_bench_LDADD = $(top_builddir)/src/lib/datasrc/memory/libdatasrc_memory.la
rdata_reader_bench_LDADD += $(top_builddir)/src/lib/exceptions/libbundy-exceptions.la
rdata_reader_bench_LDADD += $(top_builddir)/src/lib/util/libbundy-util.la
rdata_reader_bench_LDADD += $(top_builddir)/src/lib/dns/libbundy-dns++.la

rrset_render_bench_SOURCES = rrset_render_bench.cc
rrset_render_bench_LDADD = $(top_builddir)/src/lib/datasrc/memory/libdatasrc_memory.la
rrset_render_bench_LDADD += $(top_builddir)/src/lib/exceptions/libbundy-exceptions.la
rrset_render_bench_LDADD += $(top_builddir)/src/lib/util/libbundy-util.la
rrset_render_bench_LDADD += $(top_builddir)/src/lib/dns/libbundy-dns++.la
