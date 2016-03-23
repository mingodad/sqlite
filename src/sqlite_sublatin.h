#ifndef _SQLITE_SUBLATIN_H_
#define  _SQLITE_SUBLATIN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sqlite3.h"

int sqlite3RegisterSubLatinFunctions(sqlite3 *db, int flags);

#ifdef __cplusplus
}
#endif

#endif //_SQLITE_SUBLATIN_H_
