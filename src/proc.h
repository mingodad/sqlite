typedef struct Variable Variable;
//typedef struct Stmt Stmt;
//typedef struct SQLStmt SQLStmt;
//typedef struct StmtList StmtList;
typedef struct Block Block;
//typedef struct Object Object;

/*
** Each node of a variable list in the parse tree is an instance
** of this structure.
**
*/
struct Variable {
  char *zName;     /* Name of this variable */
  Expr *pDflt;     /* Default value of this variable */
  char *zType;     /* Data type for this variable */
  int mVar;		   /* memory cell number of variable */
  u8 notNull;      /* True if there is a NOT NULL constraint */
  u8 isParam;	   /* True if this is a parameter */
};

/*
 * An instance of struct ProcStack stores information required during code
 */
struct Block {
  int nVar;        /* Number of local variables in this block */
  Variable *aVar;  /* Detail of each local variable */
  int mReturn;     /* Number of memory cell with return value */
  int nExit;       /* Label no. of halt statement */
  int params;	   /* True while parsing parameters */
  Object *pObj;	   /* Object this body belongs to, if any */
  StmtList *pStList; /* List of statements in this block */
  StmtList *pExList; /* List of exception handlers in this block */
  Block *pParent;  /* Enclosing block, or null if current is outermost */
};

struct Object {
  char *name;             /* The name of the object                      */
  u8 iDb;                 /* Database containing this object             */
  u8 what;                /* One of TK_PROCEDURE, TK_FUNCTION            */
  int nParam;			  /* Number of parameters                        */
  int nOp;                /* Number of instructions in the program       */
  VdbeOp *aOp;            /* Space to hold the vdbe program              */
};