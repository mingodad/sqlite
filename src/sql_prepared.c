/*
** 2015 September 06
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This file contains code used to help implement prepared statements.
** It try to follow the postgres specification 
** http://www.postgresql.org/docs/9.5/static/sql-prepare.html
*/
#ifndef SQLITE_OMIT_SQL_PREPARED
#include "sqliteInt.h"

void sqlite3SqlPreparedBeginParse(
  Parse *pParse,        /* Parsing context */
  Token *pName,        /* Name of new prepared statement */
  int ifNotExists       /* No error if the prepared statement already exists */
){
}

void sqlite3SqlPreparedFinishParse(Parse *pParse, Token *pEnd, SqlPreparedStep *pStepList){
}

void sqlite3SqlPreparedArgInit(Parse *pParse){
}

void sqlite3SqlPreparedArgExtend(Parse *pParse, Token *p){
}

void sqlite3SqlPreparedExecute(Parse *pParse, Token *pName){
}

void sqlite3SqlPreparedDeallocate(Parse *pParse, Token *pName, int ifExists){
}

void sqlite3SqlPreparedDeletePreparedStep(sqlite3 *db, SqlPreparedStep *pSqlPreparedStep){
}

SqlPreparedStep *sqlite3SqlPreparedUpdateStep(
  sqlite3 *db,         /* The database connection */
  Token *pTableName,   /* Name of the table to be updated */
  ExprList *pEList,    /* The SET clause: list of column and new values */
  Expr *pWhere,        /* The WHERE clause */
  u8 orconf            /* The conflict algorithm. (OE_Abort, OE_Ignore, etc) */
){
  return 0;
}

SqlPreparedStep *sqlite3SqlPreparedInsertStep(
  sqlite3 *db,        /* The database connection */
  Token *pTableName,  /* Name of the table into which we insert */
  IdList *pColumn,    /* List of columns in pTableName to insert into */
  Select *pSelect,    /* A SELECT statement that supplies values */
  u8 orconf           /* The conflict algorithm (OE_Abort, OE_Replace, etc.) */
){
  return 0;
}

SqlPreparedStep *sqlite3SqlPreparedDeleteStep(
  sqlite3 *db,            /* Database connection */
  Token *pTableName,      /* The table from which rows are deleted */
  Expr *pWhere            /* The WHERE clause */
){
  return 0;
}

SqlPreparedStep *sqlite3SqlPreparedSelectStep(sqlite3 *db, Select *pSelect){
  return 0;
}

#endif /* SQLITE_OMIT_SQL_PREPARED */