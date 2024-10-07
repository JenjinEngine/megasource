#include <functional>
#include <iostream>
#include <sstream>
#include <vector>

#include <AL/al.h>
#include <AL/alext.h>

extern "C" {
#include "lua.h"
}

std::string pad(std::string s, size_t size = 16)
{
	while (s.length() < size)
		s.append(" ");

	return s;
}

int main(int argc, char **argv) {
	auto lua = [](std::stringstream &compOut) {
		std::string compiled = LUA_RELEASE;

		compiled = compiled.substr(4);

		compOut << compiled;
		return "Lua";
	};

	auto OpenAL = [](std::stringstream &compOut) {
		alIsEnabled(AL_SOURCE_DISTANCE_MODEL);

		compOut << "N/A";
		return "OpenAL";
	};

	std::vector<std::function<std::string(std::stringstream&)>> funcs;
	funcs.push_back(lua);
	funcs.push_back(OpenAL);

	for (auto& f : funcs) {
		std::stringstream c;
		std::string name = f(c);

		std::cout << "-- " << pad(name) << "   compiled: " << pad(c.str(), 7) << std::endl;
	}

	std::cout << "Press RETURN to exit..." << std::endl;
	getchar(); return 0;
}
