#include <functional>
#include <iostream>
#include <sstream>
#include <vector>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

std::string pad(std::string s, size_t size = 16)
{
	while (s.length() < size)
		s.append(" ");

	return s;
}

int main(int argc, char *argv[]) {
	auto lua = [](std::stringstream &compOut) {
		auto compiled = std::string(LUA_RELEASE).substr(4);
		compOut << compiled;

		lua_State* L = luaL_newstate();
		if (L == nullptr) {
			std::cerr << "Failed to create Lua state!" << std::endl;
			return "Lua";
		}

		// Load the Lua standard libraries
		luaL_openlibs(L);

		// Check if we have JIT and get it as a bool in C++
		if (luaL_loadstring(L, "return jit ~= nil") || lua_pcall(L, 0, 1, 0) != 0) {
			std::cerr << "Failed to check for LuaJIT!" << std::endl;
			lua_close(L);
			return "Lua";
		}

		bool jit = lua_toboolean(L, -1);

		compOut << " + JIT: " << (jit ? "Yes" : "No");
		return jit ? "LuaJIT" : "Lua";
	};

	std::vector<std::function<std::string(std::stringstream&)>> funcs = {
		lua
	};

	for (auto& f : funcs) {
		std::stringstream c;
		std::string name = f(c);

		std::cout << "-- " << pad(name) << "   compiled: " << pad(c.str(), 7) << std::endl;
	}

	std::cout << "Press RETURN to exit..." << std::endl;
	getchar(); return 0;
}
