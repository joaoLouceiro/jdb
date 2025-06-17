# Create a target called libjdb with a single source file, libjdb.cpp
add_library(libjdb libjdb.cpp)
# create a namespaced library target, which can result in more understandable errors
add_library(jdb::libjdb ALIAS libjdb)

# By default, Linux creates libraries called "lib<target name>".
# In our case, that would create a library called "liblibjdb".
# To avoid this, we specify that we want the OUTPUT_NAME of our
# target to be called "jdb"
set_target_properties(
    libjdb
    PROPERTIES OUTPUT_NAME jdb
)

# specify that the target should be compiled to C++ 17
target_compile_features(libjdb PUBLIC cxx_std_17)

# specify the direcories for libjdb
# jdb/src/include is set to PRIVATE
# jdb/include is set to PUBLIC
target_include_directories(
    libjdb
    PRIVATE include
    PUBLIC ${CMAKE_SOURCE_DIR}/include
)
