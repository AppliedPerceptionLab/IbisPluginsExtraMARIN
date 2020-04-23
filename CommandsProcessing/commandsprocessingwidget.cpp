#include "commandsprocessingwidget.h"
#include "ui_commandsprocessingwidget.h"
#include "commandsprocessingplugininterface.h"


CommandsProcessingWidget::CommandsProcessingWidget( QWidget * parent )
    : QWidget(parent), ui( new Ui::CommandsProcessingWidget )
{
    ui->setupUi(this);
}

CommandsProcessingWidget::~CommandsProcessingWidget()
{
    delete ui;
}

void CommandsProcessingWidget::SetPluginInterface( CommandsProcessingPluginInterface * interf )
{
    m_pluginInterface = interf;
    connect( m_pluginInterface, SIGNAL(Modified()), this, SLOT(UpdateUi()) );
    UpdateUi();
}

void CommandsProcessingWidget::updateui(){
    UpdateUi();
}

void CommandsProcessingWidget::UpdateUi()
{
    Q_ASSERT( m_pluginInterface );
    //TODO
}
