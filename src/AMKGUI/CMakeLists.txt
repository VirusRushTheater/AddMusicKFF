cmake_minimum_required(VERSION 3.16)

project(AMKGUI
	VERSION		1.0.8
	DESCRIPTION	"AddmusicK GUI"
	LANGUAGES	CXX
)

# Find Qt5 or Qt6
find_package(Qt5 QUIET COMPONENTS Widgets)
if (NOT Qt5_FOUND)
	find_package(Qt6 QUIET COMPONENTS Widgets)
	if (NOT Qt6_FOUND)
		message(FATAL_ERROR "Qt5 or Qt6 with Widgets component is required for AMKGUI")
	endif()
	set(QT_VERSION Qt6)
else()
	set(QT_VERSION Qt5)
endif()

message(STATUS "Using ${QT_VERSION} for AMKGUI")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(AMKGUI_SOURCES
	main.cpp
	MainWindow.cpp
	SongList.cpp
)

set(AMKGUI_HEADERS
	MainWindow.h
	SongList.h
)

add_executable(AMKGUI
	${AMKGUI_SOURCES}
	${AMKGUI_HEADERS}
)

target_link_libraries(AMKGUI
	PRIVATE
		${QT_VERSION}::Widgets
		${ADDMUSICKLIB_TARGETNAME}
		${ASAR_LIB_TARGET}
)

target_include_directories(AMKGUI
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}
		${CMAKE_SOURCE_DIR}/src
)

# Deployment
if (WIN32)
	if (${QT_VERSION} STREQUAL "Qt5")
		find_program(QT5_DEPLOYQT_EXECUTABLE windeployqt)
		if (QT5_DEPLOYQT_EXECUTABLE)
			add_custom_command(TARGET AMKGUI POST_BUILD
				COMMAND ${QT5_DEPLOYQT_EXECUTABLE} $<TARGET_FILE:AMKGUI>
				WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
			)
		endif()
	else()
		find_program(QT6_DEPLOYQT_EXECUTABLE windeployqt)
		if (QT6_DEPLOYQT_EXECUTABLE)
			add_custom_command(TARGET AMKGUI POST_BUILD
				COMMAND ${QT6_DEPLOYQT_EXECUTABLE} $<TARGET_FILE:AMKGUI>
				WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
			)
		endif()
	endif()
elseif (UNIX AND NOT APPLE)
	# For Linux, copy Qt libraries to bin directory
	if (${QT_VERSION} STREQUAL "Qt5")
		find_program(QT5_LINUXDEPLOY_EXECUTABLE linuxdeployqt)
		if (QT5_LINUXDEPLOY_EXECUTABLE)
			add_custom_command(TARGET AMKGUI POST_BUILD
				COMMAND ${QT5_LINUXDEPLOY_EXECUTABLE} $<TARGET_FILE:AMKGUI> -bundle-non-qt-libs
				WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
			)
		else()
			# Fallback: copy Qt libraries manually
			add_custom_command(TARGET AMKGUI POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${QT_VERSION}::Widgets> ${PROJECT_DEPLOY_DIR}
				WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
			)
		endif()
	else()
		find_program(QT6_LINUXDEPLOY_EXECUTABLE linuxdeployqt)
		if (QT6_LINUXDEPLOY_EXECUTABLE)
			add_custom_command(TARGET AMKGUI POST_BUILD
				COMMAND ${QT6_LINUXDEPLOY_EXECUTABLE} $<TARGET_FILE:AMKGUI> -bundle-non-qt-libs
				WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
			)
		else()
			# Fallback: copy Qt libraries manually
			add_custom_command(TARGET AMKGUI POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${QT_VERSION}::Widgets> ${PROJECT_DEPLOY_DIR}
				WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
			)
		endif()
	endif()
endif()
