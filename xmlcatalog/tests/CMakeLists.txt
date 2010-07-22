#set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

#add_definitions( -v -Wall -fno-inline-functions -O0)

include_directories(
${_SOURCE_DIR}
${xml_SOURCE_DIR}/xmlcatalog
)

set( xmlcatalogtest_SRCS oasiscatalogtest.cpp )

kde4_add_unit_test( oasiscatalogtest ${xmlcatalogtest_SRCS} )

target_link_libraries( oasiscatalogtest
    ${QT_QTTEST_LIBRARY}
    ${KDE4_KDEUI_LIBS}
    ${QT_QTXML_LIBRARY}
    kdevxmlcatalog
)
