#include "commandsprocessingplugininterface.h"
#include "commandsprocessingwidget.h"
#include <QtPlugin>
#include <QtGui>
#include "view.h"
#include "scenemanager.h"
#include "application.h"
#include "polydataobject.h"
#include "pointerobject.h"
#include "imageobject.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkSphereWidget.h"
#include "vtkProperty.h"
#include "vtkMatrix4x4.h"
#include "serializerhelper.h"
#include "vtkTransform.h"
#include "vtkBoxRepresentation.h"
#include "vtkBoxWidget2.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkCoordinate.h"

CommandsProcessingPluginInterface::CommandsProcessingPluginInterface()
{

}

CommandsProcessingPluginInterface::~CommandsProcessingPluginInterface()
{

}

void CommandsProcessingPluginInterface::Serialize( Serializer * serializer )
{

}

#include <ibisapi.h>
QWidget * CommandsProcessingPluginInterface::CreateTab()
{
    CommandsProcessingWidget * widget = new CommandsProcessingWidget;
    widget->SetPluginInterface( this );

    //init vars:
    api = this->GetIbisAPI();
    view = api->GetMain3DView();
    renderer = view->GetRenderer();
    cam = renderer->GetActiveCamera();
    uobjs = QList<SceneObject*>();
    api->GetAllUserObjects(uobjs);
    // Get callback when tracking updated
    connect( api, SIGNAL( IbisClockTick() ), this, SLOT( Update() ) );
    connect( api, SIGNAL( NewCommandReceived( igtlioCommand * ) ), this, SLOT( OnCommandReceived( igtlioCommand * ) ) );
    return widget;
}

bool CommandsProcessingPluginInterface::WidgetAboutToClose()
{
    // Remove update callback
    disconnect( api, SIGNAL(IbisClockTick()), this, SLOT(Update()) );
    disconnect( api, SIGNAL( NewCommandReceived( igtlioCommand * ) ), this, SLOT( OnCommandReceived( igtlioCommand * ) ) );

    return true;
}

void CommandsProcessingPluginInterface::SceneAboutToLoad()
{
    //TODO
}

void CommandsProcessingPluginInterface::SceneFinishedLoading()
{
    //TODO
}

void CommandsProcessingPluginInterface::SceneAboutToSave()
{
    //TODO
}

void CommandsProcessingPluginInterface::SceneFinishedSaving()
{
    //TODO
}

void CommandsProcessingPluginInterface::Update()
{
    //TODO
}

void CommandsProcessingPluginInterface::OnCommandReceived( igtlioCommand * command )
{
    Command c = ParseCommand( command );
    ExecuteCommand( c );
}

#include <igtlioCommand.h>
Command CommandsProcessingPluginInterface::ParseCommand( igtlioCommand * command )
{
    Command c = { UNDEFINED, -99., -99., -99., -99, -99, "" };
    std::string commandType = command->GetName();
    if( commandType == "ToggleAnatomy" ){
        c.c = ToggleAnatomy;
    }else if( commandType == "ToggleQuadView" ){
        c.c = ToggleQuadView;
    }else if( commandType == "NavigateSlice" ){
        c.c = NavigateSlice;
    }else if( commandType == "RotateView" ){
        c.c = RotateView;
    }else if( commandType == "ReregisterAR" ){
        c.c = ReregisterAR;
    }else if( commandType == "FreezeFrame" ){
        c.c = FreezeFrame;
    }else if( commandType == "ResetReregistration" ){
        c.c = ResetReregistration;
    }else{
        std::cerr << "[IbisHardwareIGSIO::OnCommandReceived] received an unknown command" << std::endl;
    }
    std::string commandContent = command->GetCommandContent();
    QDomDocument qdd  = QDomDocument();
    QByteArray qba = QByteArray( commandContent.c_str(), commandContent.length() );
    if( commandContent.length() > 0 ){
        QDomDocument::ParseResult parseResult = qdd.setContent( qba );
        //this case would happen if the xml string isn't formated properly
        if ( !parseResult ){ std::cout << "[Receiver] couldn't read command message parameters." << std::endl; }
        //check for known params:
        QDomNodeList qdnl;
        qdnl = qdd.elementsByTagName(QString("par1"));
        c.param1 = qdnl.at(0).toElement().text().toDouble();
        qdnl = qdd.elementsByTagName(QString("par2"));
        c.param2 = qdnl.at(0).toElement().text().toDouble();
        qdnl = qdd.elementsByTagName(QString("par3"));
        c.param3 = qdnl.at(0).toElement().text().toDouble();
        qdnl = qdd.elementsByTagName(QString("par4"));
        c.param4 = qdnl.at(0).toElement().text().toInt();
        qdnl = qdd.elementsByTagName(QString("par5"));
        c.param5 = qdnl.at(0).toElement().text().toInt();
        qdnl = qdd.elementsByTagName(QString("par6"));
        c.param6 = qdnl.at(0).toElement().text().toStdString();
    }else{
        std::cerr << "[IbisHardwareIGSIO::OnCommandReceived][BUG?] Received a 0 length command." << std::endl;
    }
    return c;
}

#define EPSILON 0.0000001
bool CommandsProcessingPluginInterface::ExecuteCommand( Command com )
{
    //Execute commands:
    switch ( com.c ) {
        case ToggleAnatomy: {
            std::cout << "[CommandsProcessingPluginInterface::ExecuteCommand] Toggling anatomy: " << com.param4 << " to " << static_cast<bool>(com.param5) << std::endl;
            return ExecuteToggleAnatomy( com.param4, static_cast<bool>(com.param5) );
        }
        case ToggleQuadView: {
            std::cout << "[CommandsProcessingPluginInterface::ExecuteCommand] Toggling QuadView should now be handled in OIGTLSender plugin. Skipping. " << std::endl;
            return false;
        }
        case NavigateSlice: {
            std::cout << "[CommandsProcessingPluginInterface::ExecuteCommand] Navigate slice (view " << com.param4 << "): " << com.param1 << ", " <<  com.param2 << std::endl;
            return ExecuteNavigateSlice( com.param4, com.param1, com.param2 );
        }
        case RotateView: {
            std::cout << "[CommandsProcessingPluginInterface::ExecuteCommand] Rotating view: ( " << com.param1 << ", " << com.param2 << " )" << std::endl;
            //Special case, nothing to do:
            if( fabs(com.param1) < EPSILON && fabs(com.param2) < EPSILON && fabs(com.param3) < EPSILON ){
                std::cout << "[CommandsProcessingPluginInterface::ExecuteCommand] There is nothing to rotate here." << std::endl;
                return true;
            }
            return ExecuteRotateView( com.param1, com.param2, com.param3 );
        }
        case ReregisterAR: {
            std::cout << "[CommandsProcessingPluginInterface::ExecuteCommand] Reregistering AR: ( " << com.param1 << ", " << com.param2 << " ) " << com.param3 << " degrees." << std::endl;
            //Special case, nothing to do:
            if( fabs( com.param1 ) < EPSILON && fabs( com.param2 ) < EPSILON && fabs( com.param3 ) < EPSILON ){
                std::cout << "[CommandsProcessingPluginInterface::ExecuteCommand] There is nothing to reregister here." << std::endl;
                return true;
            }
            return ExecuteReregisterAR( com.param1, com.param2, com.param3 );
        }
        case FreezeFrame: {
            std::cout << "[CommandsProcessingPluginInterface::ExecuteCommand] Freeze frame: " << com.param4 << std::endl;
            //TODO: not implemented
            return true;
        }
        case ResetReregistration: {
            std::cout << "[CommandsProcessingPluginInterface::ExecuteCommand] Reset Reregistration." << std::endl;
            return ExecuteResetReregistration();
        }
        default: {
            break;
        }
    }
    return true;
}

bool CommandsProcessingPluginInterface::ExecuteToggleAnatomy( int n, bool b )
{
    QList<PolyDataObject*> allPolyDataObjects = QList<PolyDataObject*>();
    api->GetAllPolyDataObjects( allPolyDataObjects );
    QList<SceneObject*> landmarks = QList<SceneObject*>();
    api->GetAllObjectsOfType( "LandmarkRegistrationObject", landmarks );
    //Look for Landmark Registration Object:
    //We are usually only interested on toggling objects in there, since objects outside of it are probably not anatomy (might be world axes, calibration grids and such)
    SceneObject * landmark;
    if( landmarks.size() < 1 ){
        std::cerr << "[CommandsProcessingPluginInterface::ExecuteToggleAnatomy] Anatomy to toggle should be placed inside a landmark registration object. Aborting command execution." << std::endl;
        return false;
    }else{
        landmark = landmarks.at(0);
    }
    QList<PolyDataObject*> polyDataObjectsInsideLandmark = QList<PolyDataObject*>();
    //construct the list:
    for ( int i = 0 ; i < allPolyDataObjects.length() ; i++ ){
        if( allPolyDataObjects.at(i)->DescendsFrom( landmark ) ){
            polyDataObjectsInsideLandmark.push_back( allPolyDataObjects.at(i) );
        }
    }
    //QList indexing is 0-based:
    n = n-1;
    //TODO: it would make much more sense for the command anatomy indexing to be 0-based as well
    if( n > polyDataObjectsInsideLandmark.size()-1 ){
        std::cerr << "[CommandsProcessingPluginInterface::ExecuteToggleAnatomy] Cannot toggle anatomy " << n << ", LandmarkRegistrationObject has only " << polyDataObjectsInsideLandmark.size() << " PolyData children." << std::endl;
        return false;
    }
    PolyDataObject * pdo = polyDataObjectsInsideLandmark.at( n );
    ((SceneObject*)pdo)->SetHidden( !b );
    return true;
}

#include <vtkImageData.h>
bool CommandsProcessingPluginInterface::ExecuteNavigateSlice( int v, double x, double y )
{
    //Update scene vars:
    ImageObject * imageObj = api->GetReferenceDataObject();
    if( imageObj == nullptr ){
        std::cerr << "[CommandsProcessingPluginInterface::ExecuteNavigateSlice] No slice to navigate. Aborting command execution." << std::endl;
        return false;
    }
    vtkImageData * image = imageObj->GetImage();
    double origin[3];
    image->GetOrigin(origin);
    double spacing[3];
    image->GetSpacing(spacing);
    int extent[6];
    image->GetExtent(extent);
    int range[3];
    range[0] = extent[1] - extent[0] + 1;
    range[1] = extent[3] - extent[2] + 1;
    range[2] = extent[5] - extent[4] + 1;
    double * position = new double[3];
    api->GetCursorPosition( position );
    if( v == 0 ){
        position[0] = ( origin[0] + spacing[0] * x * range[0] );
        position[1] = -( origin[1] + spacing[1] * y * range[1] );
    }else if( v == 1 ){
        position[0] = ( origin[0] + spacing[0] * x * range[0] );
        position[2] = -( origin[2] + spacing[2] * y * range[2] );
    }else if( v == 2 ){
        position[1] = -( origin[1] + spacing[1] * x * range[1] );
        position[2] = -( origin[2] + spacing[2] * y * range[2] );
    }else{
        std::cerr << "[CommandsProcessingPluginInterface::ExecuteNavigateSlice] Navigated slice doesn't exist. Aborting command execution." << std::endl;
    }
    api->SetCursorPosition( position );
    return true;
}

bool CommandsProcessingPluginInterface::ExecuteReregisterAR( double x, double y, double angle )
{
    view = api->GetMain3DView();
    renderer = view->GetRenderer();
    cam = renderer->GetActiveCamera();
    uobjs = QList<SceneObject*>();
    api->GetAllUserObjects(uobjs);
    //Look for Landmark Registration Object:
    SceneObject * reregistrationTransform = new SceneObject();
    bool reregistrationTransformFound = false;
    for ( int i = 0 ; i < uobjs.length() ; i++ ){
        QString s = uobjs.at(i)->GetName();
        int x = QString::compare( s, QString("Landmark Registration"), Qt::CaseInsensitive);
        if ( x == 0 ){
            //try to set the reregistrsation transform object:
            //(by convention: it should be the next object after the Landmark, it should be called "Transform" and have the Landmark as parent)
            if( i+1 < uobjs.length() &&
                    !QString::compare( uobjs.at(i+1)->GetName(), QString("Transform"), Qt::CaseInsensitive) &&
                    uobjs.at(i+1)->GetParent() == uobjs.at(i)
                    ){
                reregistrationTransform = uobjs.at(i+1);
                reregistrationTransformFound = true;
            }
            break;
        }
    }
    if( uobjs.length() < 2 ){
        std::cout << "[CommandsProcessingPluginInterface::ExecuteReregisterAR] Scene empty, nothing to reregister. Aborting command execution." << std::endl << std::flush;
        return true;
    }
    if( !reregistrationTransformFound ){
        std::cout << "[CommandsProcessingPluginInterface::ExecuteReregisterAR] Registration transform not found. The scene isn't properly constructed. (There must be a transform object called \"Transform\" in the Landmark Registration object.) Aborting command execution." << std::endl << std::flush;
        return false;
    }
    //backup the transform (for reset purposes):
    if (!backup_done){
        backup_reregistration_matrix = vtkMatrix4x4::New();
        backup_reregistration_matrix->DeepCopy( reregistrationTransform->GetLocalTransform()->GetMatrix() );
        backup_done = true;
    }
    //needed because of how gestures are handled on iPad:
    if( angle > 180. ){
        angle -= 360.;
    }if( angle < -180. ){
        angle += 360.;
    }
    //first child of Landmark Registration object should be the transform:
    SceneObject * object = reregistrationTransform;
    vtkTransform * worldTransform = object->GetWorldTransform();
    vtkSmartPointer<vtkMatrix4x4> inverseWorld = vtkSmartPointer<vtkMatrix4x4>::New();
    worldTransform->GetInverse( inverseWorld );
    vtkTransform * localTransform = object->GetLocalTransform();
    vtkTransform * camWorld = cam->GetModelViewTransformObject();
    vtkSmartPointer<vtkMatrix4x4> inverseCam = vtkSmartPointer<vtkMatrix4x4>::New();
    camWorld->GetInverse( inverseCam );
    //hardcoded scale factor to reduce sensitivity:
    //TODO: this should be user specifiable
    double factor = 0.2;
    x = factor*x;
    y = factor*y;
    angle = factor*angle;
    vtkSmartPointer<vtkTransform> translation = vtkSmartPointer<vtkTransform>::New();
    translation->Translate( x, -y, 0.0 );
    vtkSmartPointer<vtkTransform> rotation = vtkSmartPointer<vtkTransform>::New();
    rotation->RotateWXYZ( -angle, 0., 0., 1. );
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->PreMultiply();
    transform->Concatenate( inverseWorld );
    transform->Concatenate( inverseCam );
    if ( fabs(x) > 0 || fabs(y) > 0 ){
        transform->Concatenate( translation );
    }
    if ( fabs(angle) > 0 ){
        //simtodo: the rotation is done around the center of the objects' coordinate system, which might be anything.
        //Something better would be to rotate around the center of mass of the object perhaps?
        transform->Concatenate( rotation );
    }
    transform->Concatenate( camWorld );
    transform->Concatenate( worldTransform );
    //apply the transform:
    vtkSmartPointer<vtkTransform> transformFinal = vtkSmartPointer<vtkTransform>::New();
    transformFinal->PreMultiply();
    transformFinal->Identity();
    transformFinal->Concatenate( localTransform );
    transformFinal->Concatenate( transform );
    vtkMatrix4x4 * mat = object->GetLocalTransform()->GetMatrix();
    mat->DeepCopy( transformFinal->GetMatrix() );
    object->GetLocalTransform()->Modified();
    return true;
}

bool CommandsProcessingPluginInterface::ExecuteRotateView( double x, double y, double zoom_factor )
{
    view = api->GetMain3DView();
    cam = renderer->GetActiveCamera();
    double dx = x;
    double dy = y;
    //TODO: hardcoded values, should be user specifiable
    int size[2] = { 1024, 768 };
    double delta_elevation = -20.0 / size[1];
    double delta_azimuth = -20.0 / size[0];
    double MotionFactor = 8.;
    double rxf = dx * delta_azimuth * MotionFactor;
    double ryf = dy * delta_elevation * MotionFactor;
    //VTK coordinate system is flipped:
    cam->Azimuth(rxf);
    cam->Elevation(-ryf);
    cam->OrthogonalizeViewUp();
    //TODO: this doesn't work very well.
//    if( zoom_factor > 0 + EPSILON ){
//        cam->Zoom( zoom_factor );
//    }
    view->NotifyNeedRender();
    return true;
}

bool CommandsProcessingPluginInterface::ExecuteResetReregistration()
{
    uobjs = QList<SceneObject*>();
    api->GetAllUserObjects(uobjs);
    SceneObject * reregistrationTransform = new SceneObject();
    bool reregistrationTransformFound = false;
    for ( int i = 0 ; i < uobjs.length() ; i++ ){
        QString s = uobjs.at(i)->GetName();
        int x = QString::compare( s, QString("Landmark Registration"), Qt::CaseInsensitive);
        if ( x == 0 ){
            //Set the reregistrsation transform:
            //(it should usually be the next object after the Landmark, it should be called "Transform" and have the Landmark as parent)
            if( i+1 <   uobjs.length() &&
                        !QString::compare( uobjs.at(i+1)->GetName(), QString("Transform"), Qt::CaseInsensitive) &&
                        uobjs.at(i+1)->GetParent() == uobjs.at(i) ){
                reregistrationTransform = uobjs.at(i+1);
                reregistrationTransformFound = true;
            }
            break;
        }
    }
    vtkMatrix4x4 * mat = reregistrationTransform->GetLocalTransform()->GetMatrix();
    mat->DeepCopy( backup_reregistration_matrix );
    reregistrationTransform->GetLocalTransform()->Modified();
    return true;
}
