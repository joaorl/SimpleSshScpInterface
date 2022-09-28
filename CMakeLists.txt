
cmake_minimum_required (VERSION 3.16)

project (CppSsh)

set(application_name "cppsshdemo")

add_executable (${application_name} main.cpp)

add_subdirectory (Libs/CppSsh/ CppSshLib)

target_link_libraries (${application_name} LINK_PUBLIC
    cpp-ssh
)

