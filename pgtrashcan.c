#include <postgres.h>
#if PG_VERSION_NUM >= 90300
#include <access/htup_details.h>
#endif
#include <access/xact.h>
#include <catalog/namespace.h>
#include <catalog/pg_database.h>
#include <catalog/pg_namespace.h>
#include <miscadmin.h>
#include <tcop/utility.h>
#include <utils/rel.h>
#include <utils/syscache.h>

PG_MODULE_MAGIC;


static ProcessUtility_hook_type prev_ProcessUtility = NULL;
#if PG_VERSION_NUM >= 90300
static void pgtrashcan_ProcessUtility(Node *parsetree,
									  const char *queryString,
									  ProcessUtilityContext context,
									  ParamListInfo params,
									  DestReceiver *dest,
									  char *completionTag);
#else
static void pgtrashcan_ProcessUtility(Node *parsetree,
									  const char *queryString,
									  ParamListInfo params,
									  bool isTopLevel,
									  DestReceiver *dest,
									  char *completionTag);
#endif

static const char *trashcan_nspname = "Trash";

void _PG_init(void);


void
_PG_init(void)
{
	prev_ProcessUtility = ProcessUtility_hook;
	if (!prev_ProcessUtility)
		prev_ProcessUtility = standard_ProcessUtility;
	ProcessUtility_hook = pgtrashcan_ProcessUtility;
}


static RangeVar *
makeRangeVarFromAnyName(List *names)
{
	RangeVar *r = makeNode(RangeVar);

	switch (list_length(names))
	{
		case 1:
			r->catalogname = NULL;
			r->schemaname = NULL;
			r->relname = strVal(linitial(names));
			break;
		case 2:
			r->catalogname = NULL;
			r->schemaname = strVal(linitial(names));
			r->relname = strVal(lsecond(names));
			break;
		case 3:
			r->catalogname = strVal(linitial(names));;
			r->schemaname = strVal(lsecond(names));
			r->relname = strVal(lthird(names));
			break;
		default:
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("improper qualified name (too many dotted names): %s",
							NameListToString(names))));
			break;
	}

#if PG_VERSION_NUM >= 90100
	r->relpersistence = RELPERSISTENCE_PERMANENT;
#endif
	r->location = -1;

	return r;
}


static void
create_trashcan_schema(void)
{
	HeapTuple   tuple;
	Oid			datdba;

	if (SearchSysCacheExists1(NAMESPACENAME, PointerGetDatum(trashcan_nspname)))
		return;

	tuple = SearchSysCache1(DATABASEOID, ObjectIdGetDatum(MyDatabaseId));
	if (!HeapTupleIsValid(tuple))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_DATABASE),
				 errmsg("database with OID %u does not exist", MyDatabaseId)));

	datdba = ((Form_pg_database) GETSTRUCT(tuple))->datdba;
	ReleaseSysCache(tuple);

	NamespaceCreate(trashcan_nspname, datdba, false);

	CommandCounterIncrement();
}


#if PG_VERSION_NUM >= 90300
static void
pgtrashcan_ProcessUtility(Node *parsetree,
						  const char *queryString,
						  ProcessUtilityContext context,
						  ParamListInfo params,
						  DestReceiver *dest,
						  char *completionTag)
#else
static void
pgtrashcan_ProcessUtility(Node *parsetree,
						  const char *queryString,
						  ParamListInfo params,
						  bool isTopLevel,
						  DestReceiver *dest,
						  char *completionTag)
#endif
{
	if (nodeTag(parsetree) == T_DropStmt)
	{
		DropStmt *stmt = (DropStmt *) parsetree;

		if (stmt->removeType == OBJECT_TABLE)
		{
			RangeVar *r;
			AlterObjectSchemaStmt *newstmt = makeNode(AlterObjectSchemaStmt);
			newstmt->objectType = stmt->removeType;
			newstmt->newschema = pstrdup(trashcan_nspname);
#if PG_VERSION_NUM >= 90200
			newstmt->missing_ok = stmt->missing_ok;
#endif
			if (stmt->behavior != DROP_RESTRICT)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("trash can does not support DROP CASCADE")));

			r = makeRangeVarFromAnyName(linitial(stmt->objects));
			r->inhOpt = INH_YES;
			r->alias = NULL;
			newstmt->relation = r;

			if (!r->schemaname || strcmp(r->schemaname, trashcan_nspname) != 0)
			{
				parsetree = (Node *) newstmt;
				create_trashcan_schema();
			}
		}
	}

#if PG_VERSION_NUM >= 90300
	(*prev_ProcessUtility) (parsetree, queryString,	context, params, dest, completionTag);
#else
	(*prev_ProcessUtility) (parsetree, queryString,	params, isTopLevel, dest, completionTag);
#endif
}
