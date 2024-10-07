if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	# Protop: run cmake like this: cmake -G "<generator>" -H. -Bbuild
	message(FATAL_ERROR "Prevented in-tree build.")
endif()

cmake_minimum_required(VERSION 3.5)

include(ExternalProject)

project(megasource)

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBS OFF)

set (CMAKE_CXX_STANDARD 11)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")

set(MEGA TRUE)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	if(MSVC)
		# Normally we should rely on CMAKE_SYSTEM_PROCESSOR
		# but as of CMake 3.19.1, it sets to AMD64 even when targetting
		# Windows ARM64. This needs to be reevaluated in the future.
		set(_MEGATEMP_CHECKVAR CMAKE_SYSTEM_PROCESSOR)
		if("${CMAKE_GENERATOR}" MATCHES "^Visual Studio")
			set(_MEGATEMP_CHECKVAR CMAKE_GENERATOR_PLATFORM)
		# Note that MSVC_CXX_ARCHITECTURE_ID is an undocumented
		# variable, but unfortunately we need to depends on it
		# as reason stated above.
		# https://gitlab.kitware.com/cmake/cmake/-/issues/15170
		elseif(DEFINED MSVC_CXX_ARCHITECTURE_ID)
			set(_MEGATEMP_CHECKVAR MSVC_CXX_ARCHITECTURE_ID)
		endif()
		# FIXME: Code below also duplicates MEGA_X64 architecture assignment.
		if(${_MEGATEMP_CHECKVAR} STREQUAL "ARM64")
			set(MEGA_ARM64 TRUE)
			set(MEGA_ARCH ARM64)
		else()
			set(MEGA_X64 TRUE)
			set(MEGA_ARCH x64)
		endif()
		unset(_MEGATEMP_CHECKVAR)
	else()
		set(MEGA_X64 TRUE)
		set(MEGA_ARCH x64)
	endif()
else()
	set(MEGA_X86 TRUE)
	set(MEGA_ARCH x86)
endif()

if(MSVC)
	set(CMAKE_SUPPRESS_REGENERATION TRUE)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MP")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MP")
	message(STATUS "CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
	message(STATUS "CMAKE_C_FLAGS_RELEASE: ${CMAKE_C_FLAGS_RELEASE}")
endif()

function(add_move_dll ARG_POST_TARGET ARG_TARGET)
	add_custom_command(TARGET ${ARG_POST_TARGET} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		$<TARGET_FILE:${ARG_TARGET}>
		${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>/$<TARGET_FILE_NAME:${ARG_TARGET}>)
endfunction()

function(add_move_file ARG_POST_TARGET ARG_FILE)
	get_filename_component(TEMP_FILENAME ${ARG_FILE} NAME)
	add_custom_command(TARGET ${ARG_POST_TARGET} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		${ARG_FILE}
		${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>/${TEMP_FILENAME})
endfunction()

function(disable_warnings ARG_TARGET)
	get_target_property(OLD_FLAGS ${ARG_TARGET} COMPILE_FLAGS)
	if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		set(NEW_FLAGS -w)
	else ()
		set(NEW_FLAGS -W0)
	endif ()
	if(OLD_FLAGS)
		set(NEW_FLAGS "${OLD_FLAGS} ${NEW_FLAGS}")
	endif()
	set_target_properties(${ARG_TARGET} PROPERTIES COMPILE_FLAGS ${NEW_FLAGS})
endfunction()

# Find vcvarsall.bat and look for dynamic runtime DLLs.
if(MSVC)
	option(LOVE_INSTALL_UCRT "Provide the Windows 10 Universal CRT DLLs with LOVE" OFF)

	if(NOT(MSVC_TOOLSET_VERSION STREQUAL ""))
		set(VSVERSION "${MSVC_TOOLSET_VERSION}")
	elseif((MSVC_VERSION GREATER 1909) AND (MSVC_VERSION LESS 1920))
		set(VSVERSION "141")
	elseif(MSVC_VERSION EQUAL 1900)
		set(VSVERSION "140")
	elseif(MSVC_VERSION EQUAL 1800)
		set(VSVERSION "120")
	elseif(MSVC_VERSION LESS 1800)
		message(WARNING "Visual Studio 2013 (VS12) or newer is required!")
	else()
		message(WARNING "Cannot determine Visual Studio version!")
	endif()

	set(PROGRAMFILES_X86_STR "ProgramFiles(x86)")
	set(PROGRAMFILES_X86 "$ENV{${PROGRAMFILES_X86_STR}}")

	set(VSCOMNTOOLSSTRING "VS${VSVERSION}COMNTOOLS")
	set(VSCOMNTOOLS "$ENV{${VSCOMNTOOLSSTRING}}")

	if(VSCOMNTOOLS STREQUAL "")
		set(VSWHERE_PATH "${PROGRAMFILES_X86}\\Microsoft Visual Studio\\Installer\\vswhere.exe")

		execute_process(COMMAND ${VSWHERE_PATH} "-property" "installationPath" OUTPUT_VARIABLE VS_INSTALL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)

		if(NOT(VS_INSTALL_DIR STREQUAL ""))
			set(VSCOMNTOOLS "${VS_INSTALL_DIR}\\Common7\\Tools")
		endif()
	endif()

	if(VSCOMNTOOLS STREQUAL "")
		message(WARNING "Environment variable VSCOMNTOOLS not defined and vswhere.exe could not be found. Is Visual Studio properly installed?")
	else()
		# Find VsDevCmd.bat
		get_filename_component(VS_VCVARSALL_BAT ${VSCOMNTOOLS}/VsDevCmd.bat ABSOLUTE)
		if(NOT(EXISTS ${VS_VCVARSALL_BAT}))
			# Find vcvarsall.bat.
			get_filename_component(VS_VCVARSALL_BAT ${VSCOMNTOOLS}/../../VC/Auxiliary/Build/vcvarsall.bat ABSOLUTE)

			if(NOT(EXISTS ${VS_VCVARSALL_BAT}))
				# older VS versions have it here
				get_filename_component(VS_VCVARSALL_BAT ${VSCOMNTOOLS}/../../VC/vcvarsall.bat ABSOLUTE)
			endif()
		endif()

		if(EXISTS ${VS_VCVARSALL_BAT})
			set(MEGA_MSVC_VCVARSALL_BAT ${VS_VCVARSALL_BAT})

			if(VS_VCVARSALL_BAT MATCHES "VsDevCmd")
				message(STATUS "Found VsDevCmd.bat")
				if(MEGA_X64)
					set(MEGA_MSVC_VCVARSALL_BAT_ARG "-arch=amd64")
				elseif(MEGA_ARM64)
					set(MEGA_MSVC_VCVARSALL_BAT_ARG "-arch=arm64 -host_arch=amd64")
				else()
					set(MEGA_MSVC_VCVARSALL_BAT_ARG "-arch=x86")
				endif()
				math(EXPR VS_TOOLCHAIN_VERSION_MAJOR "${VSVERSION} / 10")
				math(EXPR VS_TOOLCHAIN_VERSION_MINOR "${VSVERSION} % 10")
				set(MEGA_MSVC_VCVARSALL_BAT_ARG "${MEGA_MSVC_VCVARSALL_BAT_ARG} -vcvars_ver=${VS_TOOLCHAIN_VERSION_MAJOR}.${VS_TOOLCHAIN_VERSION_MINOR}")
			else()
				message(STATUS "Found vcvarsall.bat")
				if(MEGA_X86)
					set(MEGA_MSVC_VCVARSALL_BAT_ARG "x86")
				elseif(MEGA_ARM64)
					set(MEGA_MSVC_VCVARSALL_BAT_ARG "x64_arm64")
				else()
					set(MEGA_MSVC_VCVARSALL_BAT_ARG "x86_amd64")
				endif()
			endif()
		else()
			message(WARNING "Could not find VsDevCmd.bat nor vcvarsall.bat")
		endif()
	endif()

	if(LOVE_INSTALL_UCRT)
		# We currently ship on platforms which might not have the Universal C Runtime installed.
		set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
	endif()

	set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)

	# TODO: The code for InstallRequiredSystemLibraries needs the UCRTVersion and WindowsSDKVersion
	# environment variables set by vcvarsall in order to detect the correct version of the UCRT, in
	# newer Windows SDK versions. We don't gurantee that vcvarsall has been loaded before running...
	include(InstallRequiredSystemLibraries)

	foreach(RUNTIME_LIB ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS})
		if(RUNTIME_LIB MATCHES "concrt")
			set(MSVC_RUNTIME_CONCRT_PATH ${RUNTIME_LIB})
		elseif(RUNTIME_LIB MATCHES "msvcp")
			set(MSVC_RUNTIME_MSVCP_PATH ${RUNTIME_LIB})
		endif()
	endforeach()

	# InstallRequiredSystemLibraries adds the concrt dll to its list, but we don't need it.
	if (EXISTS ${MSVC_RUNTIME_CONCRT_PATH})
		list(REMOVE_ITEM CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS "${MSVC_RUNTIME_CONCRT_PATH}")
	endif()

	# CMake 3.20 InstallRequiredSystemLibraries recognize ARM64, but their older version aren't.
	# Put this safeguard in place, otherwise it will copy AMD64 DLLS which is NOT what user wants!
	if(MEGA_ARM64 AND ${CMAKE_VERSION} VERSION_LESS "3.20.0")
		message(WARNING "InstallRequiredSystemLibraries of CMake ${CMAKE_VERSION} doesn't support Windows ARM64. Please update your CMake!")
		set(MSVC_RUNTIME_MSVCP_PATH "")
	endif()

	# InstallRequiredSystemLibraries (as of CMake 3.14.5) doesn't include msvcp140_1.dll or msvcp140_2.dll in its list.
	if(EXISTS ${MSVC_RUNTIME_MSVCP_PATH})
		get_filename_component(MSVC_RUNTIME_MSVCP_DIR ${MSVC_RUNTIME_MSVCP_PATH} DIRECTORY)
		message(STATUS "Found MSVC Redist directory: ${MSVC_RUNTIME_MSVCP_DIR}")

		file(GLOB MSVC_RUNTIME_MSVCP_FILES "${MSVC_RUNTIME_MSVCP_DIR}/msvcp*.dll")

		set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} ${MSVC_RUNTIME_MSVCP_FILES})

		list(REMOVE_DUPLICATES CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
	else()
		message(WARNING "InstallRequiredSystemLibraries did not find any msvcp runtime dll")
	endif()

	list(LENGTH CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS MSVC_RUNTIME_COUNT)
	message(STATUS "Found ${MSVC_RUNTIME_COUNT} Visual C/C++ Runtime files")

	set(MEGA_MSVC_DLLS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS})
endif()


set(MEGA_LUA51_VER "5.1.5")
set(MEGA_LUAJIT_VER "2.1.1710088188")

set(SKIP_INSTALL_ALL TRUE)

message(STATUS "-----------------------------------------------------")
message(STATUS "Configuring: lua ${MEGA_LUA51_VER}")
message(STATUS "-----------------------------------------------------")
add_subdirectory("libs/lua-${MEGA_LUA51_VER}" ${CMAKE_BINARY_DIR}/lua51)
set(MEGA_LUA51 lua51)

# LuaJIT
if(MSVC)
	message(STATUS "-----------------------------------------------------")
	message(STATUS "Configuring: LuaJIT ${MEGA_LUAJIT_VER}")
	message(STATUS "-----------------------------------------------------")
	set(MEGA_LUAJIT_DIR "LuaJIT")

	# Copy LuaJIT source to binary dir. LuaJIT builds in-source,
	# and we don't want to modify the original source tree, so
	# we copy it.
	file(COPY libs/${MEGA_LUAJIT_DIR} DESTINATION ${CMAKE_BINARY_DIR}/libs)

	set(MEGA_LUAJIT_SOURCE_DIR ${CMAKE_BINARY_DIR}/libs/${MEGA_LUAJIT_DIR})

	set(MEGA_LUAJIT_BUILD_BAT "
		if not defined DevEnvDir (
			call \"${MEGA_MSVC_VCVARSALL_BAT}\" ${MEGA_MSVC_VCVARSALL_BAT_ARG}
		)
		cd \"${MEGA_LUAJIT_SOURCE_DIR}/src\"
		cl
		msvcbuild.bat debug amalg
	")

	file(WRITE ${MEGA_LUAJIT_SOURCE_DIR}/megabuild.bat ${MEGA_LUAJIT_BUILD_BAT})

	ExternalProject_Add(luajit
		SOURCE_DIR ${MEGA_LUAJIT_SOURCE_DIR}
		BUILD_IN_SOURCE 1
		CONFIGURE_COMMAND ""
		BUILD_COMMAND "${MEGA_LUAJIT_SOURCE_DIR}/megabuild.bat"
		INSTALL_COMMAND cmake -E make_directory ${CMAKE_BINARY_DIR}/bin
			COMMAND cmake -E copy ${MEGA_LUAJIT_SOURCE_DIR}/src/lua51.dll ${CMAKE_BINARY_DIR}/bin
			COMMAND cmake -E copy ${MEGA_LUAJIT_SOURCE_DIR}/src/lua51.lib ${CMAKE_BINARY_DIR}/bin
		BUILD_BYPRODUCTS "${CMAKE_BINARY_DIR}/bin/lua51.lib" "${CMAKE_BINARY_DIR}/bin/lua51.dll"
	)
	set_target_properties(luajit PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${MEGA_LUAJIT_SOURCE_DIR}/src"
		IMPORTED_IMPLIB "${CMAKE_BINARY_DIR}/bin/lua51.lib"
		IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/bin/lua51.dll"
	)
	set(MEGA_LUAJIT_LIB ${CMAKE_BINARY_DIR}/bin/lua51.lib)
	set(MEGA_LUAJIT_DLL ${CMAKE_BINARY_DIR}/bin/lua51.dll)
	set(MEGA_LUAJIT_INCLUDE ${MEGA_LUAJIT_SOURCE_DIR}/src)
elseif(ANDROID)
	# LuaJIT is prebuilt in Android
	message(STATUS "-----------------------------------------------------")
	message(STATUS "Configuring: LuaJIT ${MEGA_LUAJIT_VER}")
	message(STATUS "-----------------------------------------------------")
	set(MEGA_LUAJIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/LuaJIT")

	# add_library(luajit IMPORTED SHARED GLOBAL)
	add_library(luajit STATIC IMPORTED GLOBAL)
	set_target_properties(luajit PROPERTIES
		IMPORTED_LOCATION ${MEGA_LUAJIT_DIR}/android/${ANDROID_ABI}/libluajit.so
	)
	target_include_directories(luajit INTERFACE ${MEGA_LUAJIT_DIR}/src ${MEGA_LUAJIT_DIR}/android/${ANDROID_ABI})
	set(MEGA_LUAJIT_LIB ${MEGA_LUAJIT_DIR}/android/${ANDROID_ABI}/libluajit.so)
	set(MEGA_LUAJIT_DLL)
	set(MEGA_LUAJIT_INCLUDE ${MEGA_LUAJIT_DIR}/src ${MEGA_LUAJIT_DIR}/android/${ANDROID_ABI})
	set(LUA_FOUND ON)
	set(LUA_INCLUDE_DIR ${MEGA_LUAJIT_INCLUDE})
	set(LUA_LIBRARY luajit)
	set(LUA_VERSION_MAJOR 5)
	set(LUA_VERSION_MINOR 1)
	set(LUA_VERSION_PATCH 5)
	set(LUA_VERSION_STRING "LuaJIT ${MEGA_LUAJIT_VER}")
endif()

set(MEGA_3P
	${MEGA_LUA51}
	${MEGA_MODPLUG}
)

# if(EXISTS ${MEGA_LOVE}/CMakeLists.txt)
# 	message(STATUS "-----------------------------------------------------")
# 	message(STATUS "Configuring: love")
# 	message(STATUS "-----------------------------------------------------")
# 	add_subdirectory(${MEGA_LOVE} ${CMAKE_BINARY_DIR}/love)

# 	# Turn off third-party warnings when building with LOVE.
# 	foreach(MEGA_LIB ${MEGA_3P})
# 		disable_warnings(${MEGA_LIB})
# 	endforeach()

# 	if(MSVC)
# 		set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT love)
# 	endif()
# else()
# 	message(STATUS "LOVE was not found")
# endif()

add_executable(megatest src/test.cpp)
target_link_libraries(megatest ${MEGA_3P})

# add_move_dll(megatest ${MEGA_SDL2} ${MEGA_SDL3})
# add_move_dll(megatest ${MEGA_LUA51})
