add_definitions(-DTRANSLATION_DOMAIN=\"ksysguard_plugins_global\")

set(KSYSTEMSTATS_PLUGIN_INSTALL_DIR ${KDE_INSTALL_PLUGINDIR}/ksystemstats)

add_subdirectory(osinfo)
add_subdirectory(network)
add_subdirectory(power)
add_subdirectory(disks)
add_subdirectory(cpu)
add_subdirectory(memory)

if(UDev_FOUND)
    add_subdirectory(gpu)
endif()
