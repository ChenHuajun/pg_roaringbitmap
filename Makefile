EXTENSION = roaringbitmap
TESTS        = $(wildcard sql/*.sql)
REGRESS      = $(patsubst sql/%.sql,%,$(TESTS))

MODULE_big = roaringbitmap
OBJS = roaringbitmap.o

roaringbitmap.o: override CFLAGS += -march=native -std=c99

PG_CONFIG = pg_config

DATA = $(wildcard *--*.sql)
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)