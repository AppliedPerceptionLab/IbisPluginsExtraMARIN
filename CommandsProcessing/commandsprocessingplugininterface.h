#ifndef __CommandsProcessingPluginInterface_h_
#define __CommandsProcessingPluginInterface_h_

#include <QObject>
#include "toolplugininterface.h"
#include <vtkSmartPointer.h>

class vtkImageData;
class vtkCamera;
class vtkMatrix4x4;
class vtkImageFlip;
class vtkRenderer;
class CameraObject;
class SceneObject;
class PolyDataObject;
class igtlioCommand;

enum CommandName { UNDEFINED, ToggleAnatomy, ToggleQuadView, NavigateSlice, ReregisterAR, RotateView, FreezeFrame, ResetReregistration };

struct Command {    CommandName c ;
                    double param1;
                    double param2;
                    double param3;
                    int param4;
                    int param5;
                    std::string param6;
               };

class CommandsProcessingPluginInterface : public ToolPluginInterface
{

    Q_OBJECT
    Q_INTERFACES(IbisPlugin)
    Q_PLUGIN_METADATA(IID "Ibis.CommandsProcessingPluginInterface" )

public:

    CommandsProcessingPluginInterface();
    ~CommandsProcessingPluginInterface();

    virtual void Serialize( Serializer * serializer );

    virtual QString GetPluginName() { return QString("CommandsProcessing"); }
    virtual QString GetMenuEntryString() { return QString("Commands processing"); }
    virtual bool CanRun() { return true; }

    virtual QWidget * CreateTab();
    virtual bool WidgetAboutToClose();

    void SceneAboutToLoad();
    void SceneFinishedLoading();
    void SceneAboutToSave();
    void SceneFinishedSaving();

signals:

    void Modified();
    void ToggleQuadViewSignal( bool );

public slots:
    void Update();
    void OnCommandReceived( igtlioCommand * command );

protected:

    Command ParseCommand( igtlioCommand * command );
    bool ExecuteCommand( Command c );
    bool ExecuteToggleQuadView( bool b );
    bool ExecuteToggleAnatomy( int anatomy_index, bool toggle_visible );
    bool ExecuteNavigateSlice( int v, double x, double y );
    bool ExecuteReregisterAR( double dx, double dy, double angle );
    bool ExecuteRotateView( double x, double y, double zoom_factor );
    bool ExecuteResetReregistration();

    IbisAPI * api;

private:

    bool backup_done = false;
    vtkMatrix4x4 * backup_reregistration_matrix;
    View * view;
    vtkRenderer * renderer;
    vtkCamera * cam;
    QList<SceneObject*> uobjs;

};

#endif
