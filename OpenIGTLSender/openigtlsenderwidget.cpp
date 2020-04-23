#include "openigtlsenderwidget.h"
#include "ui_openigtlsenderwidget.h"
#include "openigtlsenderplugininterface.h"

OpenIGTLSenderWidget::OpenIGTLSenderWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OpenIGTLSenderWidget)
{
    ui->setupUi(this);
}

OpenIGTLSenderWidget::~OpenIGTLSenderWidget()
{
    delete ui;
}

void OpenIGTLSenderWidget::SetPluginInterface( OpenIGTLSenderPluginInterface * pi )
{
    m_pluginInterface = pi;
    connect( m_pluginInterface, SIGNAL(Modified()), this, SLOT(UpdateUi()) );
    UpdateUi();
}

void OpenIGTLSenderWidget::updateui(){
    UpdateUi();
}

void OpenIGTLSenderWidget::UpdateUi()
{
    //TODO: This should all be cleaned.
    Q_ASSERT( m_pluginInterface );
    //connected video:
    bool cv = m_pluginInterface->GetConnectedVideo();
    QString scv = "Connected video: " + QString::number(cv);
    ui->connected_video->setText( scv );

    //connected commands:
    bool cc = m_pluginInterface->GetConnectedCommands();
    QString scc = "Connected commands: " + QString::number(cc);
    ui->connected_commands->setText( scc );

    //connecting commands:
    bool ccc = m_pluginInterface->GetConnectingCommands();
    QString sccc = "Connecting commands: " + QString::number(ccc);
    ui->connecting_commands->setText( sccc );

    //socket created commands:
    bool bscc = m_pluginInterface->GetSocketCreatedCommands();
    QString sscc = "Socket created commands: " + QString::number(bscc);
    ui->socket_created_commands->setText( sscc );
}

void OpenIGTLSenderWidget::on_activate_clicked(){
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->activate();
    UpdateUi();
}

void OpenIGTLSenderWidget::on_stop_clicked(){
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->deactivate();
    UpdateUi();
}

void OpenIGTLSenderWidget::on_BandwidthSpinBox_valueChanged( int kbps ){
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->changeBandwidth( kbps );
    UpdateUi();
}

void OpenIGTLSenderWidget::on_offScreen_stateChanged( int state ){
    Q_ASSERT( m_pluginInterface );
    m_pluginInterface->setOffScreen( state );
    UpdateUi();
}

void OpenIGTLSenderWidget::on_quadView_stateChanged( int state ){
    Q_ASSERT( m_pluginInterface );
    std::cout << "State is:" << state << std::endl;;
    m_pluginInterface->ToggleQuadView( (bool)state );
    UpdateUi();
}

int OpenIGTLSenderWidget::getBandwidth(){
    return ui->BandwidthSpinBox->value();
}
