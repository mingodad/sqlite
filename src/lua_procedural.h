#ifdef LUA_SQLITE_PROCEDURAL
//lua includes
#include "lua.h"
#include "lauxlib.h"
extern int luaopen_lsqlite3(lua_State *L);
extern int lsqlite_register_db(lua_State *L, sqlite3 *db, const char *name);

/*
Install the main interpreter function
*/
void sqlite3_install_lua_script(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  assert( argc==1 );
  lua_State *L = (lua_State*) sqlite3_db_user_data(context);
  if(L && sqlite3_value_type(argv[0]) == SQLITE_TEXT){
	int res = luaL_dostring(L, sqlite3_value_text(argv[0]));
	sqlite3_result_int(context,res);
  } else sqlite3_result_null(context);
}

int sqlite3_install_lua_procedural(sqlite3 *db){
	lua_State *L = lua_open();
	if(L){
		luaL_openlibs(L); /* Load Lua libraries */
		sqlite3_set_db_user_data(db,(void*)L);
		lua_state_set_user_data(L,(void*)db);
		luaopen_lsqlite3(L);
		lsqlite_register_db(L, db, "db");
		sqlite3_create_function(db, "install_lua_script", 1, SQLITE_UTF8, 0,
			sqlite3_install_lua_script, 0, 0);
		return 0;
	} else {
	  //fprintf(stderr,"Unable to register lua scripting engine !\n");
	  return -1;
	}
}
#endif