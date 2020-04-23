find_package(OpenIGTLink REQUIRED)

set( PluginSrc openigtlsenderplugininterface.cpp openigtlsenderwidget.cpp )
set( PluginHdrMoc openigtlsenderwidget.h openigtlsenderplugininterface.h )
set( PluginUi openigtlsenderwidget.ui )

DefinePlugin( "${PluginSrc}" "${PluginHdr}" "${PluginHdrMoc}" "${PluginUi}" )
target_link_libraries( ${PluginName} ${OpenIGTLink_LIBRARIES} )

find_library( YUV_LIB libyuv.a "/optional/path/to/libyuv/build" )
find_path( YUV_LIB_HEADERS libyuv.h /optional/path/libyuv/include )

include_directories( ${YUV_LIB_HEADERS} )

target_link_libraries( ${PluginName} ${YUV_LIB} )
