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
** for generating VDBE code that evaluates procedures in sqlite3.
**
** 
*/
#include "sqliteInt.h"
#include <ctype.h>

/*************************************************************************
** 
** Routines to assist in building the parse tree for procedural blocks
*/

void sqlite3StartBlock(
  Parse *pParse,    /* Parser context */
  int params		/* true if we are starting with parsing parameters */
){
  SP_Block *pBlock;

  pBlock = sqlite3Malloc( sizeof(SP_Block) );
  if( pBlock==0 ) return;

  pBlock->params = params;
  pBlock->pParent = pParse->pCurrentBlock;
  if( pBlock->pParent ) {
  	pBlock->mReturn = pBlock->pParent->mReturn;
  }
  else {
	  pBlock->mReturn = pParse->nMem++;
  }
  pParse->pCurrentBlock = pBlock;
}

SP_Block *sqlite3EndBlock(
  Parse *pParse,     /* Parser context */
  SP_StmtList *pStList, /* Statements for this block */
  SP_StmtList *pExList  /* Exception handlers */
){
  SP_Block *pBlock;
  int i;

  pBlock = pParse->pCurrentBlock;
  if( pBlock==0 ) return 0;

  for(i=0; i<pBlock->nVar; i++) {
	  if( pBlock->aVar[i].notNull && pBlock->aVar[i].pDflt==0 ){
      sqlite3ErrorMsg(pParse, "no default for variable: %s", pBlock->aVar[i].zName);
	  }
  }

  pBlock->pStList = pStList;
  pBlock->pExList = pExList;
  pParse->pCurrentBlock = pBlock->pParent;

  return pBlock;
}

void sqlite3BlockDelete(sqlite3 *db, SP_Block *pBlock){
  int i;

  if( pBlock==0 ) return;
  for( i=0; i<pBlock->nVar; i++ ) {
   sqlite3_free( pBlock->aVar[i].zName );
  }
  sqlite3_free( pBlock->aVar );
  sqlite3StmtListDelete(db, pBlock->pStList );
  sqlite3StmtListDelete(db, pBlock->pExList );
  sqlite3_free( pBlock );
}

/*
** Add a new local variable to the block currently being constructed.
**
** The parser calls this routine once for each variable declaration
** in a DECLARE ... BEGIN statement.  sqlite3StartBlock() gets called
** first to get things going.  Then this routine is called for each
** variable.
*/
void sqlite3AddProcVar(Parse *pParse, Token *pName){
  SP_Block *p;
  int i;
  char *z = 0;
  SP_Variable *pVar;

  if( (p = pParse->pCurrentBlock)==0 ) return;
  sqlite3SetNString(&z, pName->z, pName->n, 0);
  if( z==0 ) return;
  sqlite3Dequote(z);
  for(i=0; i<p->nVar; i++){
    if( strcmp(z, p->aVar[i].zName)==0 ){
      sqlite3ErrorMsg(pParse, "duplicate variable name: %s", z);
      sqlite3_free(z);
      return;
    }
  }
  if( (p->nVar & 0x7)==0 ){
    SP_Variable *aNew;
    aNew = sqlite3Realloc( p->aVar, (p->nVar+8)*sizeof(p->aVar[0]));
    if( aNew==0 ) return;
    p->aVar = aNew;
  }
  pVar = &p->aVar[p->nVar];
  memset(pVar, 0, sizeof(p->aVar[0]));
  pVar->zName = z;
  pVar->mVar = pParse->nMem++;
  pVar->isParam = p->params;
  p->nVar++;
}

/*
** This routine is called by the parser while in the middle of
** parsing a variable declaration in a procedure block.  The pFirst
** token is the first token in the sequence of tokens that describe
** the type of the variable currently under construction.   pLast is
** the last token in the sequence.  Use this information to
** construct a string that contains the typename of the variable
** and store that string in zType.
*/ 
void sqlite3AddProcVarType(Parse *pParse, Token *pFirst, Token *pLast){
  SP_Block *p;
  int i, j;
  int n;
  char *z, **pz;
  SP_Variable *pVar;

  if( (p = pParse->pCurrentBlock)==0 ) return;
  i = p->nVar-1;
  if( i<0 ) return;
  pVar = &p->aVar[i];
  pz = &pVar->zType;
  n = pLast->n + Addr(pLast->z) - Addr(pFirst->z);
  sqlite3SetNString(pz, pFirst->z, n, 0);
  z = *pz;
  if( z==0 ) return;
  for(i=j=0; z[i]; i++){
    int c = z[i];
    if( isspace(c) ) continue;
    z[j++] = c;
  }
  z[j] = 0;
}

/*
** This routine is called by the parser while in the middle of
** parsing a variable declaration in a procedure block.  pExpr is
** the default value for the current variable (if any) and notnull
** is the flag signalling that his variable has NOT NULL as part of
** its type.
*/ 
void sqlite3AddProcVarExpr(Parse *pParse, Expr *pExpr, int notnull){
  SP_Block *b;
  int i;

  if( (b = pParse->pCurrentBlock)==0 ) return;
  i = b->nVar-1;
  if( i<0 ) return;
  if( pExpr ) {
    b->aVar[i].pDflt = pExpr;
  }
  if( notnull ) {
	b->aVar[i].notNull = 1;
  }
}

/*
** Construct a new statement node and return a pointer to it.  Memory
** for this node is obtained from sqlite3Malloc().  The calling function
** is responsible for making sure the node eventually gets freed.
*/
SP_Stmt* sqlite3Stmt(
  Parse *pParse,
  int op,
  Expr *pExpr,
  SP_StmtList *pList1,
  SP_Stmt *pStmt2,
  SP_Block *pBlock
){
  SP_Stmt *pNew;
  pNew = sqlite3Malloc( sizeof(SP_Stmt) );
  if( pNew==0 ){
    /* When malloc fails, we leak memory */
    return 0;
  }
  pNew->op = op;
  pNew->pExpr1 = pExpr;
  pNew->pStmt1 = pList1;
  pNew->pStmt2 = pStmt2;
  pNew->pBlock = pBlock;

  return pNew;
}

/*
** The parser calls this routine when it sees a SQL statement inside the
** body of a block
*/
SP_Stmt *sqlite3SQLStmt(
  int op,			        /* One of TK_SELECT, TK_INSERT, TK_UPDATE, TK_DELETE */
  Token *pTableName,  /* Name of the table into which we insert */
  IdList *pColumn,    /* List of columns in pTableName to insert into */
  ExprList *pEList,   /* The VALUE clause: a list of values to be inserted */
  Select *pSelect,    /* A SELECT statement that supplies values */
  Expr *pWhere,       /* The WHERE clause */
  int orconf          /* The conflict algorithm (OE_Abort, OE_Replace, etc.) */
){
  SP_Stmt *pNew;

  pNew = sqlite3Malloc( sizeof(SP_Stmt)+sizeof(SP_SQLStmt) );
  if( pNew==0 ){
    /* When malloc fails, we leak memory */
    return 0;
  }
  pNew->pSql = (SP_SQLStmt*) (pNew+1);

  pNew->op = TK_SQL;
  pNew->pSql->op		  = op;
  pNew->pSql->pSelect   = pSelect;
  if( pTableName ) {
    pNew->pSql->target    = *pTableName;
  }
  pNew->pSql->pIdList   = pColumn;
  pNew->pSql->pExprList = pEList;
  pNew->pSql->pWhere    = pWhere;
  pNew->pSql->orconf	  = orconf;

  return pNew;
}

/*
** Recursively delete a statement tree.
*/
void sqlite3StmtDelete(sqlite3 *db, SP_Stmt *p){
  if( p==0 ) return;
  if( p->pExpr1 ) sqlite3ExprDelete(db, p->pExpr1);
  if( p->pStmt1 ) sqlite3StmtListDelete(db, p->pStmt1);
  if( p->pStmt2 ) sqlite3StmtDelete(db, p->pStmt2);
  if( p->pBlock ) sqlite3BlockDelete(db, p->pBlock);
}

/*
** Add a new element to the end of a statement list.  If pList is
** initially NULL, then create a new statement list.
*/
SP_StmtList *sqlite3StmtListAppend(SP_StmtList *pList, SP_Stmt *pStmt){
  if( pList==0 ){
    pList = sqlite3Malloc( sizeof(SP_StmtList) );
    if( pList==0 ){
      /* sqlite3StmtDelete(pExpr); // Leak memory if malloc fails */
      return 0;
    }
    assert( pList->nAlloc==0 );
  }
  if( pList->nAlloc<=pList->nStmt ){
    pList->nAlloc = pList->nAlloc*2 + 4;
    pList->a = sqlite3Realloc(pList->a, pList->nAlloc*sizeof(pList->a[0]));
    if( pList->a==0 ){
      /* sqlite3StmtDelete(pExpr); // Leak memory if malloc fails */
      pList->nStmt = pList->nAlloc = 0;
      return pList;
    }
  }
  assert( pList->a!=0 );
  if( pStmt ){
    struct SP_StmtList_item *pItem = &pList->a[pList->nStmt++];
    memset(pItem, 0, sizeof(*pItem));
    pItem->pStmt = pStmt;
  }
  return pList;
}

/*
** Delete an entire expression list.
*/
void sqlite3StmtListDelete(sqlite3 *db, SP_StmtList *pList){
  int i;
  if( pList==0 ) return;
  assert( pList->a!=0 || (pList->nStmt==0 && pList->nAlloc==0) );
  assert( pList->nStmt<=pList->nAlloc );
  for(i=0; i<pList->nStmt; i++) {
    sqlite3StmtDelete(db, pList->a[i].pStmt);
  }
  sqlite3_free(pList->a);
  sqlite3_free(pList);
}

/*************************************************************************
** 
** Routines to assist in compiling the parse tree into vdbe code
*/

/*
** Given the name of a variable, look up that name in the declarations of the
** current and enclosing blocks and make the pExpr expression node refer back
** to that variable's memory cell.  The following changes are made to pExpr:
**
**    pExpr->iColumn       Set to the number of the memory cell
**    pExpr->op            Set to TK_VAR.
**
** If the name cannot be resolved, leave an error message in pParse and return
** non-zero.  Return zero on success.
*/
int sqlite3LookupVar(
  Parse *pParse,      /* The parsing context */
  SP_Block *pBlock,	    /* The current block */
  Expr *pExpr         /* Make this EXPR node point to the selected variable */
){
  SP_Block *b = pBlock;
  char *zVar;

  assert( pExpr->op==TK_ID || pExpr->op==TK_STRING );
  assert( pExpr->pLeft==0 && pExpr->pRight==0 );

  zVar = sqlite3StrNDup(pExpr->token.z, pExpr->token.n);
  sqlite3Dequote(zVar);
  while( b ) {
	  int i;
	  for( i=0; i<b->nVar; i++ ) {
	    if( !strcmp(b->aVar[i].zName, zVar) ) {
          pExpr->iColumn  = b->aVar[i].mVar;
		  pExpr->op = TK_VAR;
		  if( b->aVar[i].notNull ){
		    pExpr->flags = EP_NotNull;
		  }
	      sqlite3_free(zVar);
		  return 0;
	    }
	  }
    b = b->pParent;    
  }
  sqlite3ErrorMsg(pParse, "Variable %s not declared", zVar);
  sqlite3_free(zVar);
  return 1;
}

/*
** Recursively walk an expression tree and resolve names to memory
** cell numbers. For assignment expressions, the left espression is
** checked to be an assignable expression (currently only variable
** names are assignable)
*/
int sqlite3ExprProcResolve(Parse *pParse, SP_Block *pBlock, Expr *pExpr){
  if( pExpr==0 || pBlock==0 ) return 0;
  switch( pExpr->op ){
    /* Double-quoted strings (ex: "abc") are used as identifiers if
    ** possible.  Otherwise they remain as strings.  Single-quoted
    ** strings (ex: 'abc') are always string literals.
    */
    case TK_STRING: {
      if( pExpr->token.z[0]=='\'' ) break;
      /* Fall thru into the TK_ID case if this is a double-quoted string */
    }
    /* A lone identifier is the name of a variable.
    */
    case TK_ID: {
      if( sqlite3LookupVar(pParse, pBlock, pExpr) ){
        return 1;
      }
      break; 
    }
  
    /* A dotted name ("X.Y.Z") is not yet allowed in procedural code
    */
    case TK_DOT: {
      sqlite3ErrorMsg(pParse, "Dotted variable name not allowed yet", 0);
      return 1;
    }

    /* An assignment expression must have a ID node on its left
    */
    case TK_ASSIGN: {
      if( pExpr->pLeft->op!=TK_ID ){
        sqlite3ErrorMsg(pParse, "Bad lvalue in assignment", 0);
        return 1;
      }
      /* fall through */
    }

	/* For all else, just recursively walk the tree */
    default: {
      if( pExpr->pLeft
      && sqlite3ExprProcResolve(pParse, pBlock, pExpr->pLeft) ){
        return 1;
      }
      if( pExpr->pRight 
      && sqlite3ExprProcResolve(pParse, pBlock, pExpr->pRight) ){
        return 1;
      }
      if( pExpr->pList ){
        int i;
        ExprList *pList = pExpr->pList;
        for(i=0; i<pList->nExpr; i++){
          Expr *pArg = pList->a[i].pExpr;
          if( sqlite3ExprProcResolve(pParse, pBlock, pArg) ){
            return 1;
          }
        }
      }
    }
  }
  return 0;
}

int sqlite3CompileSQLStmt(Parse *pParse, SP_Block *b, SP_SQLStmt* pSql){
  Vdbe *v = sqlite3GetVdbe(pParse);
  int i,j;

  switch( pSql->op ){
  case TK_SELECT: {
	assert(pSql->pSelect);
	assert(pSql->pSelect->pSrc);
	sqlite3Select(pParse, pSql->pSelect, SRT_Stack, 0, 0, 0, 0);
	if( pSql->pExprList->nExpr!=pSql->pSelect->pEList->nExpr ) {
      sqlite3ErrorMsg(pParse, "INTO list does not match column list", 0);
	  return 1;
	}
	for(i=0; i<pSql->pExprList->nExpr; i++) {
	  Expr *e = pSql->pExprList->a[i].pExpr;
	  if( e->op!=TK_ID ) {
      sqlite3ErrorMsg(pParse, "Bad lvalue in INTO list", 0);
  		return 1;
	  }
      if( sqlite3ExprProcResolve(pParse, b, e) ){
        return 1;
	  }
 	  assert( e->op==TK_VAR );
	  if( e->flags==EP_NotNull ){
      j = sqlite3VdbeMakeLabel(v);
      sqlite3VdbeAddOp(v, OP_NotNull, -1, i);
	    sqlite3VdbeOp3(v, OP_Halt, sqlite3_CONSTRAINT, OE_Abort,
                         "attempt to store null in non-null var", P3_STATIC);
      sqlite3VdbeResolveLabel(v, j);
	  }
      sqlite3VdbeAddOp(v, OP_MemStore, e->iColumn, 1);
	}
	break;
  }
  case TK_UPDATE: {
    SrcList *pSrc;
    pSrc = sqlite3SrcListAppend(db, 0, &pSql->target, 0);
    sqlite3Update(pParse, pSrc, 0, pSql->pExprList, pSql->pWhere, pSql->orconf);
    break;
  }
  case TK_INSERT: {
    SrcList *pSrc;
    pSrc = sqlite3SrcListAppend(db, 0, &pSql->target, 0);
    sqlite3Insert(pParse, pSrc, pSql->pExprList, pSql->pSelect, pSql->pIdList, pSql->orconf);
    break;
  }
  case TK_DELETE: {
    SrcList *pSrc;
    pSrc = sqlite3SrcListAppend(db, 0, &pSql->target, 0);
    sqlite3DeleteFrom(pParse, pSrc, 0, pSql->pWhere);
    break;
  }
  default:
    assert(0);
  } 

  return 0;
}

static int sqlite3CompileCall(
  Parse *pParse,
  Token *pName,
  ExprList *pEList
) {
  char *zName = 0;
  Vdbe *v = sqlite3GetVdbe(pParse);
  SP_Block *b = pParse->pCurrentBlock;
  SP_Object * pObj = 0;
  sqlite3 *db = pParse->db;
  int i, nActual = 0;

  /* Check that the object exist & get its Object pointer*/
  zName = sqlite3StrNDup(pName->z, pName->n);
  sqlite3Dequote(zName);
  pObj = sqlite3HashFind(&(db->aDb[0].objectHash), zName,pName->n+1);
  if( !pObj ){
    sqlite3ErrorMsg(pParse, "object %T not found", pName);
    goto proc_cleanup;
  }
  if( pEList ) {
    nActual = pEList->nExpr;
  }
  if( pObj->nParam!=nActual ) {
  	sqlite3ErrorMsg(pParse, "bad parameter count for object %T", pName);
    goto proc_cleanup;
  }

  for(i=0; i<nActual; i++) {
	  Expr *pExpr = pEList->a[i].pExpr;
    if( sqlite3ExprProcResolve(pParse, b, pExpr) ){
      goto proc_cleanup;
    }
    if( sqlite3ExprCheck(pParse, pExpr, 0, 0) ){
      goto proc_cleanup;
    }
    sqlite3ExprCode(pParse, pExpr);
  }
  sqlite3VdbeOp3(v, OP_Exec, nActual, 0, zName, P3_DYNAMIC);
  return 0;

proc_cleanup:
  sqlite3_free(zName);
  return 1;
}

/* Hide a variable by setting its name to an empty string. This
** is used to hide the counter variable of a FOR loop at the end
** of the statement.
*/
static void hideVar(SP_Block *b, int mVar) {
  int i;

  if( !b ) return;

  for( i=0; i<b->nVar; i++ ) {
	  if( b->aVar[i].mVar==mVar ) {
      b->aVar[i].zName[0] = 0;
      return;
    }
  }
}

static int sqlite3CompileList(Parse*, SP_Block*, SP_StmtList*, int*, int);
static int sqlite3CompileBlock(Parse*, SP_Block*);

static int sqlite3CompileStmt(
  Parse *pParse,    /* parse context */
  SP_Block *b,         /* current block */
  SP_Stmt* pStmt,      /* statement to compile */
  int *tailgoto,    /* set *tailgoto to 1 if last statement is a goto */
  int in_excep      /* set to 1 when compiling an exception handler */
){
  Vdbe *v = sqlite3GetVdbe(pParse);
  SrcList dummy;
  int i, j, skipgoto = 0;

  dummy.nSrc = 0;

  if( tailgoto ) *tailgoto = 0;

  if( pStmt->op!=TK_RAISE && pStmt->op!=TK_PROCEDURE && pStmt->pExpr1 ){
    Expr *pExpr = pStmt->pExpr1;
    if( pStmt->op==TK_FOR ) {
      /* allocate the FOR counter variable (see case TK_FOR below) */
      sqlite3AddProcVar(pParse, &(pExpr->pLeft->token));
    }
    if( sqlite3ExprProcResolve(pParse, b, pExpr) ){
      return 1;
    }
    if( sqlite3ExprCheck(pParse, pExpr, 0, 0) ){
      return 1;
    }
  }

  switch( pStmt->op ) {

  case TK_ASSIGN:{
    Expr *pLeft = pStmt->pExpr1->pLeft;
    Expr *pRight = pStmt->pExpr1->pRight;

    assert( pStmt->pExpr1->op==TK_ASSIGN );
	  assert( pLeft->op==TK_VAR );
    sqlite3ExprCode(pParse, pRight);
	  if( pLeft->flags==EP_NotNull ){
      i = sqlite3VdbeMakeLabel(v);
      sqlite3VdbeAddOp(v, OP_NotNull, -1, i);
	    sqlite3VdbeOp3(v, OP_Halt, sqlite3_CONSTRAINT, OE_Abort,
                           "attempt to store null in non-null var", P3_STATIC);
      sqlite3VdbeResolveLabel(v, i);
    }
    sqlite3VdbeAddOp(v, OP_MemStore, pLeft->iColumn, 1);
	  break;
  }

  case TK_BLOCK:{
    if( sqlite3CompileBlock(pParse, pStmt->pBlock) ){
      return 1;
    }
	  break;
  }

  case TK_CASE:{
    int jumpInst, addr;
    int nStmt;
    int searched;

    nStmt = pStmt->pStmt1->nStmt;
    searched = pStmt->pExpr1==0;
    assert( nStmt>0 );
    j = sqlite3VdbeMakeLabel(v);
    if( !searched ){
      sqlite3ExprCode(pParse, pStmt->pExpr1);
    }
    for(i=0; i<nStmt; i++){
      SP_Stmt *pWhen = pStmt->pStmt1->a[i].pStmt;
      assert( pWhen->op==TK_WHEN );
      if( sqlite3ExprProcResolve(pParse, b, pWhen->pExpr1) ){
        return 1;
      }
      if( sqlite3ExprCheck(pParse, pWhen->pExpr1, 0, 0) ){
        return 1;
      }
      sqlite3ExprCode(pParse, pWhen->pExpr1);
      if( !searched ){
        sqlite3VdbeAddOp(v, OP_Dup, 1, 1);
        jumpInst = sqlite3VdbeAddOp(v, OP_Ne, 1, 0);
      }else{
        jumpInst = sqlite3VdbeAddOp(v, OP_IfNot, 1, 0);
      }
	    if( sqlite3CompileList(pParse, b, pWhen->pStmt1, &skipgoto, 0) ){
	      return 1;
      }
      if( !skipgoto ) {
        sqlite3VdbeAddOp(v, OP_Goto, 0, j);
      }
      addr = sqlite3VdbeCurrentAddr(v);
      sqlite3VdbeChangeP2(v, jumpInst, addr);
    }
    if( !searched ){
      sqlite3VdbeAddOp(v, OP_Pop, 1, 0);
    }
    if( pStmt->pStmt2 ){
      assert( pStmt->pStmt2->op==TK_ELSE );
	    if( sqlite3CompileList(pParse, b, pStmt->pStmt2->pStmt1, tailgoto, 0) ){
	      return 1;
      }
    }else{
      sqlite3VdbeOp3(v, OP_Raise, 0, 0, "CASE_NOT_FOUND", P3_STATIC);
      if( tailgoto ) *tailgoto = 1;
    }
    sqlite3VdbeResolveLabel(v, j);
    break;
  }

  case TK_EXIT:{
    if( pParse->SP_iLoopExit==0 ) {
      sqlite3ErrorMsg(pParse, "EXIT used outside loop statement", 0);
      return 1;
    }
    if( pStmt->pExpr1 ) {
      sqlite3ExprCode(pParse, pStmt->pExpr1);
      sqlite3VdbeAddOp(v, OP_If, 1, pParse->SP_iLoopExit);
    } else {
      sqlite3VdbeAddOp(v, OP_Goto, 0, pParse->SP_iLoopExit);
      if( tailgoto ) *tailgoto = 1;
    }
	  break;
  }

  case TK_FOR:{
    Expr *pLow = pStmt->pExpr1->pRight->pLeft;
    Expr *pHigh = pStmt->pExpr1->pRight->pRight;
    int iCounter, iHigh, iPrevExit;

	  assert( pStmt->pExpr1->op==TK_ASSIGN );
	  assert( pStmt->pExpr1->pLeft->op==TK_VAR );
	  assert( pStmt->pExpr1->pRight->op==TK_FOR );
    iCounter = pParse->nMem-1;
    iHigh = pParse->nMem++;
    sqlite3ExprCode(pParse, pLow);
    sqlite3VdbeAddOp(v, OP_MemStore, iCounter, 1);
    sqlite3ExprCode(pParse, pHigh);
    sqlite3VdbeAddOp(v, OP_MemStore, iHigh, 1);
    sqlite3VdbeAddOp(v, OP_MemLoad, iCounter, 0);
    i = sqlite3VdbeCurrentAddr(v);
    sqlite3VdbeAddOp(v, OP_MemLoad, iHigh, 0);
    iPrevExit = pParse->SP_iLoopExit;
    pParse->SP_iLoopExit = sqlite3VdbeMakeLabel(v);
    sqlite3VdbeAddOp(v, OP_Gt, 1, pParse->SP_iLoopExit);
	  if( sqlite3CompileList(pParse, b, pStmt->pStmt1, 0, 0) ){
	    return 1;
    }
    sqlite3VdbeAddOp(v, OP_MemLoad, iCounter, 0);
    sqlite3VdbeAddOp(v, OP_Integer, 1, 0);
    sqlite3VdbeAddOp(v, OP_Add, 0, 0);
    sqlite3VdbeAddOp(v, OP_MemStore, iCounter, 0);
    sqlite3VdbeAddOp(v, OP_Goto, 0, i);
    sqlite3VdbeResolveLabel(v, pParse->SP_iLoopExit);
    pParse->SP_iLoopExit = iPrevExit;
    hideVar(b, iCounter);
    break;
  }

  case TK_IF: {
    i = sqlite3VdbeMakeLabel(v);
    j = sqlite3VdbeMakeLabel(v);
    sqlite3ExprCode(pParse, pStmt->pExpr1);
    sqlite3VdbeAddOp(v, OP_IfNot, 1, j);
	  if( sqlite3CompileList(pParse, b, pStmt->pStmt1, &skipgoto, 0) ){
	    return 1;
    }
    while( pStmt->pStmt2 ) {
      if( !skipgoto ) {
        sqlite3VdbeAddOp(v, OP_Goto, 0, i);
      }
      sqlite3VdbeResolveLabel(v, j);
      j = sqlite3VdbeMakeLabel(v);
      pStmt = pStmt->pStmt2;
      assert( pStmt->op==TK_ELSE || pStmt->op==TK_ELSIF );
      if( pStmt->op==TK_ELSIF ) {
        if( sqlite3ExprProcResolve(pParse, b, pStmt->pExpr1) ){
          return 1;
        }
        if( sqlite3ExprCheck(pParse, pStmt->pExpr1, 0, 0) ){
          return 1;
        }
        sqlite3ExprCode(pParse, pStmt->pExpr1);
        sqlite3VdbeAddOp(v, OP_IfNot, 1, j);
      }
	    if( sqlite3CompileList(pParse, b, pStmt->pStmt1, &skipgoto, 0) ){
	      return 1;
      }
    }
    sqlite3VdbeResolveLabel(v, i);
    sqlite3VdbeResolveLabel(v, j);
	  break;
  }

  case TK_LOOP:{
    int iPrevExit = pParse->SP_iLoopExit;
    pParse->SP_iLoopExit = sqlite3VdbeMakeLabel(v);
    i = sqlite3VdbeCurrentAddr(v);
	  if( sqlite3CompileList(pParse, b, pStmt->pStmt1, 0, 0) ){
	    return 1;
    }
    sqlite3VdbeAddOp(v, OP_Goto, 0, i);
    sqlite3VdbeResolveLabel(v, pParse->SP_iLoopExit);
    pParse->SP_iLoopExit = iPrevExit;
	  break;
  }

  case TK_NULL:{
	  break;
  }

  case TK_PRINT:{
    sqlite3ExprCode(pParse, pStmt->pExpr1);
    sqlite3VdbeAddOp(v, OP_Print, 0, 0);
	  break;
  }

  case TK_PROCEDURE: {
    Expr *pExpr = pStmt->pExpr1;
    if( sqlite3CompileCall(pParse, &(pExpr->token), pExpr->pList) ) {
      return 1;
    }
    sqlite3VdbeAddOp(v, OP_Pop, 1, 0);
	  break;
  }

  case TK_RAISE:{
    if( pStmt->pExpr1==0 ) {
      if( !in_excep ) {
        sqlite3ErrorMsg(pParse, "RAISE without argument illegal outside exception handler", 0);
        return 1;
      }
      sqlite3VdbeOp3(v, OP_Raise, 0, 0, 0, P3_STATIC);
    } else {
      char *zName = 0;
      sqlite3SetNString(&zName, pStmt->pExpr1->token.z, pStmt->pExpr1->token.n, 0);
      sqlite3VdbeOp3(v, OP_Raise, 0, 0, zName, P3_DYNAMIC);
    }
    if( tailgoto ) *tailgoto = 1;
	  break;
  }

  case TK_RETURN:{
    sqlite3ExprCode(pParse, pStmt->pExpr1);
    sqlite3VdbeAddOp(v, OP_MemStore, b->mReturn, 1);
    sqlite3VdbeAddOp(v, OP_Goto, 0, b->nExit);
    if( tailgoto ) *tailgoto = 1;
	  break;
  }

  case TK_SQL:{
	  sqlite3CompileSQLStmt(pParse, b, pStmt->pSql);
	  break;
  }

  case TK_WHILE:{
    int iPrevExit = pParse->SP_iLoopExit;
    pParse->SP_iLoopExit = sqlite3VdbeMakeLabel(v);
    i = sqlite3VdbeCurrentAddr(v);
    sqlite3ExprCode(pParse, pStmt->pExpr1);
    sqlite3VdbeAddOp(v, OP_IfNot, 1, pParse->SP_iLoopExit);
	  if( sqlite3CompileList(pParse, b, pStmt->pStmt1, 0, 0) ){
	    return 1;
    }
    sqlite3VdbeAddOp(v, OP_Goto, 0, i);
    sqlite3VdbeResolveLabel(v, pParse->SP_iLoopExit);
    pParse->SP_iLoopExit = iPrevExit;
	  break;
  }

  }
  return 0;
}

static int sqlite3CompileList(
  Parse *pParse,    /* parse context */
  SP_Block *pBlock,    /* current block */
  SP_StmtList *pList,  /* statements to compile */
  int *tailgoto,    /* set *tailgoto to 1 if last statment is a goto */
  int in_excep      /* set to 1 when compiling an exception handler */
){
  Vdbe *v = sqlite3GetVdbe(pParse);
  int i, n = pList->nStmt;

  for(i=0; i<n; i++){
    if( sqlite3CompileStmt(pParse, pBlock, pList->a[i].pStmt, tailgoto, in_excep) ){
      return 1;
    }
  }
  return 0;
}

static void sqlite3OneHandler(Expr *pExpr, int lbl, int rev, Vdbe *v) {
  char *zExcep = sqlite3StrNDup(pExpr->token.z, pExpr->token.n);

  assert( pExpr->op==TK_ID || pExpr->op==TK_STRING );
  sqlite3Dequote(zExcep);
  sqlite3VdbeOp3(v, OP_ExcepWhen, rev, lbl, zExcep, P3_DYNAMIC);
}

static int sqlite3CompileHandlers(Parse *pParse, SP_Block *pBlock, SP_StmtList *pExList){
  Vdbe *v = sqlite3GetVdbe(pParse);
  int i, n = pExList->nStmt, skiphalt = 0;

  for(i=0; i<n; i++){
    Stmt *pEWhen = pExList->a[i].pStmt;
    Expr *pExpr = pEWhen->pExpr1;
    int lbl1, lbl2;

    assert( pEWhen->op==TK_WHEN );
    if( pExpr ) {
      lbl2 = sqlite3VdbeMakeLabel(v);
      while( pExpr && pExpr->op==TK_OR ) {
        sqlite3OneHandler( pExpr->pRight, lbl2, 1, v );
        pExpr = pExpr->pLeft;
      }
      lbl1 = sqlite3VdbeMakeLabel(v);
      sqlite3OneHandler( pExpr, lbl1, 0, v );
      sqlite3VdbeResolveLabel(v, lbl2);
    } else {
      lbl1 = sqlite3VdbeMakeLabel(v);
      sqlite3VdbeOp3(v, OP_ExcepWhen, 0, lbl1, 0, P3_STATIC);
    }
    if( sqlite3CompileList(pParse, pBlock, pEWhen->pStmt1, &skiphalt, 1) ){
      return 1;
    }
    if( !skiphalt ) {
      sqlite3VdbeAddOp(v, OP_Goto, 0, pBlock->nExit);
    }
    sqlite3VdbeResolveLabel(v, lbl1);
  }
  /* if no handler caught the exception, reraise it */
  sqlite3VdbeOp3(v, OP_Raise, 0, 0, 0, P3_STATIC);
  return 0;
}

#include "vdbeInt.h"

static int sqlite3CompileBlock(Parse *pParse, SP_Block *b){
  Vdbe *v = sqlite3GetVdbe(pParse);
  SP_Block *saveCurBlock;
  int i, handler = 0;

  saveCurBlock = pParse->pCurrentBlock;
  pParse->pCurrentBlock = b;
  DbSetProperty(pParse->db, 0, DB_Cookie);

  b->nExit = sqlite3VdbeMakeLabel(v);
	if( b->pExList ) {
		handler = sqlite3VdbeMakeLabel(v);
    sqlite3VdbeAddOp(v, OP_NewHandler, 0, handler);
	}
  for(i=0; i<b->nVar; i++) {
	  if( b->aVar[i].pDflt!=0 ) {
	    Expr *pExpr = b->aVar[i].pDflt;
      if( sqlite3ExprProcResolve(pParse, b, pExpr) ){
        return 1;
	    }
      if( sqlite3ExprCheck(pParse, pExpr, 0, 0) ){
        return 1;
	    }
      sqlite3ExprCode(pParse, pExpr);
      sqlite3VdbeAddOp(v, OP_MemStore, b->aVar[i].mVar , 1);
    }
  }
  if( sqlite3CompileList(pParse, b, b->pStList, 0, 0) ){
    return 1;
  }
  if( b->pExList ) {
    sqlite3VdbeAddOp(v, OP_Goto, 0, b->nExit);
		sqlite3VdbeResolveLabel(v, handler);
		if( sqlite3CompileHandlers(pParse, b, b->pExList) ){
      return 1;
		}
  }
  sqlite3VdbeResolveLabel(v, b->nExit);
	if( b->pExList && b->pParent!=0 ) {
    sqlite3VdbeAddOp(v, OP_PrevHandler, 0, 0);
	}
  /* if we end with 'goto next' (last stmt is a return), remove it */
//  if( v->aOp[v->nOp-1].opcode==OP_Goto && v->aOp[v->nOp-1].p2==v->nOp ) {
//    v->nOp--;
//  }
  pParse->pCurrentBlock = saveCurBlock;
  return 0;
}

/*************************************************************************
** 
** Routines to assist in building / compiling CREATE PROCEDURE, FUNCTION
** and DROP PROCEDURE, FUNCTION
*/

void sqlite3BeginProc(
  Parse *pParse,      /* The parse context of the statement */
  int what,           /* One of TK_PROCEDURE or TK_FUNCTION */
  Token *pName        /* The name of the object */
){
  SP_Object *no;
  SP_Block *pBlock = pParse->pCurrentBlock;
  char *zName = 0;        /* Name of the object */
  sqlite3 *db = pParse->db;

  /* Check that the object name does not already exist */
  zName = sqlite3StrNDup(pName->z, pName->n);
  sqlite3Dequote(zName);
  if( !pParse->explain &&
	  sqlite3HashFind(&(db->aDb[0].objectHash), zName,pName->n+1) ){
    sqlite3ErrorMsg(pParse, "object %T already exists", pName);
    goto object_cleanup;
  }
  /* Build the object */
  no = (SP_Object*)sqlite3Malloc(sizeof(SP_Object));
  if( no==0 ) goto object_cleanup;
  no->name = zName;
  zName = 0;
  no->what = what;
  no->iDb = 0;
  no->nParam = pBlock->nVar;
  /* add param checks here */
  pBlock->pObj = no;
  pBlock->params = 0;
  assert( pParse->pNewTrigger==0 );
  pParse->pNewObject = no;
  return;

object_cleanup:
  sqlite3_free(zName);
}

void sqlite3DeleteObject(SP_Object *pObject) {
  int i;

  if( pObject==0 ) return;
  sqlite3_free(pObject->name);
  for(i=0; i<pObject->nOp; i++){
    if( pObject->aOp[i].p3type==P3_DYNAMIC ){
      sqlite3_free(pObject->aOp[i].p3);
    }
  }
  sqlite3_free(pObject->aOp);
  sqlite3_free(pObject);
}

/*
** This routine is called after the body of the procedure has been parsed
** in order to complete the process of building the procedure object.
*/
void sqlite3FinishProc(
  sqlite3 *db,
  Parse *pParse,          /* Parser context */
  SP_Block *pBlock,		  /* The procedure body */
  Token *pAll             /* Token that describes the complete CREATE text */
){
  SP_Object *no = 0;           /* The object whose construction is finishing up */
  sqlite3 *db = pParse->db;  /* The database */
  Vdbe *v = sqlite3GetVdbe(pParse);

  if( pParse->nErr || pParse->pNewObject==0 ) goto objectfinish_cleanup;
  no = pParse->pNewObject;
  pParse->pNewObject = 0;

  sqlite3CompileBlock(pParse, pBlock);
  sqlite3VdbeAddOp(v, OP_Halt, 0, 0);

  /* save compiled body code, reset vdbe */
  if( !pParse->explain ){
    no->nOp = v->nOp;
    v->nOp = 0;
    no->aOp = v->aOp;
    v->aOp = 0;
    v->nOpAlloc = 0;
  }
  DbClearProperty(db, 0, DB_Locked);
  DbClearProperty(db, 1, DB_Locked);

  /* if we are not initializing build the sqlite3_master entry */
  if( !db->init.busy ){
    static VdbeOpList insertObj[] = {
      { OP_NewRecno,   0, 0,  0           },
      { OP_String,     0, 0,  "procedure" },
      { OP_String,     0, 0,  0           },  /* 2: object name */
      { OP_String,     0, 0,  0           },  
      { OP_Integer,    0, 0,  0           },
      { OP_String,     0, 0,  0           },  /* 5: SQL */
      { OP_MakeRecord, 5, 0,  0           },
      { OP_PutIntKey,  0, 0,  0           },
    };
    int addr;

    /* Make an entry in the sqlite3_master table */
    if( v==0 ) goto objectfinish_cleanup;
    sqlite3BeginWriteOperation(pParse, 0, 0);
    sqlite3OpenMasterTable(v, 0);
    addr = sqlite3VdbeAddOpList(v, ArraySize(insertObj), insertObj);
    sqlite3VdbeChangeP3(v, addr+2, no->name, 0); 
    sqlite3VdbeChangeP3(v, addr+5, pAll->z, pAll->n);
    if( no->iDb==0 ){
      sqlite3ChangeCookie(db, v);
    }
    sqlite3VdbeAddOp(v, OP_Close, 0, 0);
    sqlite3EndWriteOperation(pParse);
  }

  if( !pParse->explain ){
    sqlite3HashInsert(&db->aDb[no->iDb].objectHash, 
                     no->name, strlen(no->name)+1, no);
    no = 0;
  }

objectfinish_cleanup:
  sqlite3DeleteObject(no);
  sqlite3DeleteObject(pParse->pNewObject);
  pParse->pNewObject = 0;
}

void sqlite3DropProc(Parse *pParse, Token *pName){
  sqlite3 *db,
  SP_Object *pObj;
  char *zName;
  Vdbe *v = sqlite3GetVdbe(pParse);
  sqlite3 *db = pParse->db;

  zName = sqlite3StrNDup(pName->z, pName->n);
  sqlite3Dequote(zName);
  pObj = sqlite3HashFind(&(db->aDb[0].objectHash), zName, pName->n+1);
  if( !pParse->explain && !pObj ){
    sqlite3ErrorMsg(pParse, "no such object: %T", pName);
    goto dropobject_cleanup;
  }

  /* Generate code to destroy the database record of the trigger.
  */
  if( v ){
    int base;
    static VdbeOpList dropObject[] = {
      { OP_Rewind,     0, ADDR(9),  0},
      { OP_String,     0, 0,        0}, /* 1 */
      { OP_Column,     0, 1,        0},
      { OP_Ne,         0, ADDR(8),  0},
      { OP_String,     0, 0,        "procedure"},
      { OP_Column,     0, 0,        0},
      { OP_Ne,         0, ADDR(8),  0},
      { OP_Delete,     0, 0,        0},
      { OP_Next,       0, ADDR(1),  0}, /* 8 */
    };

    sqlite3BeginWriteOperation(pParse, 0, 0);
    sqlite3OpenMasterTable(v, 0);
    base = sqlite3VdbeAddOpList(v,  ArraySize(dropObject), dropObject);
    sqlite3VdbeChangeP3(v, base+1, zName, 0);
    if( pObj && pObj->iDb==0 ){
      sqlite3ChangeCookie(db, v);
    }
    sqlite3VdbeAddOp(v, OP_Close, 0, 0);
    sqlite3EndWriteOperation(pParse);
  }

  /*
   * If this is not an "explain", then delete the trigger structure.
   */
  if( !pParse->explain ){
    sqlite3HashInsert(&(db->aDb[pObj->iDb].objectHash), zName, pName->n+1, 0);
    sqlite3DeleteObject(pObj);
  }

dropobject_cleanup:
  sqlite3_free(zName);
}

/*************************************************************************
** 
** Routines to assist in building / compiling EXEC commands
*/

void sqlite3ExecProc(sqlite3 *db, Parse *pParse, Token *pName, ExprList *pEList) {
  Vdbe *v = sqlite3GetVdbe(pParse);

  sqlite3VdbeOp3(v, OP_ColumnName, 0, 1, "Result", P3_STATIC);
  if( sqlite3CompileCall(pParse, pName, pEList) ) {
    return;
  }
  sqlite3VdbeAddOp(v, OP_Callback, 1, 0);
  sqlite3VdbeAddOp(v, OP_Halt, 0, 0);
  return;
}

void sqlite3ExecBlock(sqlite3 *db, Parse *pParse, SP_Block *pBlock) {
  Vdbe *v = sqlite3GetVdbe(pParse);

  sqlite3VdbeOp3(v, OP_ColumnName, 0, 1, "Result", P3_STATIC);
  sqlite3VdbeAddOp(v, OP_String, 0, 0);
  sqlite3VdbeAddOp(v, OP_MemStore, 0, 1);
  sqlite3CompileBlock(pParse, pBlock);
  sqlite3VdbeAddOp(v, OP_MemLoad, 0, 0);
  sqlite3VdbeAddOp(v, OP_Callback, 1, 0);
  sqlite3VdbeAddOp(v, OP_Halt, 0, 0);

  sqlite3BlockDelete(db, pBlock);
}
