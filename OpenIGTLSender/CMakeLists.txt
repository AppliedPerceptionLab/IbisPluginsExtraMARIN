find_package( OpenIGTLink REQUIRED )
find_package( OpenIGTLinkIO REQUIRED )

set( PluginSrc openigtlsenderplugininterface.cpp openigtlsenderwidget.cpp )
set( PluginHdrMoc openigtlsenderwidget.h openigtlsenderplugininterface.h )
set( PluginUi openigtlsenderwidget.ui )

DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )
target_link_libraries( ${PluginName} ${OpenIGTLinkIO_LIBRARIES} ${OpenIGTLink_LIBRARIES} )

# The sender captures the axial and sagittal reslice views of the navigation plugin. Each plugin
# exports its own source directory as PUBLIC, so linking the target also brings in its header.
# PedicleScrewNavigation therefore has to be enabled in the build.
target_link_libraries( ${PluginName} PedicleScrewNavigation )

# libyuv is no longer needed: frames are JPEG-encoded with Qt, not converted to YUV for H.264.
