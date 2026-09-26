EXTENSION = roaringbitmap
# roaringbitmap_upgrade.sql must run last: it drops and reinstalls the extension
# to exercise the 1.2 -> 1.3 upgrade path.
TESTS        = sql/roaringbitmap.sql sql/roaringbitmap64.sql sql/roaringbitmap_upgrade.sql
REGRESS      = $(patsubst sql/%.sql,%,$(TESTS))

MODULE_big = roaringbitmap
OBJS = roaring_buffer_reader.o roaringbitmap.o roaring_group_by_source.o roaring64_buffer_reader.o roaringbitmap64.o roaring64_group_by_source.o

$(OBJS): override CFLAGS += -std=c11 -Wno-error=maybe-uninitialized \
	-Wno-declaration-after-statement -Wno-missing-prototypes -Wno-missing-variable-declarations

PG_CONFIG = pg_config

DATA = $(wildcard *--*.sql)
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
