
set(LIBNAME
    cpp-ssh
)

file(GLOB SOURCES_LIBS *.cpp)

add_library(${LIBNAME} ${SOURCES_LIBS})

target_include_directories(${LIBNAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(${LIBNAME} PUBLIC
    ssh
)