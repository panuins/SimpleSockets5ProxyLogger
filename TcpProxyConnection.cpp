/*!
 * \file       TcpProxyConnection.cpp
 * \version    1.0
 * \date       2016-06-03, 09:46
 * \author     unknown
 * \copyright  unknown
 *
 * \brief       source file.
 * \details     source file.
 */

#include "TcpProxyConnection.h"

/*!
 * \class TcpProxyConnection
 * \brief 
 * \details 
 */

/*!
 * \fn TcpProxyConnection::TcpProxyConnection(QObject *parent)
 * \brief Default constructor.
 * \param parent pointer to parent object
 * \details Default constructor.
 */
TcpProxyConnection::TcpProxyConnection(QObject *parent) :
    ProxyConnection(parent)
{
}

/*!
 * \fn TcpProxyConnection::TcpProxyConnection()
 * \brief Destructor.
 * \details Destructor.
 */
TcpProxyConnection::~TcpProxyConnection()
{
}

