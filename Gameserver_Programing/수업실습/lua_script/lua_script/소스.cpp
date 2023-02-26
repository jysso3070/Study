#include <iostream>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
using namespace std;

void lua_error(lua_State *L, const char *fmt, ...)
{
	va_list argp; 
	va_start(argp, fmt); 
	vfprintf(stderr, fmt, argp); 
	va_end(argp); 
	lua_close(L); 
	exit(EXIT_FAILURE);
	while (true);
}

int addnum_c(lua_State *L)
{
	int a = (int)lua_tonumber(L, -2);
	int b = (int)lua_tonumber(L, -1);
	lua_pop(L, 3);
	lua_pushnumber(L, a + b);
	return 1;
}

int main()
{
	int rows, cols;
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	lua_register(L, "addnum_c", addnum_c);
	luaL_loadfile(L, "dragon.lua"); 
	if (0 != lua_pcall(L, 0, 0, 0)) {
		lua_error(L, "error running function ‘dragon.lua’: %s\n", lua_tostring(L, -1));
	}

	

	lua_getglobal(L, "cols");
	int result = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);

	cout << "Result = " << result << "\n";


	//lua_getglobal(L, "plustwo");
	//lua_pushnumber(L, 2);
	//if (0 != lua_pcall(L, 1, 1, 0)) {  // (L, 파라티머, 리턴값)
	//	lua_error(L, "error running function ‘plustwo’: %s\n", lua_tostring(L, -1));
	//}
	//int result = (int)lua_tonumber(L, -1);
	//lua_pop(L, 1);

	//cout << "Result = " << result << "\n";


	//lua_getglobal(L, "rows"); // 변수를 꺼내서 인터프리터 맨위로 올려놓는다
	//rows = (int)lua_tonumber(L, -1); // -1이 맨위에있는걸 가르킨다 -2는 맨위에서 다음
	//lua_pop(L, 1);

	//lua_getglobal(L, "cols");
	//cols = (int)lua_tonumber(L, -1);
	//lua_pop(L, 1);

	//cout << "Cols = " << cols << ", Lows = " << rows << "\n";
	//lua_close(L);
	//system("pause");

	//--------------------
	/*char buf[256];
	while (NULL != fgets(buf, sizeof(buf), stdin)) {
		luaL_loadbuffer(L, buf, strlen(buf), "line");
		lua_pcall(L, 0, 0, 0);
	}
	lua_close(L);*/
	

}