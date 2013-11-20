
set(kdevsgmlformatter_PART_SRCS
  formatter_debug.cpp
  sgmlformatter.cpp
  xmlformatter.cpp
  htmlformatter.cpp
  formatterplugin.cpp
  formatterpreferences.cpp
)

include_directories(
    ${KDEVPLATFORM_INCLUDE_DIR}
    ${KDE4_INCLUDES}
    ${QT4_INCLUDES}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${xml_SOURCE_DIR}/language/duchain
    ${xml_BINARY_DIR}/language/duchain
    ${xml_SOURCE_DIR}/language/duchain/parser
    ${xml_BINARY_DIR}/language/duchain/parser
)

kde4_add_ui_files(kdevsgmlformatter_PART_SRCS formatterwidget.ui)

kde4_add_plugin(kdevsgmlformatter ${kdevsgmlformatter_PART_SRCS})

target_link_libraries(kdevsgmlformatter
  ${KDE4_KDEUI_LIBS}
  ${KDEVPLATFORM_INTERFACES_LIBRARIES}
  kdevsgmlduchain
)

install(TARGETS kdevsgmlformatter DESTINATION ${PLUGIN_INSTALL_DIR})

##next target

set(formattertest_SRCS tests/formattertest.cpp sgmlformatter.cpp htmlformatter.cpp xmlformatter.cpp formatter_debug.cpp)
kde4_add_unit_test(formattertest ${formattertest_SRCS})
target_link_libraries( formattertest
    ${QT_QTTEST_LIBRARY}
    ${KDE4_KDEUI_LIBS}
    ${KDEVPLATFORM_INTERFACES_LIBRARIES}
    kdevsgmlduchain
)

########### install files ###############

configure_file(kdevsgmlformatter.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/kdevsgmlformatter.desktop)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kdevsgmlformatter.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES kdevsgmlformatter.rc DESTINATION ${DATA_INSTALL_DIR}/kdevsgmlformatter)

