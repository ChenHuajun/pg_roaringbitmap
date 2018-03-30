MODULE_big = roaringbitmap
OBJS =		\
			roaringbitmap.o \
			$(NULL)

EXTENSION = roaringbitmap
DATA =		\
			roaringbitmap--0.0.3.sql \
			$(NULL)

EXTRA_CLEAN += -r $(RPM_BUILD_ROOT)

PG_CPPFLAGS += -fPIC
roaringbitmap.o: override CFLAGS += -march=native -std=c99

ifdef DEBUG
COPT		+= -O0
CXXFLAGS	+= -g -O0
else
COPT		+= -O3
CXXFLAGS	+= -O3
endif

#SHLIB_LINK	+= -lstdc++

ifndef PG_CONFIG
PG_CONFIG = pg_config
endif

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)