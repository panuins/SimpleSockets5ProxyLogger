#include "widget.h"
#include "ui_widget.h"
#include "ProxyServer.h"
#include <QDir>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    m_server(Q_NULLPTR)
{
    ui->setupUi(this);
    m_server = new ProxyServer();
    m_originWindowTitle = windowTitle();
    QDir dirServer(QCoreApplication::applicationDirPath());
    dirServer.mkdir("log");
    dirServer.cd("log");
    ui->lineEdit_logDir->setText(dirServer.absolutePath());
    ui->lineEdit_logDir->hide();
    ui->label_logDir->hide();
    ui->toolButton_logDir->hide();
    ui->checkBox_logToFile->hide();
}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_pushButton_clicked()
{
    if (m_server->isListening())
    {
        m_server->close();
        this->setWindowTitle(
                    m_originWindowTitle
                    +tr(" stopped"));
        ui->pushButton->setText(tr("start"));
    }
    else
    {
        m_server->start(QHostAddress::Any, quint16(ui->spinBox_port->value()));
        this->setWindowTitle(
                    m_originWindowTitle
                    +tr(" port:%2")
                    .arg(m_server->serverPort()));
        ui->pushButton->setText(tr("stop"));
    }
}
