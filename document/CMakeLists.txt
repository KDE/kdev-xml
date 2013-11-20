
set(kdevxmldocument_SRCS
    xmldocumentplugin.cpp
    xmldocument.cpp
    xmldocumentview.cpp
    xmltree.cpp
    abstractxmlmodel.cpp
    xmldocumentmodel.cpp
    #htmldocumentmodel.cpp
    xsddocumentmodel.cpp
)

set(kdevxmldocument_PICS_RCCS pics/xmlpics.qrc )
qt4_add_resources(kdevxmldocument_PICS_SRCS  ${kdevxmldocument_PICS_RCCS})


kde4_add_plugin(kdevxmldocument ${kdevxmldocument_SRCS} ${kdevxmldocument_PICS_SRCS})

target_link_libraries(kdevxmldocument
  ${KDE4_KDEUI_LIBS}
  ${KDE4_THREADWEAVER_LIBRARIES}
  ${KDEVPLATFORM_INTERFACES_LIBRARY}
  ${KDEVPLATFORM_INTERFACES_LIBRARIES}
  ${KDEVPLATFORM_LANGUAGE_LIBRARIES}
  ${KDEVPLATFORM_SHELL_LIBRARIES}
  ${KDEVPLATFORM_SUBLIME_LIBRARIES}
  ${KDE4_KTEXTEDITOR_LIBS}
)

install(TARGETS kdevxmldocument DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

configure_file(kdevxmldocument.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/kdevxmldocument.desktop)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kdevxmldocument.desktop DESTINATION ${SERVICES_INSTALL_DIR})
