
#add_definitions( -v -Wall -fno-inline-functions -O0)

set(KDEVXMLCATALOG_LIB_VERSION 1.0.0)
set(KDEVXMLCATALOG_LIB_SOVERSION 1)

set(kdevxmlcatalog_LIB_SRCS
    catalog_debug.cpp
    cataloghelper.cpp
    documentcachemanager.cpp
    idocumentcachemanager.cpp
    catalogmanager.cpp
    icataloggroup.cpp
    icatalogmanager.cpp
    icatalog.cpp
    icatalogentry.cpp
    icatalogreaderwriter.cpp
    publicid.cpp
    oasiscatalog.cpp
    oasiscatalogreaderwriter.cpp
    sgmlcatalog.cpp
    sgmlcatalogreaderwriter.cpp
)

kde4_add_library(kdevxmlcatalog SHARED ${kdevxmlcatalog_LIB_SRCS})


target_link_libraries(kdevxmlcatalog
    ${KDE4_KIO_LIBS}
    ${QT_QTXML_LIBRARY}
)

set_target_properties(kdevxmlcatalog PROPERTIES
    VERSION ${KDEVXMLCATALOG_LIB_VERSION}
    SOVERSION ${KDEVXMLCATALOG_LIB_SOVERSION}
)

install(TARGETS kdevxmlcatalog EXPORT KDevPlatformTargets ${INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES
    icatalog.h
    icatalogentry.h
    icataloggroup.h
    icatalogreaderwriter.h
    icatalogmanager.h
    cataloghelper.h
    parameterized.h
    idocumentcachemanager.h
    publicid.h
    DESTINATION ${INCLUDE_INSTALL_DIR}/kdevplatform/kdevxmlcatalog
    COMPONENT Devel
)

install(FILES 
    catalogs/config.xml
    catalogs/KDE.xml 
    catalogs/kcfg.dtd
    catalogs/kpartgui.dtd
    catalogs/HTML.xml
    catalogs/XML.xml
    catalogs/docbookx.xml
    catalogs/docbookx.dtd
    DESTINATION ${DATA_INSTALL_DIR}/kdevxmlcatalog/catalogs
)

add_subdirectory(tests)
