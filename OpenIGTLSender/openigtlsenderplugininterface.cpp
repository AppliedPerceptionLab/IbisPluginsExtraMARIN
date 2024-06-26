#include "openigtlsenderplugininterface.h"
#include "scenemanager.h"
#include "pointerobject.h"
#include "application.h"

#include <igtlioConnector.h>

#include <vtkWindowToImageFilter.h>
#include <vtkRendererSource.h>
#include <vtkOpenGLRenderer.h>
#include <vtkImageResize.h>
#include <vtkSmartPointer.h>
#include <vtkPNGWriter.h>

#include <libyuv.h>

#include <QPainter>

/////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////   SEND_THREAD   ////////////////////////////////////////
static void * vtkOpenIGTLSenderSendThread( vtkMultiThreader::ThreadInfo * data )
{
    OpenIGTLSenderPluginInterface * self = (OpenIGTLSenderPluginInterface *)(data->UserData);
    self->set_dimensions( SEND_WIDTH, SEND_HEIGHT);
    igtl::StatusMessage::Pointer statusMsg = igtl::StatusMessage::New();
    igtl::VideoMessage::Pointer videoMessageSend = igtl::VideoMessage::New();
    uchar * converted = new uchar[ SEND_WIDTH * SEND_HEIGHT * 3 / 2];
    std::cout << "[SendThread] Created send thread." << std::endl;
    while( !(self->sendTerminated) ){
        if ( !self->init_done ||
             !self->connected_video ||
             ( !self->sending_video && !self->sending_status ) ||
             ( !self->video_to_send && !self->status_to_send ) ){
            //simtodo: tune this value
            igtl::Sleep(5);
            continue;
        }
        //update on the tracker status:
        if ( self->status_to_send && self->sending_status ){
            if( !self->connected_status && !self->connecting_status ){
                self->connectStatus();
            }
            if( self->connected_status ){
                statusMsg = igtl::StatusMessage::New();
                statusMsg->SetDeviceName( DEVICE_NAME );
                statusMsg->SetHeaderVersion( IGTL_HEADER_VERSION_2 );
                if ( self->trackingOK ){
                    statusMsg->SetCode(igtl::StatusMessage::STATUS_OK);
                    statusMsg->SetSubCode(128);
                    statusMsg->SetErrorName("OK!");
                }else{
                    statusMsg->SetCode(igtl::StatusMessage::STATUS_DISABLED);
                    statusMsg->SetSubCode(128);
                    statusMsg->SetErrorName("NOT_OK!");
                }
                //TODO: eventually add more information, but for now it's just OK / not_OK
                statusMsg->SetStatusString("OK or not OK?");
                statusMsg->Pack();
                std::cout << "[OpenIGTLSenderPlugin] Sending tracking status message." << std::endl << std::flush;
                int send_result = self->socket->Send( statusMsg->GetPackPointer(), statusMsg->GetPackSize() );
                if( send_result < 1 ){
                    std::cerr << "[OpenIGTLSenderPlugin] Status message couldn't be sent. Will try reconnecting and sending it again." << std::endl << std::flush;
                    self->connected_status = false;
                }else{
                    self->status_to_send = false;
                }
            }
        }
        //update on the video:
        if( self->video_to_send ){
            videoMessageSend = igtl::VideoMessage::New();
            videoMessageSend->SetHeaderVersion( IGTL_HEADER_VERSION_2 );
            videoMessageSend->SetDeviceName( DEVICE_NAME );
            videoMessageSend->SetMessageID( self->currentMessageID++ );
            SourcePicture * srcPic = new SourcePicture();
            srcPic->colorFormat = FormatI420;
            srcPic->picWidth = SEND_WIDTH;
            srcPic->picHeight = SEND_HEIGHT;
            self->reading = true;
            libyuv::ABGRToI420( self->buffer1, SEND_WIDTH*4, converted, SEND_WIDTH, converted + SEND_WIDTH * SEND_HEIGHT, SEND_WIDTH / 2, converted + SEND_WIDTH * SEND_HEIGHT * 5 / 4  , SEND_WIDTH / 2, SEND_WIDTH, SEND_HEIGHT);
//            self->encoder->ConvertRGBAToYUV( self->buffer1, converted, SEND_WIDTH, SEND_HEIGHT );
            self->reading = false;
            srcPic->data[0] = converted;
            srcPic->data[1] = converted + SEND_HEIGHT * SEND_WIDTH;
            srcPic->data[2] = converted + SEND_HEIGHT * SEND_WIDTH * 5 / 4;
            srcPic->stride[0] = SEND_WIDTH;
            srcPic->stride[1] = SEND_WIDTH / 4;
            srcPic->stride[2] = SEND_WIDTH / 4;
            self->Timer->GetTime();
            videoMessageSend->SetTimeStamp( self->Timer );
            //encode frame:
            int iEncFrames = self->encoder->EncodeSingleFrameIntoVideoMSG( srcPic, videoMessageSend, false );
            uchar * frame = new uchar[videoMessageSend->GetBufferSize()];
            std::memcpy( frame, videoMessageSend->GetPackPointer(), videoMessageSend->GetBufferSize());
            self->rtpWrapper->WrapMessageAndSend( self->videoServerSocket, frame, videoMessageSend->GetBufferSize() );
            delete[] frame;
            self->video_to_send = false;
        }
    }
    std::cerr << "[Sender] Send thread done." << std::endl << std::flush;
    return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////   CONNECT_THREAD   //////////////////////////////////////
static void * vtkOpenIGTLSenderConnectThread( vtkMultiThreader::ThreadInfo * data )
{
    //get pointer to self and initialize:
    OpenIGTLSenderPluginInterface * self = (OpenIGTLSenderPluginInterface *)(data->UserData);
    if( self->socket.IsNull() ){
    self->connecting_status = true;
    self->socket = self->statusServerSocket->WaitForConnection(self->connection_wait_time);
        std::cout << "[OpenIGTLSenderPlugin] Couldn't connect in time. (TCP for commands)" << std::endl << std::flush;
        self->connected_status = false;
    }else{
        std::cout << "[OpenIGTLSenderPlugin] Connected for commands" << std::endl << std::flush;
        //TODO: need a timeout?
//        self->socket->SetTimeout(3000);
        self->connected_status = true;
    }
    self->connecting_status = false;
    return nullptr;
}

OpenIGTLSenderPluginInterface::OpenIGTLSenderPluginInterface()
{
    videoServerSocket = igtl::UDPServerSocket::New();
    rtpWrapper = igtl::MessageRTPWrapper::New();
    Threader = vtkMultiThreader::New();
    glock = igtl::MutexLock::New();
    socket = igtl::Socket::New();
    statusServerSocket = igtl::ServerSocket::New();
    buffer1 = new uchar[ SEND_WIDTH * SEND_HEIGHT * 4 ];
    //TODO: eventually add a second buffer?
//    buffer2 = new uchar[ SEND_WIDTH * SEND_HEIGHT * 4 ];
    H264StreamEncoder = new H264Encoder( CONFIG_FILE_PATH );
    int r = statusServerSocket->CreateServer( port_status );
    if( r < 0 )
    {
        std::cerr << "[OpenIGTLSenderPlugin] Cannot create a server socket." << std::endl;
    }
}

bool OpenIGTLSenderPluginInterface::CanRun()
{
    return true;
}

QWidget * OpenIGTLSenderPluginInterface::CreateTab()
{
    //init variables:
    widget = new OpenIGTLSenderWidget;
    widget->SetPluginInterface( this );
    api = this->GetIbisAPI();
    view = api->GetMain3DView();
    renderer = view->GetRenderer( 0 );
    window = renderer->GetRenderWindow();
    Timer = igtl::TimeStamp::New();
    //connect to ibis update:
    connect( api , SIGNAL(IbisClockTick()), this, SLOT( OnUpdate() ) );
    connect( api, SIGNAL( NewCommandReceived( igtlioCommand * ) ), this, SLOT( OnCommandReceived( igtlioCommand * ) ) );
    SendThreadId = this->Threader->SpawnThread( (vtkThreadFunctionType)&vtkOpenIGTLSenderSendThread, this );
    connectStatus();
    //create vtk pipeline:
    windowToImageFilter = vtkSmartPointer<vtkWindowToImageFilter>::New();
    windowToImageFilter->SetInput( window );
    windowToImageFilter->SetInputBufferTypeToRGBA();
    windowToImageFilter->ReadFrontBufferOn();
    windowToImageFilter->Modified();
    sending_video = SEND_VIDEO_ON_STARTUP;
    videoServerSocket->SetPortNumber( port_video );
    //default value is set in widget:
    changeBandwidth( widget->getBandwidth() );
    return widget;
}

bool OpenIGTLSenderPluginInterface::WidgetAboutToClose()
{
    disconnect( static_cast<QObject*>( this->GetIbisAPI() ), SIGNAL( IbisClockTick() ), this, SLOT( OnUpdate() ) );
    sendTerminated = true;
    this->Threader->TerminateThread(SendThreadId);
    this->Threader->TerminateThread(ConnectThreadId);
    return true;
}

#include <vtkCamera.h>
void OpenIGTLSenderPluginInterface::OnUpdate()
{
    widget->updateui();
    if( !sending_video ){
        return;
    }
    else if( !connected_video && sending_video ){
        connectVideo();
        return;
    }else if( connected_video && sending_video ){
        if( m_displayingQuadView ){
            displayQuadView();
        }else{
            windowToImageFilter->Modified();
            windowToImageFilter->Update();
            img_dat = windowToImageFilter->GetOutput();
            int * dims = img_dat->GetDimensions();
            window_width = dims[0];
            window_height = dims[1];
            if (window_width == 0 || window_height == 0) {
                std::cout << "[OpenIGTLSenderPluginInterface::OnUpdate] No window to send" << std::endl;
                return;
            }
            uchar * pixels = static_cast<uchar*>( img_dat->GetScalarPointer() );
            window_image = QImage( pixels,
                                       window_width,
                                       window_height,
                                       QImage::Format_RGBA8888 );
            //Crop depends on aspect ratio of current window (since we're using the window for now).
            //simtodo: Eventually it would be better to use off-screen-rendering instead, that way I could set the renderer to the appropriate size directly.
            double window_aspect_ratio = window_width/window_height;
            QImage final = QImage( QSize( SEND_WIDTH, SEND_HEIGHT ), QImage::Format_RGBA8888_Premultiplied );
            QPainter painter( &final );
            painter.setCompositionMode( QPainter::CompositionMode_Source );
            painter.fillRect( final.rect(), Qt::green );
            painter.setCompositionMode( QPainter::CompositionMode_SourceOver );
            if( window_aspect_ratio < SEND_ASPECT_RATIO )
            {
                scaled = window_image.scaledToHeight( SEND_HEIGHT, Qt::SmoothTransformation );
                painter.drawImage( SEND_WIDTH/2 - scaled.width()/2, 0, scaled );
            }else
            {
                croped = window_image.copy( window_width/2 - window_height*SEND_ASPECT_RATIO/2, 0, window_height*SEND_ASPECT_RATIO, window_height );
                scaled = croped.scaledToHeight( SEND_HEIGHT, Qt::SmoothTransformation );
                painter.drawImage( 0, 0, scaled );
            }
            if ( !reading )
            {
                //TODO: add second buffer, or is that unnecessary?
                memcpy( buffer1, final.bits(), SEND_WIDTH * SEND_HEIGHT * 4 );
                video_to_send = true;
            }else
            {
                std::cerr << "[Sender] SKIPPED A FRAME" << std::endl;
            }
            init_done = true;
            checkTrackingState( api );
        }
    }
}

void OpenIGTLSenderPluginInterface::displayQuadView()
{
    QList<ImageObject*> objects = QList<ImageObject*>();
    api->GetAllImageObjects( objects );
    QImage qimg_col_3d = getMainView( MAIN_VIEW_TYPE::_3D, api, true );
    QImage qimg_col_transverse = getMainView( MAIN_VIEW_TYPE::TRANSVERSE, api, true );
    QImage qimg_col_sagittal = getMainView( MAIN_VIEW_TYPE::SAGITTAL, api, true );
    QImage qimg_col_coronal = getMainView( MAIN_VIEW_TYPE::CORONAL, api, true );
    QImage combinedImage = QImage( QSize( SEND_WIDTH, SEND_HEIGHT ), QImage::Format_RGBA8888_Premultiplied );
    QPainter painter( &combinedImage );
    painter.setCompositionMode( QPainter::CompositionMode_Source );
    painter.fillRect( combinedImage.rect(), Qt::black );
    painter.setBrush( QColor( 0, 0, 0, 255 ) );
    painter.drawRect( 0, 0, SEND_WIDTH, SEND_HEIGHT );

    //images don't necessarily have the correct ratio, therefore they need to be resized/scaled/croped:
    QImage sagittal_scaled;
    QImage coronal_scaled;
    QImage transverse_scaled;
    double window_ratio = 1.* qimg_col_coronal.width() / qimg_col_coronal.height();
    if( window_ratio > SEND_ASPECT_RATIO )
    {
        //TODO: on slower system, using other than SmoothTransform might be better
        transverse_scaled = qimg_col_transverse.scaled( SEND_WIDTH/2, SEND_HEIGHT/2, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        sagittal_scaled = qimg_col_sagittal.scaled( SEND_WIDTH/2, SEND_HEIGHT/2, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        coronal_scaled = qimg_col_coronal.scaled( SEND_WIDTH/2, SEND_HEIGHT/2, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        painter.setCompositionMode( QPainter::CompositionMode_SourceOver );
        painter.drawImage( 0, 0, coronal_scaled );
        painter.setCompositionMode( QPainter::CompositionMode_SourceOver );
        painter.drawImage( SEND_WIDTH/2, 0, sagittal_scaled );
        painter.setCompositionMode( QPainter::CompositionMode_SourceOver );
        painter.drawImage( 0, SEND_HEIGHT/2, transverse_scaled );
    }else
    {
        //TODO: this needs to be done (just copy-pasted at the moment)
        transverse_scaled = qimg_col_transverse.scaled( SEND_WIDTH/2, SEND_HEIGHT/2, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        sagittal_scaled = qimg_col_sagittal.scaled( SEND_WIDTH/2, SEND_HEIGHT/2, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        coronal_scaled = qimg_col_coronal.scaled( SEND_WIDTH/2, SEND_HEIGHT/2, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        painter.setCompositionMode( QPainter::CompositionMode_SourceOver );
        painter.drawImage( 0, 0, coronal_scaled );
        painter.setCompositionMode( QPainter::CompositionMode_SourceOver );
        painter.drawImage( SEND_WIDTH/2, 0, sagittal_scaled );
        painter.setCompositionMode( QPainter::CompositionMode_SourceOver );
        painter.drawImage( 0, SEND_HEIGHT/2, transverse_scaled );
    }
    painter.setCompositionMode( QPainter::CompositionMode_SourceOver );
    painter.drawImage( SEND_WIDTH/2, SEND_HEIGHT/2, qimg_col_3d );
    painter.end();
    if ( !reading )
    {
        //TODO: add second buffer
        memcpy( buffer1, combinedImage.bits(), SEND_WIDTH * SEND_HEIGHT * 4 );
        video_to_send = true;
    }else
    {
        std::cerr << "[Sender] SKIPPED A FRAME" << std::endl;
    }
    init_done = true;
    checkTrackingState( api );
}

QImage OpenIGTLSenderPluginInterface::getMainView( MAIN_VIEW_TYPE viewType, IbisAPI * api )
{
    return getMainView(viewType, api, false);
}

QImage OpenIGTLSenderPluginInterface::getMainView( MAIN_VIEW_TYPE viewType, IbisAPI * api, bool quadView )
{
    View * view = nullptr;
    switch(viewType){
    case _3D:
        view = api->GetMain3DView();
        break;
    case SAGITTAL:
        view = api->GetMainSagittalView();
        break;
    case CORONAL:
        view = api->GetMainCoronalView();
        break;
    case TRANSVERSE:
        view = api->GetMainTransverseView();
        break;
    }
    vtkRenderer * renderer = view->GetRenderer( 0 );
    vtkRenderWindow * window = renderer->GetRenderWindow();
    vtkSmartPointer<vtkWindowToImageFilter> windowToImageFilter = vtkSmartPointer<vtkWindowToImageFilter>::New();
    windowToImageFilter->SetInput(window);
    windowToImageFilter->SetInputBufferTypeToRGBA();
    windowToImageFilter->ReadFrontBufferOff();
    windowToImageFilter->Update();
    vtkImageData * img_dat = windowToImageFilter->GetOutput();
    uchar * pixels = (uchar*)(img_dat->GetScalarPointer());
    int * dims = img_dat->GetDimensions();
    window_width = dims[0];
    window_height = dims[1];
    QImage image = QImage( (uchar*)(pixels),
                                           window_width,
                                           window_height,
                                           QImage::Format_RGBA8888);
    if(quadView){
        //TODO: here too, use FastTransform on slower machines?
        image = image.scaled(QSize(SEND_WIDTH*0.5, SEND_HEIGHT*0.5), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
 return image;
}

void OpenIGTLSenderPluginInterface::changeBandwidth( int kbps )
{
    int netWorkBandWidthInBPS = kbps * 1000; //networkBandwidth is in kbps
    int time = floor( 8*RTP_PAYLOAD_LENGTH*1e9/netWorkBandWidthInBPS + 1.0 ); // the needed time in nanosecond to send a RTP payload.
    rtpWrapper->packetIntervalTime = time;
    //set encoder target too:
    H264StreamEncoder->SetRCTaregetBitRate( netWorkBandWidthInBPS );
    std::cout << "[OpenIGTLSenderPluginInterface::changeBandwidth] Set RTPWrapper bandwidth to " << kbps << " kbps." << std::endl;
}


OpenIGTLSenderPluginInterface::~OpenIGTLSenderPluginInterface()
{
    //TODO
}

void OpenIGTLSenderPluginInterface::connectVideo() {
    if( !video_socket_created ){
        connected_video = false;
        int s = videoServerSocket->CreateUDPServer();
        if( s < 0 ){
            std::cerr << "[OpenIGTLSenderPlugin] Could not create a UDP server socket for video." << std::endl;
        }else{
            video_socket_created = true;
        }
    }
    if( video_socket_created && !connected_video ){
        clientID = videoServerSocket->AddClient( client_address.c_str(), port_video, 0 );
        std::cout << "[Sender] added client: " << clientID << std::endl;
        std::cout << "[OpenIGTLSenderPlugin] Created a UDP server socket for video." << std::endl;
        connected_video = true;
    }
    widget->updateui();
}

void OpenIGTLSenderPluginInterface::connectStatus()
{
    if( !status_socket_created ){
        int s = statusServerSocket->CreateServer( port_status );
        if( s < 0 ){
            std::cerr << "[OpenIGTLSenderPlugin] Could not create a TCP server socket for status updates. This typically means that the socket from a previously running instance of IBIS hasn't yet been closed. I will wait a few seconds and try again." << std::endl;
            //5 seconds should do it I presume
            igtl::Sleep(5000);
            return;
        }else{
            status_socket_created = true;
        }
    }
    ConnectThreadId = Threader->SpawnThread( (vtkThreadFunctionType)&(vtkOpenIGTLSenderConnectThread), this );
    widget->updateui();
}

bool OpenIGTLSenderPluginInterface::set_dimensions( int w, int h )
{
    std::cout << "[OpenIGTLSenderPluginInterface] Dimensions of the H264 encoder have been set to:" << w << " x " << h << std::endl;
    H264StreamEncoder->SetPicWidthAndHeight( w, h );
    //simtodo: could thinker with these values and see what works best
    H264StreamEncoder->SetSpeed( HIGH_COMPLEXITY );
    H264StreamEncoder->InitializeEncoder();
    H264StreamEncoder->SetRCMode( IMAGE_SEND_MODE );
    H264StreamEncoder->SetRCTaregetBitRate( TARGET_BIT_RATE );
    encoder = H264StreamEncoder;
    return true;
}

//TODO: this whole function should be cleaned
void OpenIGTLSenderPluginInterface::checkTrackingState( IbisAPI * api)
{
    QString supposed_name_of_device = QString( MARIN_CAMERA_NAME_IN_CONFIG );
    QList<TrackedSceneObject*> all;
    api->GetAllTrackedObjects( all );
    bool allOK = false;
    bool iPadExists = false;
    foreach(TrackedSceneObject* obj, all){
        QString objname = obj->GetName();
        if ( QString::compare( objname, supposed_name_of_device, Qt::CaseInsensitive) == 0 ) {
            if ( obj->GetState() != TrackerToolState::Ok ) { allOK = false; }
            else{ allOK = true; }
            iPadExists = true;
        }
    }
    //if something has changed:
    if ( allOK != trackingOK ){
        status_to_send = true;
        if(allOK)
            std::cout << "[OpenIGTLSenderPlugin][DEBUG]: Camera tracking is back." << std::endl;
        else
            std::cout << "[OpenIGTLSenderPlugin][DEBUG]: Camera tracking lost." << std::endl;
    }
    trackingOK = allOK;
}

bool OpenIGTLSenderPluginInterface::SetClientAddress( QString s )
{
    //todo: sanitize input
    std::cout << "[OpenIGTLSenderPlugin][SetClientAddress]: Changing client address to: " << s.toStdString() << std::endl;
    client_address = s.toStdString();
    connected_video = false;
    videoServerSocket->DeleteClient( clientID );
    return true;
}

void OpenIGTLSenderPluginInterface::OnCommandReceived( igtlioCommand * command ){
    if( command->GetName() == "ToggleQuadView" ){
        bool toggled = false;
        std::string commandContent = command->GetCommandContent();
        QDomDocument qdd  = QDomDocument();
        QByteArray qba = QByteArray( commandContent.c_str(), commandContent.length() );
        if ( commandContent.length() > 0 ){
            bool read_params_success = qdd.setContent(qba);
            //this case would happen if the xml string isn't formated properly
            if (!read_params_success){std::cout << "[OpenIGTLSenderPluginInterface::OnCommandReceived] couldn't read command message parameters." << std::endl;}
            //check for known params:
            QDomNodeList qdnl;
            qdnl = qdd.elementsByTagName(QString("par4"));
            toggled = static_cast<bool>( qdnl.at(0).toElement().text().toInt() );
        }else{
            std::cerr << "[OpenIGTLSenderPluginInterface::OnCommandReceived] Received a 0 length command." << std::endl;
        }
        ToggleQuadView( toggled );
        std::cout << "[OpenIGTLSenderPluginInterface::OnCommandReceived] Toggled quadView to " << toggled << std::endl;
    }
}
