#ifndef PROXYSERVER_H
#define PROXYSERVER_H

#include <QTcpServer>
#include <QPointer>
#include <QMap>
#include <QDir>

class ProxyConnection;

class ProxyServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit ProxyServer(QObject *parent = 0);
    ~ProxyServer();

    bool start(const QHostAddress & address = QHostAddress::Any, quint16 port = 0);
    void deleteConnection(int id);

    void setDir(const QDir &dir);

protected:
    void incomingConnection(qintptr id);

private:
    QMap<int, QPointer<ProxyConnection> > m_mapConnections;
    QDir m_dirServer;
};

#endif // PROXYSERVER_H
