#ifndef _CODECEXT_H_
#define _CODECEXT_H_
#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */
SQLITE_API int sqlite3_key_v2(sqlite3 *db, const char *zDbName, const void *zKey, int nKey);
SQLITE_API int sqlite3_rekey_v2(sqlite3 *db, const char *zDbName, const void *zKey, int nKey);
SQLITE_API int sqlite3_CodecAttach(sqlite3* db, int nDb, const void* zKey, int nKey);
SQLITE_API void sqlite3_CodecGetKey(sqlite3* db, int nDb, void** zKey, int* nKey);
#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */
#endif //_CODECEXT_H_
