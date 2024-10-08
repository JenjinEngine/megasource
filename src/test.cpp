#include <functional>
#include <iostream>
#include <sol/state.hpp>
#include <sstream>
#include <vector>
#include <string>
#include <cstdlib>

#include <glad/glad.h> // GLAD
#include <GLFW/glfw3.h> // GLFW
#include <glm/glm.hpp> // GLM
#include <imgui/imgui.h> // ImGui
#include <spdlog/spdlog.h> // Spdlog
#include <sol/sol.hpp> // Sol3

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h> // stb_image

// Lua
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

std::string pad(std::string s, size_t size = 16) {
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

	auto sol = [](std::stringstream &compOut) {
		sol::state lua;
		lua.safe_script("a = 3 + 4");
		if (lua["a"] != 7) {
			std::cerr << "Failed to execute Lua code!" << std::endl;
			return "Sol3";
		}

		compOut << fmt::format("{}.{}.{}", SOL_VERSION_MAJOR, SOL_VERSION_MINOR, SOL_VERSION_PATCH);

		return "Sol3";
	};

	auto glm = [](std::stringstream &compOut) {
		auto compiled = GLM_VERSION;
		compOut << compiled;
		return "GLM";
	};

	auto spdlog = [](std::stringstream &compOut) {
		auto compiled = fmt::format("{}.{}.{}", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);
		compOut << compiled;
		return "Spdlog";
	};

	auto stb = [](std::stringstream &compOut) {
		auto compiled = STBI_VERSION;
		compOut << compiled;
		return "stb_image";
	};

	GLFWwindow* window;
	auto glfw = [&](std::stringstream &compOut) {
		glfwSetErrorCallback([](int error, const char* description) {
			std::cerr << "GLFW Error " << error << ": " << description;
			spdlog::error("GLFW Error {}: {}", error, description);
		});

		if (!glfwInit()) {
			std::cerr << "Failed to initialize GLFW!" << std::endl;
			return "GLFW";
		}

		window = glfwCreateWindow(800, 600, "Hello World", NULL, NULL);
		glfwMakeContextCurrent(window);

		compOut << glfwGetVersionString();
		return "GLFW";
	};

	auto glad = [](std::stringstream &compOut) {
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			std::cerr << "Failed to initialize GLAD!" << std::endl;
			return "GLAD";
		}

		glfwTerminate();

		compOut << GLVersion.major << "." << GLVersion.minor;
		return "GLAD (OpenGL)";
	};

	std::vector<std::function<std::string(std::stringstream&)>> funcs = {
		lua,
		sol,
		glm,
		spdlog,
		stb,
		glfw,
		glad
	};

	for (auto& f : funcs) {
		std::stringstream c;
		std::string name = f(c);

		std::cout << "-- " << pad(name) << "   compiled: " << pad(c.str(), 7) << std::endl;
	}

	std::cout << "\nPress RETURN to exit...";
	getchar(); return 0;
}
