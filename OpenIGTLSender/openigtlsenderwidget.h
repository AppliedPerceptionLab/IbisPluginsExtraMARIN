#ifndef __OpenIGTLSenderWidget_h_
#define __OpenIGTLSenderWidget_h_

#include <QWidget>

class OpenIGTLSenderPluginInterface;

namespace Ui
{
    class OpenIGTLSenderWidget;
}


class OpenIGTLSenderWidget : public QWidget
{

    Q_OBJECT

public:

    explicit OpenIGTLSenderWidget( QWidget *parent = nullptr );
    ~OpenIGTLSenderWidget();

    void SetPluginInterface( OpenIGTLSenderPluginInterface * pi );
    int getBandwidth();
    void updateui();

private slots:

    void UpdateUi();
    void on_activate_clicked();
    void on_stop_clicked();
    void on_BandwidthSpinBox_valueChanged( int kbps );
    void on_offScreen_stateChanged( int state );
    void on_quadView_stateChanged( int state );

private:

    OpenIGTLSenderPluginInterface * m_pluginInterface;
    Ui::OpenIGTLSenderWidget * ui;

};

#endif
