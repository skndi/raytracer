cmake_minimum_required(VERSION 3.0.0)

project(RTInOneWeekend)

set(HEADERS
	src/Material.h
	src/Image.hpp
	src/Primitive.h
	src/Utils.hpp
	src/Threading.hpp

	src/third_party/stb_image_write.h
)

set(SOURCES
	src/Material.cpp
	src/Primitive.cpp
	src/main.cpp
)


add_executable(${PROJECT_NAME} "${SOURCES};${HEADERS}")
target_include_directories(${PROJECT_NAME} PRIVATE src/third_party)
