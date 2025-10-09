add_executable(jdb jdb.cpp)
target_link_libraries(
    jdb PRIVATE jdb::libjdb
    PkgConfig::libedit
    fmt::fmt
)

include(GNUInstallDirs)
install(
    TARGETS jdb
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
