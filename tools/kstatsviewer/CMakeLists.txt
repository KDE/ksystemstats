set(SOURCES
    main.cpp
)

add_executable(kstatsviewer ${SOURCES})
target_link_libraries(kstatsviewer Qt::Core Qt::DBus KSysGuard::Formatter KSysGuard::SystemStats)

install(TARGETS kstatsviewer DESTINATION ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
