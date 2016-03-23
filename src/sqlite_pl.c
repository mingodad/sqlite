/*
** 2008 June 8
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This file contains routines used for analyzing procedures and
** for generating VDBE code that evaluates procedures in sqlite_pl_.
**
** 
*/
#ifdef SQLITE_WITH_PROCEDURAL_LANGUAGE

#include "sqliteInt.h"
#include <ctype.h>
#include "sqlite_pl.h"

/*************************************************************************
** 
** Routines to assist in building the parse tree for procedural blocks
*/

void sqlite_pl_StartPL_Block(
  Parse *pParse,    /* Parser context */
  int params		/* true if we are starting with parsing parameters */
){
}

PL_Block *sqlite_pl_EndPL_Block(
  Parse *pParse,     /* Parser context */
  PL_StmtList *pStList, /* Statements for this block */
  PL_StmtList *pExList  /* Exception handlers */
){
  return 0;
}

void sqlite_pl_PL_BlockDelete(PL_Block *pPL_Block){
}

/*
** Add a new local PL_Variable to the block currently being constructed.
**
** The parser calls this routine once for each PL_Variable declaration
** in a DECLARE ... BEGIN statement.  sqlite_pl_StartPL_Block() gets called
** first to get things going.  Then this routine is called for each
** PL_Variable.
*/
void sqlite_pl_AddProcVar(Parse *pParse, Token *pName){
}

/*
** This routine is called by the parser while in the middle of
** parsing a PL_Variable declaration in a procedure block.  The pFirst
** token is the first token in the sequence of tokens that describe
** the type of the PL_Variable currently under construction.   pLast is
** the last token in the sequence.  Use this information to
** construct a string that contains the typename of the PL_Variable
** and store that string in zType.
*/ 
void sqlite_pl_AddProcVarType(Parse *pParse, Token *pFirst, Token *pLast){
}

/*
** This routine is called by the parser while in the middle of
** parsing a PL_Variable declaration in a procedure block.  pExpr is
** the default value for the current PL_Variable (if any) and notnull
** is the flag signalling that his PL_Variable has NOT NULL as part of
** its type.
*/ 
void sqlite_pl_AddProcVarExpr(Parse *pParse, Expr *pExpr, int notnull){
}

/*
** Construct a new statement node and return a pointer to it.  Memory
** for this node is obtained from sqlite_pl_Malloc().  The calling function
** is responsible for making sure the node eventually gets freed.
*/
PL_Stmt* sqlite_pl_Stmt(
  Parse *pParse,
  int op,
  Expr *pExpr,
  PL_StmtList *pList1,
  PL_Stmt *pStmt2,
  PL_Block *pPL_Block
){
  return 0;
}

/*
** The parser calls this routine when it sees a SQL statement inside the
** body of a block
*/
PL_Stmt *sqlite_pl_PL_SQLStmt(
  int op,			        /* One of TK_SELECT, TK_INSERT, TK_UPDATE, TK_DELETE */
  Token *pTableName,  /* Name of the table into which we insert */
  IdList *pColumn,    /* List of columns in pTableName to insert into */
  ExprList *pEList,   /* The VALUE clause: a list of values to be inserted */
  Select *pSelect,    /* A SELECT statement that supplies values */
  Expr *pWhere,       /* The WHERE clause */
  int orconf          /* The conflict algorithm (OE_Abort, OE_Replace, etc.) */
){
  return 0;
}

/*
** Recursively delete a statement tree.
*/
void sqlite_pl_StmtDelete(PL_Stmt *p){
}

/*
** Add a new element to the end of a statement list.  If pList is
** initially NULL, then create a new statement list.
*/
PL_StmtList *sqlite_pl_PL_StmtListAppend(PL_StmtList *pList, PL_Stmt *pStmt){
  return 0;
}

/*
** Delete an entire expression list.
*/
void sqlite_pl_PL_StmtListDelete(PL_StmtList *pList){
}

/*************************************************************************
** 
** Routines to assist in compiling the parse tree into vdbe code
*/

/*
** Given the name of a PL_Variable, look up that name in the declarations of the
** current and enclosing blocks and make the pExpr expression node refer back
** to that PL_Variable's memory cell.  The following changes are made to pExpr:
**
**    pExpr->iColumn       Set to the number of the memory cell
**    pExpr->op            Set to TK_VAR.
**
** If the name cannot be resolved, leave an error message in pParse and return
** non-zero.  Return zero on success.
*/
int sqlite_pl_LookupVar(
  Parse *pParse,      /* The parsing context */
  PL_Block *pPL_Block,	    /* The current block */
  Expr *pExpr         /* Make this EXPR node point to the selected PL_Variable */
){
  return 0;
}

/*
** Recursively walk an expression tree and resolve names to memory
** cell numbers. For assignment expressions, the left espression is
** checked to be an assignable expression (currently only PL_Variable
** names are assignable)
*/
int sqlite_pl_ExprProcResolve(Parse *pParse, PL_Block *pPL_Block, Expr *pExpr){
  return 0;
}

int sqlite_pl_CompilePL_SQLStmt(Parse *pParse, PL_Block *b, PL_SQLStmt* pSql){
  return 0;
}

static int sqlite_pl_CompileCall(
  Parse *pParse,
  Token *pName,
  ExprList *pEList
) {
  return 0;
}

/* Hide a PL_Variable by setting its name to an empty string. This
** is used to hide the counter PL_Variable of a FOR loop at the end
** of the statement.
*/
static void pl_hideVar(PL_Block *b, int mVar) {
}

static int sqlite_pl_CompileList(Parse*, PL_Block*, PL_StmtList*, int*, int);
static int sqlite_pl_CompilePL_Block(Parse*, PL_Block*);

static int sqlite_pl_CompileStmt(
  Parse *pParse,    /* parse context */
  PL_Block *b,         /* current block */
  PL_Stmt* pStmt,      /* statement to compile */
  int *tailgoto,    /* set *tailgoto to 1 if last statement is a goto */
  int in_excep      /* set to 1 when compiling an exception handler */
){
  return 0;
}

static int sqlite_pl_CompileList(
  Parse *pParse,    /* parse context */
  PL_Block *pPL_Block,    /* current block */
  PL_StmtList *pList,  /* statements to compile */
  int *tailgoto,    /* set *tailgoto to 1 if last statment is a goto */
  int in_excep      /* set to 1 when compiling an exception handler */
){
  return 0;
}

static void sqlite_pl_OneHandler(Expr *pExpr, int lbl, int rev, Vdbe *v) {
}

static int sqlite_pl_CompileHandlers(Parse *pParse, PL_Block *pPL_Block, PL_StmtList *pExList){
  return 0;
}

#include "vdbeInt.h"

static int sqlite_pl_CompilePL_Block(Parse *pParse, PL_Block *b){
  return 0;
}

/*************************************************************************
** 
** Routines to assist in building / compiling CREATE PROCEDURE, FUNCTION
** and DROP PROCEDURE, FUNCTION
*/

void sqlite_pl_BeginProc(
  Parse *pParse,      /* The parse context of the statement */
  int what,           /* One of TK_PROCEDURE or TK_FUNCTION */
  Token *pName        /* The name of the object */
){
}

void sqlite_pl_DeletePL_Object(PL_Object *pPL_Object) {
}

/*
** This routine is called after the body of the procedure has been parsed
** in order to complete the process of building the procedure object.
*/
void sqlite_pl_FinishProc(
  Parse *pParse,          /* Parser context */
  PL_Block *pPL_Block,		  /* The procedure body */
  Token *pAll             /* Token that describes the complete CREATE text */
){
}

void sqlite_pl_DropProc(Parse *pParse, Token *pName){
}

/*************************************************************************
** 
** Routines to assist in building / compiling EXEC commands
*/

void sqlite_pl_ExecProc(Parse *pParse, Token *pName, ExprList *pEList) {
  return;
}

void sqlite_pl_ExecPL_Block(Parse *pParse, PL_Block *pPL_Block) {
}

#endif //SQLITE_WITH_PROCEDURAL_LANGUAGE
