#ifndef __CommandsProcessingWidget_h_
#define __CommandsProcessingWidget_h_

#include <QWidget>

class CommandsProcessingPluginInterface;

namespace Ui
{
    class CommandsProcessingWidget;
}

class CommandsProcessingWidget : public QWidget
{
    Q_OBJECT

public:

    explicit CommandsProcessingWidget( QWidget * parent = nullptr );
    ~CommandsProcessingWidget();

    void SetPluginInterface( CommandsProcessingPluginInterface * interf );
    void updateui();

private slots:

    void UpdateUi();

private:

    CommandsProcessingPluginInterface * m_pluginInterface;
    Ui::CommandsProcessingWidget * ui;

};

#endif
