

add_subdirectory(duchain)
add_subdirectory(completion)

find_package(KDevelop-PG-Qt REQUIRED)

include_directories(
    ${KDEVPLATFORM_INCLUDE_DIR}
    ${KDE4_INCLUDES}
    ${QT4_INCLUDES}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/duchain
    ${CMAKE_CURRENT_SOURCE_DIR}/duchain
    ${CMAKE_CURRENT_SOURCE_DIR}/duchain/parser
    ${CMAKE_CURRENT_BINARY_DIR}/duchain/parser
    ${KDEVPGQT_INCLUDE_DIR}
    ${xml_SOURCE_DIR}
)
set(kdevsgmllanguagesupport_PART_SRCS
    languagesupport.cpp
    language_debug.cpp
    parsejob.cpp
)

kde4_add_plugin(kdevsgmllanguagesupport ${kdevsgmllanguagesupport_PART_SRCS})

target_link_libraries(kdevsgmllanguagesupport
    ${KDE4_KDEUI_LIBS}
    ${KDEVPLATFORM_INTERFACES_LIBRARIES}
    ${KDEVPLATFORM_LANGUAGE_LIBRARIES}
    ${KDE4_KTEXTEDITOR_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
    kdevsgmlduchain
    kdevsgmlcompletion
    kdevxmlcatalog
)

install(TARGETS kdevsgmllanguagesupport DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES kdevsgmlsupport.desktop DESTINATION ${SERVICES_INSTALL_DIR})

