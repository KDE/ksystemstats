include(ECMAddTests)

set(SOURCES
    main.cpp
)

ecm_add_test(
    ${SOURCES}
    TEST_NAME ksystemstatstest
    LINK_LIBRARIES Qt::Test ksystemstats_core
)
