PG_CONFIG = pg_config

MODULE_big = pgtrashcan
OBJS = pgtrashcan.o

REGRESS = test
REGRESS_OPTS = --inputdir=test

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
