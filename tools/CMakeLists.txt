add_executable(jdb jdb.cpp)
target_link_libraries(
    jdb PRIVATE jdb::libjdb
)
