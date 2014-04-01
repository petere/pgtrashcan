# :put_litter_in_its_place: PostgreSQL Trash Can :put_litter_in_its_place:

This is a PostgreSQL plugin that implements a trash
can/wastebasket/rubbish bin/recycling container.  This allows
desktop-minded users to drop tables willy-nilly while giving them the
warm and fuzzy feeling of knowing that their data is still there.  Now
they only need to think of "vacuum" as "disk defragmentation", and
they'll feel right at home.

## Installation

This plugin supports PostgreSQL 9.0 and later.

To build and install it, run:

    make
    make install

You can choose a different PostgreSQL installation in the usual way:

    make PG_CONFIG=/else/where/pg_config
    make install PG_CONFIG=/else/where/pg_config

To activate the module, add the following setting to `postgresql.conf`:

    shared_preload_libraries = 'pgtrashcan'

This requires a server restart.

Alternatively, the module can also be loaded by
`local_preload_libraries` if appropriate preparations are made.

In PostgreSQL 9.4 or later, `session_preload_libraries` can also be
used.

## Use

Whenever a `DROP TABLE` command is executed, it will actually only
move the table to a schema named "Trash".  To actually delete the
table permanently, either drop again it from the Trash schema, or drop
the Trash schema altogether (it will be recreated as needed).  Note
that only tables are covered; other objects will be dropped normally.

Note: In case it isn't obvious, this whole thing is a bit silly, but
it works, so go for it.
