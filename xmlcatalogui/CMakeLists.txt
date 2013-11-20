set(kdevxmlcatalogui_SRCS 
    xmlcatalogmodel.cpp
    xmlcatalogplugin.cpp
    xmlcatalogdialog.cpp
)

kde4_add_ui_files( kdevxmlcatalogui_SRCS xmlcatalogform.ui addschemaform.ui )

include_directories(${xml_SOURCE_DIR})

set(kdevxmlcatalogui_PICS_RCCS pics/catalogpics.qrc )
qt4_add_resources(kdevxmlcatalogui_PICS_SRCS  ${kdevxmlcatalogui_PICS_RCCS})

kde4_add_plugin(kdevxmlcatalogui ${kdevxmlcatalogui_SRCS} ${kdevxmlcatalogui_PICS_SRCS})


target_link_libraries(kdevxmlcatalogui
${KDE4_KDEUI_LIBS}
${KDEVPLATFORM_INTERFACES_LIBRARIES}
kdevxmlcatalog
)

install(TARGETS kdevxmlcatalogui DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

configure_file(kdevxmlcatalogui.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/kdevxmlcatalogui.desktop)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kdevxmlcatalogui.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES kdevxmlcatalogui.rc DESTINATION ${DATA_INSTALL_DIR}/kdevxmlcatalogui)
