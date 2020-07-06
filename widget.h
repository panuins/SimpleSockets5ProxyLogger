#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

namespace Ui {
class Widget;
}

class ProxyServer;
class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

private slots:
    void on_pushButton_clicked();

private:
    QString m_originWindowTitle;
    Ui::Widget *ui;
    ProxyServer *m_server;
};

#endif // WIDGET_H
