//#include "../sqlite3.c"
#include "sqlite3.h"
#include "sqliteInt.h"
#include "pager.h"

#ifndef SQLITE_OMIT_DISKIO

#ifdef SQLITE_HAS_CODEC

/*
** Get the codec argument for this pager
*/

#if (SQLITE_VERSION_NUMBER >= 3006016)
void *sqlite3PagerGetCodec(Pager *pPager);
#endif

void* mySqlite3PagerGetCodec(
  Pager *pPager
){
#if (SQLITE_VERSION_NUMBER >= 3006016)
  return sqlite3PagerGetCodec(pPager);
#else
  return (pPager->xCodec) ? pPager->pCodecArg : NULL;
#endif
}

/*
** Set the codec argument for this pager
*/

void mySqlite3PagerSetCodec(
  Pager *pPager,
  void *(*xCodec)(void*,void*,Pgno,int),
  void (*xCodecSizeChng)(void*,int,int),
  void (*xCodecFree)(void*),
  void *pCodec
){
  sqlite3PagerSetCodec(pPager, xCodec, xCodecSizeChng, xCodecFree, pCodec);
}

#ifndef SQLITE_AMALGAMATION
#include "rijndael.c"
#include "codec.c"
#include "codecext.c"
#endif

#endif

#endif

