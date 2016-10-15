#include "sqlite_sublatin.h"
#include "sublatin.h"
#include "sqliteInt.h"

static inline int sl_func_cmp_CollatingFunc(
  int (*fcmp)(const char*, const char*, int),
  int nKey1, const void *pKey1,
  int nKey2, const void *pKey2
){
  int nCmpKey1 = nstrLenSubSetLatinUtf8(pKey1, nKey1);
  int nCmpKey2 = nstrLenSubSetLatinUtf8(pKey2, nKey2);
  int ncmp = (nCmpKey1<nCmpKey2)?nCmpKey1:nCmpKey2;
  int r = fcmp(
      (const char *)pKey1, (const char *)pKey2, ncmp);
  return (r == 0) ? (nCmpKey1-nCmpKey2) : r;
}

static int sl_nicmp_deaccent_CollatingFunc(
  void *NotUsed,
  int nKey1, const void *pKey1,
  int nKey2, const void *pKey2
){
  UNUSED_PARAMETER(NotUsed);
  return sl_func_cmp_CollatingFunc(strNICmpSubSetLatinUtf8NoAccents, nKey1, pKey1, nKey2, pKey2);
}

static int sl_nicmp_CollatingFunc(
  void *NotUsed,
  int nKey1, const void *pKey1,
  int nKey2, const void *pKey2
){
  UNUSED_PARAMETER(NotUsed);
  return sl_func_cmp_CollatingFunc(strNICmpSubSetLatinUtf8, nKey1, pKey1, nKey2, pKey2);
}

/*
** Allocate nByte bytes of space using sqlite3_malloc(). If the
** allocation fails, call sqlite3_result_error_nomem() to notify
** the database handle that malloc() has failed and return NULL.
** If nByte is larger than the maximum string or blob length, then
** raise an SQLITE_TOOBIG exception and return NULL.
*/
static void *myContextMalloc(sqlite3_context *context, i64 nByte){
  char *z;
  sqlite3 *db = sqlite3_context_db_handle(context);
  assert( nByte>0 );
  testcase( nByte==db->aLimit[SQLITE_LIMIT_LENGTH] );
  testcase( nByte==db->aLimit[SQLITE_LIMIT_LENGTH]+1 );
  if( nByte>db->aLimit[SQLITE_LIMIT_LENGTH] ){
    sqlite3_result_error_toobig(context);
    z = 0;
  }else{
    z = sqlite3Malloc((int)nByte);
    if( !z ){
      sqlite3_result_error_nomem(context);
    }
  }
  return z;
}

/*
** Implementation of the upper() and lower() SQL functions.
*/
static void sl_upperFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  char *z1;
  const char *z2;
  int n;
  UNUSED_PARAMETER(argc);
  z2 = (char*)sqlite3_value_text(argv[0]);
  n = sqlite3_value_bytes(argv[0]);
  /* Verify that the call to _bytes() does not invalidate the _text() pointer */
  assert( z2==(char*)sqlite3_value_text(argv[0]) );
  if( z2 ){
    z1 = myContextMalloc(context, ((i64)n)+1);
    if( z1 ){
      memcpy(z1, z2, n+1);
	  toUpperSubSetLatinUtf8(z1);
      sqlite3_result_text(context, z1, -1, sqlite3_free);
    }
  }
}

static void sl_lowerFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  u8 *z1;
  const char *z2;
  int n;
  UNUSED_PARAMETER(argc);
  z2 = (char*)sqlite3_value_text(argv[0]);
  n = sqlite3_value_bytes(argv[0]);
  /* Verify that the call to _bytes() does not invalidate the _text() pointer */
  assert( z2==(char*)sqlite3_value_text(argv[0]) );
  if( z2 ){
    z1 = myContextMalloc(context, ((i64)n)+1);
    if( z1 ){
      memcpy(z1, z2, n+1);
	  toLowerSubSetLatinUtf8((char*)z1);
      sqlite3_result_text(context, (char *)z1, -1, sqlite3_free);
    }
  }
}

static void sl_lower_deaccentFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  u8 *z1;
  const char *z2;
  int n;
  UNUSED_PARAMETER(argc);
  z2 = (char*)sqlite3_value_text(argv[0]);
  n = sqlite3_value_bytes(argv[0]);
  /* Verify that the call to _bytes() does not invalidate the _text() pointer */
  assert( z2==(char*)sqlite3_value_text(argv[0]) );
  if( z2 ){
    z1 = myContextMalloc(context, ((i64)n)+1);
    if( z1 ){
      memcpy(z1, z2, n+1);
	  toLowerDeaccentSubSetLatinUtf8((char*)z1);
      sqlite3_result_text(context, (char *)z1, -1, sqlite3_free);
    }
  }
}

static void sl_deaccentFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  u8 *z1;
  const char *z2;
  int n;
  UNUSED_PARAMETER(argc);
  z2 = (char*)sqlite3_value_text(argv[0]);
  n = sqlite3_value_bytes(argv[0]);
  /* Verify that the call to _bytes() does not invalidate the _text() pointer */
  assert( z2==(char*)sqlite3_value_text(argv[0]) );
  if( z2 ){
    z1 = myContextMalloc(context, ((i64)n)+1);
    if( z1 ){
      memcpy(z1, z2, n+1);
	  deAccentSubSetLatinUtf8((char*)z1);
      sqlite3_result_text(context, (char *)z1, -1, sqlite3_free);
    }
  }
}

static void sl_likeFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  const unsigned char *zA, *zB;
  int escape = 0;
  int nPat;
  sqlite3 *db = sqlite3_context_db_handle(context);

  zB = sqlite3_value_text(argv[0]);
  zA = sqlite3_value_text(argv[1]);

  /* Limit the length of the LIKE or GLOB pattern to avoid problems
  ** of deep recursion and N*N behavior in patternCompare().
  */
  nPat = sqlite3_value_bytes(argv[0]);
  testcase( nPat==db->aLimit[SQLITE_LIMIT_LIKE_PATTERN_LENGTH] );
  testcase( nPat==db->aLimit[SQLITE_LIMIT_LIKE_PATTERN_LENGTH]+1 );
  if( nPat > db->aLimit[SQLITE_LIMIT_LIKE_PATTERN_LENGTH] ){
    sqlite3_result_error(context, "LIKE or GLOB pattern too complex", -1);
    return;
  }
  assert( zB==sqlite3_value_text(argv[0]) );  /* Encoding did not change */

  if( argc==3 ){
    /* The escape character string must consist of a single UTF-8 character.
    ** Otherwise, return an error.
    */
    const unsigned char *zEsc = sqlite3_value_text(argv[2]);
    if( zEsc==0 ) return;
    if( sqlite3Utf8CharLen((char*)zEsc, -1)!=1 ){
      sqlite3_result_error(context,
          "ESCAPE expression must be a single character", -1);
      return;
    }
    escape = sqlite3Utf8Read(&zEsc);
  }
  if( zA && zB ){
    //struct compareInfo *pInfo = sqlite3_user_data(context);
#ifdef SQLITE_TEST
    sqlite3_like_count++;
#endif

    sqlite3_result_int(context, subLatinLikeCompare((const char*)zB, (const char*)zA, escape));
  }
}

static void sl_like_deaccentFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  const unsigned char *zA, *zB;
  int escape = 0;
  int nPat;
  sqlite3 *db = sqlite3_context_db_handle(context);

  zB = sqlite3_value_text(argv[0]);
  zA = sqlite3_value_text(argv[1]);

  /* Limit the length of the LIKE or GLOB pattern to avoid problems
  ** of deep recursion and N*N behavior in patternCompare().
  */
  nPat = sqlite3_value_bytes(argv[0]);
  testcase( nPat==db->aLimit[SQLITE_LIMIT_LIKE_PATTERN_LENGTH] );
  testcase( nPat==db->aLimit[SQLITE_LIMIT_LIKE_PATTERN_LENGTH]+1 );
  if( nPat > db->aLimit[SQLITE_LIMIT_LIKE_PATTERN_LENGTH] ){
    sqlite3_result_error(context, "LIKE or GLOB pattern too complex", -1);
    return;
  }
  assert( zB==sqlite3_value_text(argv[0]) );  /* Encoding did not change */

  if( argc==3 ){
    /* The escape character string must consist of a single UTF-8 character.
    ** Otherwise, return an error.
    */
    const unsigned char *zEsc = sqlite3_value_text(argv[2]);
    if( zEsc==0 ) return;
    if( sqlite3Utf8CharLen((char*)zEsc, -1)!=1 ){
      sqlite3_result_error(context,
          "ESCAPE expression must be a single character", -1);
      return;
    }
    escape = sqlite3Utf8Read(&zEsc);
  }
  if( zA && zB ){
    //struct compareInfo *pInfo = sqlite3_user_data(context);
#ifdef SQLITE_TEST
    sqlite3_like_count++;
#endif

    sqlite3_result_int(context, subLatinLikeCompareNoAccents((const char*)zB, (const char*)zA, escape));
  }
}

/*
** Set the LIKEOPT flag on the 2-argument function with the given name.
*/
static void setLikeOptFlagSubLatin(sqlite3 *db, int narg){
  FuncDef *pDef;
  pDef = sqlite3FindFunction(db, "like", narg, SQLITE_UTF8, 0);
  if( ALWAYS(pDef) ){
    pDef->funcFlags |= SQLITE_FUNC_LIKE;
  }
}

/*After the LIKE optmization implementation the user defined like was broken
and now we copy the missing bits here to make it work again */
/*
** A structure defining how to do GLOB-style comparisons.
*/
struct compareInfoSubLatin {
  u8 matchAll;          /* "*" or "%" */
  u8 matchOne;          /* "?" or "_" */
  u8 matchSet;          /* "[" or 0 */
  u8 noCase;            /* true to ignore case differences */
};

static const struct compareInfoSubLatin likeInfoNormSubLatin = { '%', '_',   0, 1 };

int sqlite3RegisterSubLatinFunctions(sqlite3 *db, int flags){
  int rc;
  rc = sqlite3_create_collation(db, "NOCASE_SLNA", SQLITE_UTF8, NULL,
      sl_nicmp_deaccent_CollatingFunc);
  if(rc != SQLITE_OK) return rc;

  rc = sqlite3_create_collation(db, "NOCASE_SL", SQLITE_UTF8, NULL,
      sl_nicmp_CollatingFunc);
  if(rc != SQLITE_OK) return rc;

  rc = sqlite3_create_function(db, "upper_sl", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC,
      NULL, sl_upperFunc, NULL, NULL);
  if(rc != SQLITE_OK) return rc;

  rc = sqlite3_create_function(db, "lower_sl", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC,
      NULL, sl_lowerFunc, NULL, NULL);
  if(rc != SQLITE_OK) return rc;

  rc = sqlite3_create_function(db, "lower_slna", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC,
      NULL, sl_lower_deaccentFunc, NULL, NULL);
  if(rc != SQLITE_OK) return rc;

  rc = sqlite3_create_function(db, "deaccent_sl", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC,
      NULL, sl_deaccentFunc, NULL, NULL);
  if(rc != SQLITE_OK) return rc;

  rc = sqlite3_create_function(db, "like_sl", 2, SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_FUNC_LIKE,
      (void*)&likeInfoNormSubLatin, sl_likeFunc, NULL, NULL);
  if(rc != SQLITE_OK) return rc;

  rc = sqlite3_create_function(db, "like_sl", 3, SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_FUNC_LIKE,
      (void*)&likeInfoNormSubLatin, sl_likeFunc, NULL, NULL);
  if(rc != SQLITE_OK) return rc;

  rc = sqlite3_create_function(db, "like_slna", 2, SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_FUNC_LIKE,
      (void*)&likeInfoNormSubLatin, sl_like_deaccentFunc, NULL, NULL);
  if(rc != SQLITE_OK) return rc;

  rc = sqlite3_create_function(db, "like_slna", 3, SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_FUNC_LIKE,
      (void*)&likeInfoNormSubLatin, sl_like_deaccentFunc, NULL, NULL);
  if(rc != SQLITE_OK) return rc;

  if((flags & SQLITE_OPEN_SUBLATIN_LIKE) || (flags & SQLITE_OPEN_SUBLATIN_NA_LIKE)){
	rc = sqlite3_create_collation(db, "NOCASE", SQLITE_UTF8, NULL,
	  sl_nicmp_CollatingFunc);
	if(rc != SQLITE_OK) return rc;

	rc = sqlite3_create_function(db, "upper", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC,
	  NULL, sl_upperFunc, NULL, NULL);
	if(rc != SQLITE_OK) return rc;

	rc = sqlite3_create_function(db, "lower", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC,
	  NULL, sl_lowerFunc, NULL, NULL);
	if(rc != SQLITE_OK) return rc;
  }

  if(flags & SQLITE_OPEN_SUBLATIN_LIKE){
    rc = sqlite3_create_function(db, "like", 2, SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_FUNC_LIKE,
	  (void*)&likeInfoNormSubLatin, sl_likeFunc, NULL, NULL);
	if(rc != SQLITE_OK) return rc;
	setLikeOptFlagSubLatin(db, 2);

	rc = sqlite3_create_function(db, "like", 3, SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_FUNC_LIKE,
	  (void*)&likeInfoNormSubLatin, sl_likeFunc, NULL, NULL);
	if(rc != SQLITE_OK) return rc;
	setLikeOptFlagSubLatin(db, 3);
  }

  if(flags & SQLITE_OPEN_SUBLATIN_NA_LIKE){
	rc = sqlite3_create_function(db, "like", 2, SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_FUNC_LIKE,
	  (void*)&likeInfoNormSubLatin, sl_like_deaccentFunc, NULL, NULL);
	if(rc != SQLITE_OK) return rc;
	setLikeOptFlagSubLatin(db, 2);

	rc = sqlite3_create_function(db, "like", 3, SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_FUNC_LIKE,
	  (void*)&likeInfoNormSubLatin, sl_like_deaccentFunc, NULL, NULL);
	if(rc != SQLITE_OK) return rc;
	setLikeOptFlagSubLatin(db, 3);
  }

  return rc;

}

