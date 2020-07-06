/*!
 * \file       UdpProxyConnection.h
 * \version    1.0
 * \date       2016-06-03, 09:49
 * \author     unknown
 * \copyright  unknown
 *
 * \brief       header file.
 * \details     header file.
 */

#ifndef UDPPROXYCONNECTION_H
#define UDPPROXYCONNECTION_H

#include "ProxyConnection.h"

class UdpProxyConnection : public ProxyConnection
{
    Q_OBJECT

public:
    explicit UdpProxyConnection(QObject *parent = 0);
    ~UdpProxyConnection();

    QPointer<QUdpSocket> m_socketToUdpServer;

public slots:

signals:

private:
};

#endif // UDPPROXYCONNECTION_H
