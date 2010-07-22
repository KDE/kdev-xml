

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_LIBXML2 REQUIRED libxml-2.0)

set(kdevxmlvalidator_SRCS
  xmlvalidator.cpp
  xmlvalidatorplugin.cpp
  xmlschemadialog.cpp
  xmlvalidatorjob.cpp
)

include_directories(${xml_SOURCE_DIR} ${PC_LIBXML2_INCLUDE_DIRS})

kde4_add_plugin(kdevxmlvalidator ${kdevxmlvalidator_SRCS})

target_link_libraries(kdevxmlvalidator
  ${KDE4_KDEUI_LIBS}
  ${KDEVPLATFORM_INTERFACES_LIBRARIES}
  kdevxmlcatalog
  ${PC_LIBXML2_LIBRARIES}
  ${QT4_XML_LIBRARIES}
)

install(TARGETS kdevxmlvalidator DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install(FILES kdevxmlvalidator.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES kdevxmlvalidator.rc DESTINATION ${DATA_INSTALL_DIR}/kdevxmlsupport)
