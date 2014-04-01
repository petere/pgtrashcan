CREATE TABLE test1 (a int, b text);
CREATE TABLE test2 (a int, b text);

DROP TABLE test1;

\dt public.*
\dt "Trash".*

DROP TABLE test2;

\dt public.*
\dt "Trash".*

DROP TABLE "Trash".test1;

\dt "Trash".*

DROP SCHEMA "Trash" CASCADE;

\dn *rash*
