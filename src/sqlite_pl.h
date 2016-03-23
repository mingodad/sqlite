#ifdef SQLITE_WITH_PROCEDURAL_LANGUAGE
#ifndef _SQLITE_PL_H_
#define _SQLITE_PL_H_

// TK_ASSIGN

typedef struct PL_Variable PL_Variable;
typedef struct PL_Stmt PL_Stmt;
typedef struct PL_SQLStmt PL_SQLStmt;
typedef struct PL_StmtList PL_StmtList;
typedef struct PL_Block PL_Block;
typedef struct PL_Object PL_Object;

/*
** Each node of a PL_Variable list in the parse tree is an instance
** of this structure.
**
*/
struct PL_Variable {
  char *zName;     /* Name of this PL_Variable */
  Expr *pDflt;     /* Default value of this PL_Variable */
  char *zType;     /* Data type for this PL_Variable */
  int mVar;		   /* memory cell number of PL_Variable */
  u8 notNull;      /* True if there is a NOT NULL constraint */
  u8 isParam;	   /* True if this is a parameter */
};

/*
** Each node of a statement list in the parse tree is an instance
** of this structure.
**
*/
struct PL_Stmt {
  int op;            /* Type of statement, e.g. TK_IF, TK_WHILE, TK_SQL, etc. */
  Expr *pExpr1;      /* Statement expression */
  PL_StmtList *pStmt1;	 /* First sub-statements */
  PL_Stmt *pStmt2;	     /* Second sub-statement (used for IF & CASE)*/
  PL_Block *pPL_Block;	   /* PL_Block, if this statement is a sub-block */
  PL_SQLStmt *pSql;	   /* SQL statement details, if this is one */
};

struct PL_SQLStmt {
  int op;              /* One of TK_DELETE, TK_UPDATE, TK_INSERT, TK_SELECT */
  int orconf;          /* OE_Rollback etc. */
  Select *pSelect;     /* Valid for SELECT and sometimes 
						  INSERT steps (when pExprList == 0) */
  Token target;        /* Valid for DELETE, UPDATE, INSERT steps */
  Expr *pWhere;        /* Valid for DELETE, UPDATE steps */
  ExprList *pExprList; /* Valid for UPDATE statements and sometimes 
						  INSERT steps (when pSelect == 0)         */
  IdList *pIdList;     /* Valid for INSERT statements only */
};

/*
** A list of statements.
*/
struct PL_StmtList {
  int nStmt;             /* Number of expressions on the list */
  int nAlloc;            /* Number of entries allocated below */
  struct PL_StmtList_item {
    PL_Stmt *pStmt;         /* The list of statements */
  } *a;                  /* One entry for each expression */
};

/*
 * An instance of struct ProcStack stores information required during code
 */
struct PL_Block {
  int nVar;        /* Number of local PL_Variables in this block */
  PL_Variable *aVar;  /* Detail of each local PL_Variable */
  int mReturn;     /* Number of memory cell with return value */
  int nExit;       /* Label no. of halt statement */
  int params;	   /* True while parsing parameters */
  PL_Object *pObj;	   /* PL_Object this body belongs to, if any */
  PL_StmtList *pStList; /* List of statements in this block */
  PL_StmtList *pExList; /* List of exception handlers in this block */
  PL_Block *pParent;  /* Enclosing block, or null if current is outermost */
};

struct PL_Object {
  char *name;             /* The name of the object                      */
  u8 iDb;                 /* Database containing this object             */
  u8 what;                /* One of TK_PROCEDURE, TK_FUNCTION            */
  int nParam;			  /* Number of parameters                        */
  int nOp;                /* Number of instructions in the program       */
  VdbeOp *aOp;            /* Space to hold the vdbe program              */
};


void sqlite_pl_Proc(Parse *pParse, ExprList *pList);
void sqlite_pl_StartPL_Block(Parse*, int);
PL_Block *sqlite_pl_EndPL_Block(Parse*, PL_StmtList*, PL_StmtList*);
void sqlite_pl_PL_StmtListDelete(PL_StmtList *pList);
void sqlite_pl_PL_BlockDelete(PL_Block *pPL_Block);
void sqlite_pl_AddProcVar(Parse *pParse, Token *pName);
void sqlite_pl_AddProcVarType(Parse *pParse, Token *pFirst, Token *pLast);
void sqlite_pl_AddProcVarExpr(Parse *pParse, Expr *pExpr1, int notnull);
int sqlite_pl_LookupVar(Parse*, PL_Block*, Expr*);
PL_Stmt* sqlite_pl_Stmt(Parse*, int, Expr*, PL_StmtList*, PL_Stmt*, PL_Block*);
void sqlite_pl_StmtDelete(PL_Stmt *p);
PL_StmtList *sqlite_pl_PL_StmtListAppend(PL_StmtList *pList, PL_Stmt *pStmt);
void sqlite_pl_PL_StmtListDelete(PL_StmtList *pList);
PL_Stmt *sqlite_pl_PL_SQLStmt(int, Token *, IdList *, ExprList *, Select *, Expr *, int);

void sqlite_pl_FinishProc(Parse*, PL_Block*, Token*);
void sqlite_pl_BeginProc(Parse*, int, Token*);
void sqlite_pl_DropProc(Parse*, Token*);
void sqlite_pl_DeletePL_Object(PL_Object*);
void sqlite_pl_DropProcPtr(Parse*, PL_Object*);
void sqlite_pl_ExecProc(Parse*, Token*, ExprList*);
void sqlite_pl_ExecPL_Block(Parse*, PL_Block*);

int sqlite_pl_DoDefDDL(sqlite3*, char**);

#endif //_SQLITE_PL_H_
#endif //SQLITE_WITH_PROCEDURAL_LANGUAGE
