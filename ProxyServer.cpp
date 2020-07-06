#include "ProxyServer.h"
#include "ProxyConnection.h"
#include <QDebug>

inline quint16 getPort(const char *p)
{
    quint16 ret = 0;
    ret += p[0];
    ret *= 256;
    ret += p[1];
    return ret;
}

ProxyServer::ProxyServer(QObject *parent) :
    QTcpServer(parent)
{
}

ProxyServer::~ProxyServer()
{
}

bool ProxyServer::start(const QHostAddress &address, quint16 port)
{
//    QString strServerDir = QString("%1_%2").arg(m_host.toString()).arg(QString::number(m_port));
//    QDir dirServer(QCoreApplication::applicationDirPath());
//    dirServer.cd("log");
//    if (dirServer.exists())
//    bool ret = dirServer.mkdir(strServerDir);
//    if (!ret)
//    {
//        qDebug() << "Server::Server: Error: make dir" << strServerDir << "in " << dirServer << " failed";
//    }
    return listen(address, port);
}

void ProxyServer::deleteConnection(int id)
{
    QPointer<ProxyConnection> c = m_mapConnections.find(id).value();
    if (!c.isNull())
    {
        delete c.data();
        m_mapConnections.remove(id);
    }
}

void ProxyServer::setDir(const QDir &dir)
{
    m_dirServer = dir;
}

void ProxyServer::incomingConnection(qintptr id)
{
    //QByteArray data;
    //QByteArray dataRet;
    ProxyConnection *c = new ProxyConnection(this, id);
    //c->m_server = this;
    //c->m_entrySocketID = id;
//    c->m_socketEntry = new QTcpSocket();
//    c->m_socketEntry->setSocketDescriptor(id);
    m_mapConnections.insert(c->connectionId(), QPointer<ProxyConnection>(c));
    c->processConnection();
    qDebug() << "ProxyServer::incomingConnection: ================new connection";
#if 0
    auto readData = [&] (int readBytes)
    {
        while (data.size() < readBytes)
        {
            bool ok = true;
            if (c->m_socketEntry->bytesAvailable() <= 0)
            {
                ok = c->m_socketEntry->waitForReadyRead(1000);
            }
            if (!ok)
            {
                qDebug() << "ProxyServer::incomingConnection: time out";
                data.append(c->m_socketEntry->readAll());
                qDebug() << "ProxyServer::incomingConnection: readBytes=" << readBytes << " pending data:" << data.toHex();
                delete c;
                return false;
            }
            data.append(c->m_socketEntry->read(readBytes-data.size()));
            //qDebug() << "ProxyServer::incomingConnection: pending data:" << data.toHex();
        }
        return true;
    };
    //read first header
    {
        bool willContinue = false;
        c->m_server = this;
        c->m_socketEntry = new QTcpSocket(c);
        c->m_socketEntry->setSocketDescriptor(id);
        qDebug() << "ProxyServer::incomingConnection: id=" << id
                 << "peerName:" << c->m_socketEntry->peerName()
                 << "local address:" << c->m_socketEntry->localAddress()
                 << "local port:" << c->m_socketEntry->localPort();
        if (!readData(2))
        {
            return;
        }
        if (data.at(0) != 5)
        {
            qDebug() << "ProxyServer::incomingConnection: protocol version (" << (int)data.at(0) << ") is not 5";
            delete c;
            return;
        }
        int i = 0;
        const unsigned char methods = (unsigned char)(data.at(1));
        if (!readData(2+methods))
        {
            return;
        }
        for (i = 0; i < methods; i++)
        {
            const unsigned char method = (unsigned char)(data.at(2+i));
            switch (method)
            {
            case 0:
                if (!willContinue)
                {
                    dataRet.append((char)5);
                    dataRet.append((char)0);
                    c->m_socketEntry->write(dataRet);
                    c->m_socketEntry->flush();
                    dataRet.clear();
                    willContinue = true;
                }
                break;
            case 1:            //GSSAPI
                break;
            case 2:            //USERNAME/PASSWORD
                break;
            default:
                break;
            }
        }
        if (!willContinue)
        {
            qDebug() << "ProxyServer::incomingConnection: this server is only support NO AUTHENTICATION";
            delete c;
            return;
        }
        willContinue = false;
        data.remove(0, 2+methods);
    }

    //read command
    {
        if (!readData(5))
        {
            return;
        }
        if (data.at(0) != 5)
        {
            qDebug() << "ProxyServer::incomingConnection: protocol version (" << (int)data.at(0) << ") is not 5";
            delete c;
            return;
        }
        const unsigned char cmd = (unsigned char)(data.at(1));
        const unsigned char hostType = (unsigned char)(data.at(3));
        data.remove(0, 4);
//        QHostAddress host;
//        quint16 port = 0;
        QByteArray hostData;
        hostData.append((char)hostType);
        switch (hostType)
        {
        case 1:            //IP V4 address
            qDebug() << "ProxyServer::incomingConnection: IP V4 address";
            if (!readData(6))
            {
                return;
            }
            else
            {
                hostData.append(data.mid(0, 6));
                quint32 ip = 0;
                memcpy(&ip, data.constData(), 4);
                c->m_portExit = getPort(data.constData()+4);
                c->m_hostExit.setAddress(ip);
            }
            break;
        case 3:            //DOMAINNAME
            qDebug() << "ProxyServer::incomingConnection: DOMAINNAME";
            if (!readData(1))
            {
                return;
            }
            else
            {
                const unsigned char size = (unsigned char)(data.at(0));
                if (!readData(3+size))
                {
                    return;
                }
                QByteArray hostName = data.mid(1, size);
                QByteArray portData = data.mid(1+size, 2);
                hostName.append((char)0);
                //QByteArray portData = data.mid(5+size, 2);
                qDebug() << "hostName:" << QString::fromUtf8(hostName) << "host data:" << data.toHex();
                bool ok = c->m_hostExit.setAddress(QString::fromUtf8(hostName));
                if (!ok)
                {
                    c->m_strhostExit = QString::fromUtf8(hostName);
                }
                c->m_portExit = getPort(portData.constData());
                hostData.append(data.mid(0, 3+size));
            }
            break;
        case 4:            //IP V6 address
        {
            qDebug() << "ProxyServer::incomingConnection: IP V6 address";
            quint8 ip[16];
            if (!readData(18))
            {
                return;
            }
            memcpy(&ip, data.constData(), 16);
            c->m_portExit = getPort(data.constData()+16);
            c->m_hostExit.setAddress(ip);
            hostData.append(data.mid(0, 18));
            break;
        }
        default:
            qDebug() << "ProxyServer::incomingConnection: unknown command " << (int)cmd;
            delete c;
            return;
        }
        /*qDebug() << "ProxyServer::incomingConnection: exit host:" << c->m_hostExit
                 << "exit port:" << c->m_portExit;*/
        connect(c->m_socketEntry.data(), SIGNAL(readyRead()),
                c, SLOT(on_Entry_readyRead()));
        QByteArray cmdReplyData;
        cmdReplyData.append((char)5);
        cmdReplyData.append((char)0);
        cmdReplyData.append((char)0);
        cmdReplyData.append(hostData);
//        {
//            QString serverHost;
//            if ((c->m_hostExit.isNull()) || (!c->m_strhostExit.isEmpty()))
//            {
//                serverHost = c->m_strhostExit;
//            }
//            else
//            {
//                serverHost = c->m_hostExit.toString();
//            }
//            QString strServerDir = QString("%1_%2").arg(serverHost.toString()).arg(QString::number(c->m_portExit));
//            strServerDir.replace(QChar(':'), QChar('_'));
//            QDir dirServer(QCoreApplication::applicationDirPath());
//            dirServer.cd("log");
//            bool ret = dirServer.cd(strServerDir);
//            if (!ret)
//            {
//                ret = dirServer.mkdir(strServerDir);
//                if (!ret)
//                {
//                    qDebug() << "TcpSocket::TcpSocket: Error: make dir" << strServerDir << "in " << dirServer << " failed.";
//                }
//                ret = dirServer.cd(strServerDir);
//                if (!ret)
//                {
//                    qDebug() << "TcpSocket::TcpSocket: Error: cd to" << strServerDir << "in " << dirServer << " failed.";
//                }
//            }
//            m_dirServer = dirServer;
//        }
        switch (cmd)
        {
        case 1:            //CONNECT
        {
            c->m_socketExit_Tcp = new QTcpSocket(c);
            connect(c->m_socketExit_Tcp.data(), SIGNAL(readyRead()),
                    c, SLOT(on_TcpExit_readyRead()));
            if ((c->m_hostExit.isNull()) || (!c->m_strhostExit.isEmpty()))
            {
                qDebug() << "ProxyServer::incomingConnection: connect to" << c->m_strhostExit << ":" << c->m_portExit;
                c->m_socketExit_Tcp->connectToHost(c->m_strhostExit, c->m_portExit);
            }
            else
            {
                qDebug() << "ProxyServer::incomingConnection: connect to" << c->m_hostExit << ":" << c->m_portExit;
                c->m_socketExit_Tcp->connectToHost(c->m_hostExit, c->m_portExit);
            }
            bool ok = c->m_socketExit_Tcp->waitForConnected();
            if (ok)
            {
                c->m_socketEntry->write(cmdReplyData);
            }
            else
            {
                QAbstractSocket::SocketError err = c->m_socketExit_Tcp->error();
                qDebug() << "ProxyServer::incomingConnection: error string:" << c->m_socketExit_Tcp->errorString();
                switch (err) {
                case QAbstractSocket::ConnectionRefusedError:
                    cmdReplyData[1] = 5;
                    break;
                case QAbstractSocket::HostNotFoundError:
                    cmdReplyData[1] = 4;
                    break;
                case QAbstractSocket::NetworkError:
                    cmdReplyData[1] = 3;
                    break;
                default:
                    cmdReplyData[1] = 1;
                }
                c->m_socketEntry->write(cmdReplyData);
            }
            break;
        }
        case 2:            //BIND
        {
            if ((c->m_hostExit.isNull()) || (!c->m_strhostExit.isEmpty()))
            {
                qDebug() << "ProxyServer::incomingConnection: bind to" << c->m_strhostExit << ":" << c->m_portExit;
            }
            else
            {
                qDebug() << "ProxyServer::incomingConnection: bind to" << c->m_hostExit << ":" << c->m_portExit;
            }
            //qDebug() << "ProxyServer::incomingConnection: unsupported command bind.";
//            c->m_socketExit_Tcp = new QTcpSocket(c);
//            connect(c->m_socketExit_Tcp.data(), SIGNAL(readyRead()),
//                    c, SLOT(on_TcpExit_readyRead()));
//            c->m_socketExit_Tcp->connectToHost(c->m_hostExit, c->m_portExit);
//            bool ok = c->m_socketExit_Tcp->waitForConnected();
//            if (ok)
//            {
//                c->m_socketEntry->write(cmdReplyData);
//            }
//            else
//            {
//                QAbstractSocket::SocketError err = c->m_socketExit_Tcp->error();
//                qDebug() << "ProxyServer::incomingConnection: error string:" << c->m_socketExit_Tcp->errorString();
//                switch (err) {
//                case QAbstractSocket::ConnectionRefusedError:
//                    cmdReplyData[1] = 5;
//                    break;
//                case QAbstractSocket::HostNotFoundError:
//                    cmdReplyData[1] = 4;
//                    break;
//                case QAbstractSocket::NetworkError:
//                    cmdReplyData[1] = 3;
//                    break;
//                default:
//                    cmdReplyData[1] = 1;
//                }
//                c->m_socketEntry->write(cmdReplyData);
//            }
            cmdReplyData[1] = 7;
            c->m_socketEntry->write(cmdReplyData);
            delete c;
            return;
            break;
        }
        case 3:            //UDP ASSOCIATE
            qDebug() << "ProxyServer::incomingConnection: UDP ASSOCIATE to" << c->m_hostExit << ":" << c->m_portExit;
            c->m_socketExit_Udp = new QUdpSocket(c);
            connect(c->m_socketExit_Udp.data(), SIGNAL(readyRead()),
                    c, SLOT(on_UdpExit_readyRead()));
            if ((c->m_hostExit.isNull()) || (!c->m_strhostExit.isEmpty()))
            {
                qDebug() << "ProxyServer::incomingConnection: UDP ASSOCIATE to" << c->m_strhostExit << ":" << c->m_portExit;
                //c->m_socketExit_Udp->bind(c->m_strhostExit, c->m_portExit);
            }
            else
            {
                qDebug() << "ProxyServer::incomingConnection: UDP ASSOCIATE to" << c->m_hostExit << ":" << c->m_portExit;
                c->m_socketExit_Udp->bind(c->m_hostExit, c->m_portExit);
            }
            c->m_socketEntry->write(cmdReplyData);
            break;
        default:
            qDebug() << "ProxyServer::incomingConnection: unknown command " << (int)cmd;
            delete c;
            return;
        }
    }
    m_mapConnections.insert(c->id, QPointer<ProxyConnection>(c));
#endif
    //c->start();
}
