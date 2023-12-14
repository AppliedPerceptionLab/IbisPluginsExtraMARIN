#ifndef __OpenIGTLSenderPluginInterface_h_
#define __OpenIGTLSenderPluginInterface_h_

#include <QObject>

#include "ibisapi.h"
#include "toolplugininterface.h"
#include "view.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkMultiThreader.h"
#include "vtkImageFlip.h"

#include "igtl_header.h"
#include "igtlOSUtil.h"
#include "VideoStreaming/igtlVideoMessage.h"
#include "igtlImageMessage.h"
#include "igtlStatusMessage.h"
#include "igtlServerSocket.h"
#include "VideoStreaming/igtl_video.h"
#include "VideoStreaming/igtlH264Encoder.h"
#include "VideoStreaming/igtlH264Decoder.h"
#include "igtlUDPServerSocket.h"
#include "igtlMessageRTPWrapper.h"
#include "igtlMultiThreader.h"

#include "openigtlsenderwidget.h"

//User defined parameters to tweak:
#define SEND_WIDTH 800
#define SEND_HEIGHT 600
#define SEND_ASPECT_RATIO (1.*SEND_WIDTH/SEND_HEIGHT)

#define SEND_VIDEO_ON_STARTUP true
#define IMAGE_SEND_MODE RC_BITRATE_MODE
#define TARGET_BIT_RATE 16000000

#define CONFIG_FILE_PATH "/Path/To/OH264Config.cfg"

#define CLIENT_ADDRESS "192.168.0.8"

#define DEVICE_NAME "IBIS"
#define VIDEO_PORT 18946
#define STATUS_PORT 18949

#define MARIN_CAMERA_NAME_IN_CONFIG "iPad"

using namespace igtl;
using namespace std;

enum MAIN_VIEW_TYPE{ _3D, SAGITTAL, CORONAL, TRANSVERSE };

class QLabel;
class QPushButton;
class QUdpSocket;
class vtkWindowToImageFilter;
class vtkRendererSource;
class vtkImageResize;
class VideoStreamIGTLinkServer;

class OpenIGTLSenderPluginInterface : public ToolPluginInterface{

    Q_OBJECT
    Q_INTERFACES(IbisPlugin)
    Q_PLUGIN_METADATA(IID "Ibis.OpenIGTLSenderPluginInterface" )

public:
    OpenIGTLSenderPluginInterface();
    ~OpenIGTLSenderPluginInterface();
    virtual QString GetPluginName() { return QString("OpenIGTLSender"); }
    virtual bool CanRun();
    virtual QString GetMenuEntryString() { return QString("OpenIGTLink Sender"); }

    virtual QWidget * CreateTab();
    virtual bool WidgetAboutToClose();
    void connectVideo();
    void connectStatus();

    bool set_dimensions( int w, int h );
    void toggleVideo(){
        sending_video = !sending_video;
        widget->updateui();
    }
    void toggleStatus(){
        sending_status = !sending_status;
        widget->updateui();
    }
    void changeBandwidth( int kbps );
    void setOffScreen( int state ){
        isOffScreen = (bool)state;
        widget->updateui();
    }
    void ToggleQuadView( bool b ){
        displayQuadFlag = b;
        widget->updateui();
    }
    void displayQuadView();
    QImage getMainView(MAIN_VIEW_TYPE viewType, IbisAPI * api, bool quadView);
    QImage getMainView(MAIN_VIEW_TYPE viewType, IbisAPI * api);

    bool GetSendingVideo() { return sending_video; }
    bool GetConnectedStatus() { return connected_status; }
    bool GetConnectingStatus() { return connecting_status; }
    bool GetSocketCreatedStatus() { return status_socket_created; }

    bool SetClientAddress( QString s );
    std::string GetClientAddress() { return client_address; }

    OpenIGTLSenderWidget * widget;

    IbisAPI * api;
    View * view;
    vtkRenderer * renderer;
    vtkRenderWindow * window;

    ushort port_video = VIDEO_PORT;
    ushort port_status = STATUS_PORT;
    int version = 4;
    std::string client_address = CLIENT_ADDRESS;
    int clientID = -1;
    uint connection_wait_time = 100000000;
    igtl::TimeStamp::Pointer Timer;
    igtlUint32 currentMessageID = 0;

    igtl::UDPServerSocket::Pointer videoServerSocket;
    igtl::MessageRTPWrapper::Pointer rtpWrapper;
    igtl::MutexLock::Pointer glock;

    igtl::Socket::Pointer socket;
    igtl::ServerSocket::Pointer statusServerSocket;

    bool connected_video = false;
    bool sending_video = false;
    bool video_socket_created = false;

    bool connecting_status = false;
    bool connected_status = false;
    bool sending_status = false;
    bool status_socket_created = false;

    vtkMultiThreader * Threader;
    int ConnectThreadId;
    int SendThreadId;
    bool sendTerminated = false;

    uchar * buffer1;
    //TODO: add a second buffer
//    uchar * buffer2;
    bool init_done = false;
    bool reading = false;
    vtkSmartPointer<vtkWindowToImageFilter> windowToImageFilter;
    vtkSmartPointer<vtkImageResize> resize;
    vtkImageData * img_dat;
    vtkSmartPointer<vtkRendererSource> rendererSource;

    int window_width = SEND_WIDTH;
    int window_height = SEND_HEIGHT;
    QImage window_image, scaled, croped;

    GenericEncoder * encoder = nullptr;
    H264Encoder* H264StreamEncoder;
    bool video_to_send = false;

    bool trackingOK = false;
    bool status_to_send = false;
    void checkTrackingState( IbisAPI * api );

    //TODO: not yet implemented
    bool isOffScreen = true;

    bool displayQuadFlag = false;

private slots:
    void OnUpdate();
    void ToggleQuadViewSlot( bool b ){
        ToggleQuadView( b );
    }

signals:
    void Modified();

};

#endif
