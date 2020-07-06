/*!
 * \file       TcpProxyConnection.h
 * \version    1.0
 * \date       2016-06-03, 09:46
 * \author     unknown
 * \copyright  unknown
 *
 * \brief       header file.
 * \details     header file.
 */

#ifndef TCPPROXYCONNECTION_H
#define TCPPROXYCONNECTION_H

#include "ProxyConnection.h"

class TcpProxyConnection : public ProxyConnection
{
    Q_OBJECT

public:
    explicit TcpProxyConnection(QObject *parent = 0);
    ~TcpProxyConnection();

    QPointer<QTcpSocket> m_socketToTcpServer;

public slots:

signals:

private:
};

#endif // TCPPROXYCONNECTION_H
