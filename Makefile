MODULES = base32
EXTENSION = base32
DATA = base32--1.0.sql
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
