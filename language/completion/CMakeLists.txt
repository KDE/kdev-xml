add_subdirectory(tests)

find_package(KDevelop-PG-Qt REQUIRED)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${KDEVPGQT_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/../duchain
    ${CMAKE_CURRENT_SOURCE_DIR}/../duchain
    ${CMAKE_CURRENT_BINARY_DIR}/../duchain/parser
    ${CMAKE_CURRENT_SOURCE_DIR}/../duchain/parser
)

set(sgmlcompletion_SRCS
    ../language_debug.cpp
    sgmlcodecompletionmodel.cpp
    completionsupport.cpp
    completionbase.cpp
    completionitem.cpp
    xsdcompletionsupport.cpp
    schemacompletionsupport.cpp
    dtdcompletionsupport.cpp
    htmlcompletionsupport.cpp
)


set(sgmlcompletion_PICS_RCCS pics/pics.qrc )
qt4_add_resources(sgmlcompletion_PICS_SRCS  ${sgmlcompletion_PICS_RCCS})

kde4_add_library( kdevsgmlcompletion SHARED ${sgmlcompletion_SRCS} ${sgmlcompletion_PICS_SRCS} )
target_link_libraries(kdevsgmlcompletion
    ${KDE4_KDECORE_LIBS}
    ${KDEVPLATFORM_LANGUAGE_LIBRARIES}
    ${KDE4_KTEXTEDITOR_LIBS}
    ${KDEVPLATFORM_INTERFACES_LIBRARIES}
    ${KDEVPLATFORM_PROJECT_LIBRARIES}
    ${KDE4_KIO_LIBS}
    kdevsgmlduchain
 )

install(TARGETS kdevsgmlcompletion DESTINATION ${INSTALL_TARGETS_DEFAULT_ARGS})
