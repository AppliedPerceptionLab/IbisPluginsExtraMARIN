set( AutoIgtlPath ${IBIS_EXTERNAL_DEPENDENCIES_DIR}/OpenIGTLink/build )
find_package( OpenIGTLink REQUIRED PATHS ${AutoIgtlPath} )
set( AutoIgtlIOPath ${IBIS_EXTERNAL_DEPENDENCIES_DIR}/OpenIGTLinkIO/build )
find_package( OpenIGTLinkIO REQUIRED PATHS ${AutoIgtlIOPath} )

set( PluginSrc commandsprocessingplugininterface.cpp commandsprocessingwidget.cpp )
set( PluginHdrMoc commandsprocessingwidget.h commandsprocessingplugininterface.h )
set( PluginUi commandsprocessingwidget.ui )

DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )
target_link_libraries( ${PluginName} ${OpenIGTLinkIO_LIBRARIES} ${OpenIGTLink_LIBRARIES}  )
