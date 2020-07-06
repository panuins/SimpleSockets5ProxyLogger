/*!
 * \file       BindProxyConnection.h
 * \version    1.0
 * \date       2016-06-03, 09:49
 * \author     unknown
 * \copyright  unknown
 *
 * \brief       header file.
 * \details     header file.
 */

#ifndef BINDPROXYCONNECTION_H
#define BINDPROXYCONNECTION_H

#include "ProxyConnection.h"

class BindProxyConnection : public ProxyConnection
{
    Q_OBJECT

public:
    explicit BindProxyConnection(QObject *parent = 0);
    ~BindProxyConnection();


public slots:

signals:

private:
};

#endif // BINDPROXYCONNECTION_H