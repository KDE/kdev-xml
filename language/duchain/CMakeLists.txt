
add_subdirectory(navigation)

enable_testing()

find_package(KDevelop-PG-Qt REQUIRED)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}/parser
    ${CMAKE_CURRENT_SOURCE_DIR}/parser
    ${CMAKE_CURRENT_BINARY_DIR}/schema
    ${CMAKE_CURRENT_SOURCE_DIR}/schema
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${KDEVPGQT_INCLUDE_DIR}
    ${KDEVPLATFORM_INCLUDE_DIR}
    ${KDE4_INCLUDES}
    ${QT4_INCLUDES}
    ${xml_SOURCE_DIR}
)

kdevpgqt_generate(_kdevpgList sgml NAMESPACE Xml DEBUG_VISITOR
    "${CMAKE_CURRENT_SOURCE_DIR}/parser/sgml.g"
    "${CMAKE_CURRENT_SOURCE_DIR}/parser/sgmltokenizer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/parser/dtdtokenizer.h"
)

set( parser_SRCS
    ${_kdevpgList}
)

set(parser_STAT_SRCS
    ../language_debug.cpp
    parser/tokenizer.cpp
    parser/sgmltokenizer.cpp
    parser/dtdtokenizer.cpp
    parser/parsesession.cpp
    parser/editorintegrator.cpp
    parser/dtdhelper.cpp
)

set( duchain_SRCS
    elementdeclaration.cpp
    importdeclaration.cpp
    declarationbuilder.cpp
    contextbuilder.cpp
    schemabuilder.cpp
    includebuilder.cpp
    navigation/navigationwidget.cpp
    schema/schema.cpp
)


kde4_add_library( kdevsgmlduchain SHARED ${duchain_SRCS} ${parser_SRCS} ${parser_STAT_SRCS} )
target_link_libraries(kdevsgmlduchain
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KTEXTEDITOR_LIBS}
    ${KDEVPLATFORM_INTERFACES_LIBRARIES}
    ${KDEVPLATFORM_LANGUAGE_LIBRARIES}
    ${KDEVPLATFORM_SHELL_LIBRARIES}
    ${KDEVPLATFORM_PROJECT_LIBRARIES}
    ${KDEVPLATFORM_TESTS_LIBRARIES}
    ${KDE4_THREADWEAVER_LIBRARIES}
    kdevxmlcatalog
)

install(TARGETS kdevsgmlduchain DESTINATION ${INSTALL_TARGETS_DEFAULT_ARGS})

### next target
set(duchaintest_SRCS tests/duchain.cpp tests/duchaintestbase.cpp)
kde4_add_unit_test(duchaintest ${duchaintest_SRCS})
target_link_libraries(duchaintest
    ${QT_QTTEST_LIBRARY}
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KTEXTEDITOR_LIBS}
    ${KDEVPLATFORM_INTERFACES_LIBRARIES}
    ${KDEVPLATFORM_LANGUAGE_LIBRARIES}
    ${KDEVPLATFORM_SHELL_LIBRARIES}
    ${KDEVPLATFORM_PROJECT_LIBRARIES}
    ${KDEVPLATFORM_TESTS_LIBRARIES}
    kdevsgmlduchain
    kdevxmlcatalog
)

### next target
kde4_add_executable(sgml-parser parser/tests/main.cpp ${parser_SRCS} ${parser_STAT_SRCS} )
target_link_libraries(sgml-parser ${QT_QTCORE_LIBRARY} ${KDEVPLATFORM_LANGUAGE_LIBRARIES} ${KDEVPLATFORM_INTERFACES_LIBRARIES} ${KDEVPLATFORM_TESTS_LIBRARIES} kdevsgmlduchain kdevxmlcatalog ${KDE4_THREADWEAVER_LIBRARIES})

### next target
set(parsertest_SRCS parser/tests/parsertest.cpp tests/duchaintestbase.cpp)
kde4_add_unit_test(sgmlparsertest ${parsertest_SRCS})
target_link_libraries(sgmlparsertest
    ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARIES}
    ${KDE4_KTEXTEDITOR_LIBS}
    ${KDEVPLATFORM_INTERFACES_LIBRARIES}
    ${KDEVPLATFORM_LANGUAGE_LIBRARIES}
    ${KDEVPLATFORM_SHELL_LIBRARIES}
    ${KDEVPLATFORM_PROJECT_LIBRARIES}
    ${KDEVPLATFORM_TESTS_LIBRARIES}
    kdevsgmlduchain kdevxmlcatalog)

### next target
set(sgmltokenizertest_SRCS parser/tests/sgmltokenizertest.cpp tests/duchaintestbase.cpp)
kde4_add_unit_test(sgmltokenizertest ${sgmltokenizertest_SRCS})
target_link_libraries(sgmltokenizertest
    ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARIES} ${KDE4_KDECORE_LIBS}
    ${KDEVPLATFORM_INTERFACES_LIBRARIES}
    ${KDEVPLATFORM_LANGUAGE_LIBRARIES}
    ${KDEVPLATFORM_SHELL_LIBRARIES}
    ${KDEVPLATFORM_PROJECT_LIBRARIES}
    ${KDEVPLATFORM_TESTS_LIBRARIES}
    kdevsgmlduchain kdevxmlcatalog)

get_property(inc_dirs DIRECTORY PROPERTY INCLUDE_DIRECTORIES)
get_property(ln_dirs DIRECTORY PROPERTY LINK_DIRECTORIES)
message(STATUS "include_directories: ${inc_dirs}")
message(STATUS "link_directories: ${ln_dirs}")

install(FILES
    elementdeclaration.h
    duchainexport.h
    parser/parserexport.h
    parser/dtdhelper.h
    schema/schema.h
    DESTINATION ${INCLUDE_INSTALL_DIR}/kdevsgmlduchain
    COMPONENT Devel
)
