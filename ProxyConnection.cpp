#include "ProxyConnection.h"
#include <QApplication>
#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QDir>

int ProxyConnection::m_nextId=0;

inline quint16 getPort(const char *p)
{
    quint16 ret = 0;
    quint8 *pIn = (quint8 *)(p);
    ret += pIn[0];
    ret *= 256;
    ret += pIn[1];
    return ret;
}

inline quint32 getIpv4(const char *p)
{
    quint32 ret = 0;
    quint8 *pIn = (quint8 *)(p);
    ret += pIn[0];
    ret *= 256;
    ret += pIn[1];
    ret *= 256;
    ret += pIn[2];
    ret *= 256;
    ret += pIn[3];
    return ret;
}

inline void getIpv6(const char *in, quint8 *out)
{
    int i = 0;
    quint8 *pIn = (quint8 *)(in);
    for (; i < 16; i++)
    {
        out[16-i] = pIn[i];
    }
}

ProxyConnection::ProxyConnection(QTcpServer *server,
                                 qintptr entrySocketID,
                                 QObject *parent) :
    QThread(parent),
    m_portExit(0),
    m_entrySocketID(entrySocketID),
    m_id(m_nextId)
{
    m_server = server;
    m_nextId++;
}

ProxyConnection::~ProxyConnection()
{
    if (!m_socketEntry.isNull())
    {
        m_socketEntry->close();
        //delete m_socketEntry.data();
    }
    if (!m_socketExit_Tcp.isNull())
    {
        m_socketExit_Tcp->close();
        //delete m_socketExit_Tcp.data();
    }
    if (!m_socketExit_Udp.isNull())
    {
        m_socketExit_Udp->close();
        //delete m_socketExit_Udp.data();
    }
    if (m_file_received.isOpen())
    {
        m_file_received.close();
    }
    if (m_file_send.isOpen())
    {
        m_file_send.close();
    }
}

void ProxyConnection::processConnection()
{
    QByteArray data;
    QByteArray dataRet;
    auto readData = [&] (int readBytes)
    {
        while (data.size() < readBytes)
        {
            bool ok = true;
            if (m_socketEntry->bytesAvailable() <= 0)
            {
                ok = m_socketEntry->waitForReadyRead(1000);
            }
            if (!ok)
            {
                qDebug() << "ProxyConnection::processConnection: time out";
                data.append(m_socketEntry->readAll());
                qDebug() << "ProxyConnection::processConnection: readBytes=" << readBytes << " pending data:" << data.toHex();
                //delete c;
                this->deleteLater();
                return false;
            }
            data.append(m_socketEntry->read(readBytes-data.size()));
            //qDebug() << "ProxyConnection::processConnection: pending data:" << data.toHex();
        }
        return true;
    };
    //read first header
    {
        bool willContinue = false;
        m_socketEntry = new QTcpSocket(this);
        m_socketEntry->setSocketDescriptor(m_entrySocketID);
        qDebug() << "ProxyConnection::processConnection: id=" << m_entrySocketID
                 << "peerName:" << m_socketEntry->peerName()
                 << "local address:" << m_socketEntry->localAddress()
                 << "local port:" << m_socketEntry->localPort();
        if (!readData(2))
        {
            return;
        }
        if (data.at(0) != 5)
        {
            qDebug() << "ProxyConnection::processConnection: protocol version (" << (int)data.at(0) << ") is not 5";
            //delete c;
            this->deleteLater();
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
                    m_socketEntry->write(dataRet);
                    m_socketEntry->flush();
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
            qDebug() << "ProxyConnection::processConnection: this server is only support NO AUTHENTICATION";
            //delete c;
            this->deleteLater();
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
            qDebug() << "ProxyConnection::processConnection: protocol version (" << (int)data.at(0) << ") is not 5";
            //delete c;
            this->deleteLater();
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
            qDebug() << "ProxyConnection::processConnection: IP V4 address";
            if (!readData(6))
            {
                return;
            }
            else
            {
                hostData.append(data.mid(0, 6));
                quint32 ip = getIpv4(data.constData());
                //memcpy(&ip, data.constData(), 4);
                m_portExit = getPort(data.constData()+4);
                m_hostExit.setAddress(ip);
            }
            break;
        case 3:            //DOMAINNAME
            qDebug() << "ProxyConnection::processConnection: DOMAINNAME";
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
                bool ok = m_hostExit.setAddress(QString::fromUtf8(hostName));
                if (!ok)
                {
                    m_strhostExit = QString::fromUtf8(hostName);
                    if (m_strhostExit.contains("google"))
                    {
                        this->deleteLater();
                        return;
                    }
                }
                m_portExit = getPort(portData.constData());
                hostData.append(data.mid(0, 3+size));
            }
            break;
        case 4:            //IP V6 address
        {
            qDebug() << "ProxyConnection::processConnection: IP V6 address";
            quint8 ip[16];
            if (!readData(18))
            {
                return;
            }
            getIpv6(data.constData(), ip);
            m_portExit = getPort(data.constData()+16);
            m_hostExit.setAddress(ip);
            hostData.append(data.mid(0, 18));
            break;
        }
        default:
            qDebug() << "ProxyConnection::processConnection: unknown command " << (int)cmd;
            //delete c;
            this->deleteLater();
            return;
        }
        /*qDebug() << "ProxyConnection::processConnection: exit host:" << m_hostExit
                 << "exit port:" << m_portExit;*/
        connect(m_socketEntry.data(), SIGNAL(readyRead()),
                this, SLOT(on_Entry_readyRead()));
        connect(m_socketEntry.data(), SIGNAL(disconnected()),
                this, SLOT(on_Entry_disconnected()));
        QByteArray cmdReplyData;
        cmdReplyData.append((char)5);
        cmdReplyData.append((char)0);
        cmdReplyData.append((char)0);
        cmdReplyData.append(hostData);
        initLogDir();
        makeLogFile();
        switch (cmd)
        {
        case 1:            //CONNECT
        {
            m_socketExit_Tcp = new QTcpSocket(this);
            connect(m_socketExit_Tcp.data(), SIGNAL(readyRead()),
                    this, SLOT(on_TcpExit_readyRead()));
            connect(m_socketExit_Tcp.data(), SIGNAL(disconnected()),
                    this, SLOT(on_TcpExit_disconnected()));
            if ((m_hostExit.isNull()) || (!m_strhostExit.isEmpty()))
            {
                qDebug() << "ProxyConnection::processConnection: connect to" << m_strhostExit << ":" << m_portExit;
                m_socketExit_Tcp->connectToHost(m_strhostExit, m_portExit);
            }
            else
            {
                qDebug() << "ProxyConnection::processConnection: connect to" << m_hostExit << ":" << m_portExit;
                m_socketExit_Tcp->connectToHost(m_hostExit, m_portExit);
            }
            bool ok = m_socketExit_Tcp->waitForConnected();
            if (ok)
            {
                m_socketEntry->write(cmdReplyData);
            }
            else
            {
                QAbstractSocket::SocketError err = m_socketExit_Tcp->error();
                qDebug() << "ProxyConnection::processConnection: error string:" << m_socketExit_Tcp->errorString();
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
                m_socketEntry->write(cmdReplyData);

                if (!m_socketEntry.isNull())
                {
                    m_socketEntry->close();
                    //delete m_socketEntry.data();
                }
                if (!m_socketExit_Tcp.isNull())
                {
                    m_socketExit_Tcp->close();
                    //delete m_socketExit_Tcp.data();
                }
                if (!m_socketExit_Udp.isNull())
                {
                    m_socketExit_Udp->close();
                    //delete m_socketExit_Udp.data();
                }
                this->deleteLater();
            }
            break;
        }
        case 2:            //BIND
        {
            if ((m_hostExit.isNull()) || (!m_strhostExit.isEmpty()))
            {
                qDebug() << "ProxyConnection::processConnection: bind to" << m_strhostExit << ":" << m_portExit;
            }
            else
            {
                qDebug() << "ProxyConnection::processConnection: bind to" << m_hostExit << ":" << m_portExit;
            }
            cmdReplyData[1] = 7;
            m_socketEntry->write(cmdReplyData);
            //delete c;
            this->deleteLater();
            return;
        }
        case 3:            //UDP ASSOCIATE
            qDebug() << "ProxyConnection::processConnection: UDP ASSOCIATE to" << m_hostExit << ":" << m_portExit;
            m_socketExit_Udp = new QUdpSocket(this);
            connect(m_socketExit_Udp.data(), SIGNAL(readyRead()),
                    this, SLOT(on_UdpExit_readyRead()));
            m_udpEntryHeader.append((char)0);
            m_udpEntryHeader.append((char)0);
            m_udpEntryHeader.append((char)0);//frag
            m_udpEntryHeader.append(hostData);//type
            if ((m_hostExit.isNull()) || (!m_strhostExit.isEmpty()))
            {
                qDebug() << "ProxyConnection::processConnection: UDP ASSOCIATE to" << m_strhostExit << ":" << m_portExit;
                //m_socketExit_Udp->bind(m_strhostExit, m_portExit);
            }
            else
            {
                qDebug() << "ProxyConnection::processConnection: UDP ASSOCIATE to" << m_hostExit << ":" << m_portExit;
                m_socketExit_Udp->bind(m_hostExit, m_portExit);
            }
            m_socketEntry->write(cmdReplyData);
            break;
        default:
            qDebug() << "ProxyConnection::processConnection: unknown command " << (int)cmd;
            //delete c;
            this->deleteLater();
            return;
        }
    }
}

void ProxyConnection::on_Entry_readyRead()
{
    while (m_socketEntry->bytesAvailable() > 0)
    {
        QByteArray data = m_socketEntry->readAll();
        if (!m_socketExit_Tcp.isNull())
        {
            //qDebug() << "ProxyConnection::on_Entry_readyRead:" << data.toHex();
            m_socketExit_Tcp->write(data);
            m_socketExit_Tcp->flush();
            if (m_file_send.isOpen())
            {
                m_file_send.write(data);
                m_file_send.flush();
            }
        }
        else if (!m_socketExit_Udp.isNull())
        {
            //m_socketExit_Udp->writeDatagram(data);
        }
    }
}

void ProxyConnection::on_Entry_disconnected()
{
    qDebug() << "ProxyConnection::on_Entry_disconnected";
    on_Entry_readyRead();
    if (!m_socketEntry.isNull())
    {
        m_socketEntry->close();
        //m_socketExit_Tcp->deleteLater();
    }
    if (!m_socketExit_Tcp.isNull())
    {
        m_socketExit_Tcp->close();
        //m_socketExit_Tcp->deleteLater();
    }
    else if (!m_socketExit_Udp.isNull())
    {
        //m_socketExit_Udp->deleteLater();
    }
    this->deleteLater();
}

void ProxyConnection::on_TcpExit_readyRead()
{
    while (m_socketExit_Tcp->bytesAvailable() > 0)
    {
        QByteArray data = m_socketExit_Tcp->readAll();
        //qDebug() << "ProxyConnection::on_TcpExit_readyRead:" << data.toHex();
        m_socketEntry->write(data);
        m_socketEntry->flush();
        if (m_file_received.isOpen())
        {
            m_file_received.write(data);
            m_file_received.flush();
        }
    }
}

void ProxyConnection::on_TcpExit_disconnected()
{
    qDebug() << "ProxyConnection::on_TcpExit_disconnected";
    on_TcpExit_readyRead();
    if (!m_socketEntry.isNull())
    {
        m_socketEntry->close();
        //m_socketExit_Tcp->deleteLater();
    }
    if (!m_socketExit_Tcp.isNull())
    {
        m_socketExit_Tcp->close();
        //m_socketExit_Tcp->deleteLater();
    }
    this->deleteLater();
}

void ProxyConnection::on_UdpExit_readyRead()
{

}

void ProxyConnection::run()
{
    //this->moveToThread(this);
    QByteArray data;
    QByteArray dataRet;
    auto readData = [&] (int readBytes)
    {
        while (data.size() < readBytes)
        {
            bool ok = true;
            if (m_socketEntry->bytesAvailable() <= 0)
            {
                ok = m_socketEntry->waitForReadyRead(1000);
            }
            if (!ok)
            {
                qDebug() << "ProxyConnection::processConnection: time out";
                data.append(m_socketEntry->readAll());
                qDebug() << "ProxyConnection::processConnection: readBytes=" << readBytes << " pending data:" << data.toHex();
                this->deleteLater();
                return false;
            }
            data.append(m_socketEntry->read(readBytes-data.size()));
            //qDebug() << "ProxyConnection::processConnection: pending data:" << data.toHex();
        }
        return true;
    };
    //read first header
    {
        bool willContinue = false;
        //m_socketEntry = new QTcpSocket();
        //m_socketEntry->setSocketDescriptor(m_entrySocketID);
        qDebug() << "ProxyConnection::processConnection: id=" << m_entrySocketID
                 << "peerName:" << m_socketEntry->peerName()
                 << "local address:" << m_socketEntry->localAddress()
                 << "local port:" << m_socketEntry->localPort();
        if (!readData(2))
        {
            return;
        }
        if (data.at(0) != 5)
        {
            qDebug() << "ProxyConnection::processConnection: protocol version (" << (int)data.at(0) << ") is not 5";
            this->deleteLater();
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
                    m_socketEntry->write(dataRet);
                    m_socketEntry->flush();
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
            qDebug() << "ProxyConnection::processConnection: this server is only support NO AUTHENTICATION";
            this->deleteLater();
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
            qDebug() << "ProxyConnection::processConnection: protocol version (" << (int)data.at(0) << ") is not 5";
            this->deleteLater();
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
            qDebug() << "ProxyConnection::processConnection: IP V4 address";
            if (!readData(6))
            {
                return;
            }
            else
            {
                hostData.append(data.mid(0, 6));
                quint32 ip = 0;
                memcpy(&ip, data.constData(), 4);
                m_portExit = getPort(data.constData()+4);
                m_hostExit.setAddress(ip);
            }
            break;
        case 3:            //DOMAINNAME
            qDebug() << "ProxyConnection::processConnection: DOMAINNAME";
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
                m_hostExit.setAddress(QString::fromUtf8(hostName));
                m_portExit = getPort(portData.constData());
                hostData.append(data.mid(0, 3+size));
            }
            break;
        case 4:            //IP V6 address
        {
            qDebug() << "ProxyConnection::processConnection: IP V6 address";
            quint8 ip[16];
            if (!readData(18))
            {
                return;
            }
            memcpy(&ip, data.constData(), 16);
            m_portExit = getPort(data.constData()+16);
            m_hostExit.setAddress(ip);
            hostData.append(data.mid(0, 18));
            break;
        }
        default:
            qDebug() << "ProxyConnection::processConnection: unknown command " << (int)cmd;
            this->deleteLater();
            return;
        }
        qDebug() << "ProxyConnection::processConnection: exit host:" << m_hostExit
                 << "exit port:" << m_portExit;
        connect(m_socketEntry.data(), SIGNAL(readyRead()),
                this, SLOT(on_Entry_readyRead()));
        QByteArray cmdReplyData;
        cmdReplyData.append((char)5);
        cmdReplyData.append((char)0);
        cmdReplyData.append((char)0);
        cmdReplyData.append(hostData);
        switch (cmd)
        {
        case 1:            //CONNECT
        {
            qDebug() << "ProxyConnection::processConnection: connect to" << m_hostExit << ":" << m_portExit;
            m_socketExit_Tcp = new QTcpSocket();
            connect(m_socketExit_Tcp.data(), SIGNAL(readyRead()),
                    this, SLOT(on_TcpExit_readyRead()));
            m_socketExit_Tcp->connectToHost(m_hostExit, m_portExit);
            bool ok = m_socketExit_Tcp->waitForConnected();
            if (ok)
            {
                m_socketEntry->write(cmdReplyData);
                on_Entry_readyRead();
            }
            else
            {
                QAbstractSocket::SocketError err = m_socketExit_Tcp->error();
                qDebug() << "ProxyConnection::processConnection: error string:" << m_socketExit_Tcp->errorString();
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
                m_socketEntry->write(cmdReplyData);
            }
            break;
        }
        case 2:            //BIND
        {
            qDebug() << "ProxyConnection::processConnection: bind to" << m_hostExit << ":" << m_portExit;
            //qDebug() << "ProxyConnection::processConnection: unsupported command bind.";
            cmdReplyData[1] = 7;
            m_socketEntry->write(cmdReplyData);
            this->deleteLater();
            return;
            break;
        }
        case 3:            //UDP ASSOCIATE
            qDebug() << "ProxyConnection::processConnection: UDP ASSOCIATE to" << m_hostExit << ":" << m_portExit;
            m_socketExit_Udp = new QUdpSocket();
            connect(m_socketExit_Udp.data(), SIGNAL(readyRead()),
                    this, SLOT(on_UdpExit_readyRead()));
            m_socketExit_Udp->bind(m_hostExit, m_portExit);
            m_socketEntry->write(cmdReplyData);
            break;
        default:
            qDebug() << "ProxyConnection::processConnection: unknown command " << (int)cmd;
            this->deleteLater();
            return;
        }
    }
    exec();
}

QString ProxyConnection::newLogFileName() const
{
    QString strDateTime = QDateTime::currentDateTime().toString("yyyy.MM.dd_hh.mm.ss.zzz");
    //QString strServer = QString("%1_%2").arg(m_serverHost.toString()).arg(QString::number(m_serverPort));
    QString strPeer = QString("%1_%2").arg(m_socketEntry->peerAddress().toString()).arg(QString::number(m_socketEntry->peerPort()));
    QString strRet = QString("[%1]%2").arg(strDateTime).arg(strPeer);
    strRet.replace(QChar(':'), QChar('_'));
    return strRet;
}

QString ProxyConnection::newLogFilePath() const
{
    QString strFileName = newLogFileName();
    QString strRet = m_dirServer.absoluteFilePath(strFileName);
    //qDebug() << m_dirServer;
    return strRet;
}

void ProxyConnection::initLogDir()
{
    QString serverHost(serverHostName());
    QString strServerDir = QString("%1_%2").arg(serverHost).arg(QString::number(m_portExit));
    strServerDir.replace(QChar(':'), QChar('_'));
    QDir dirServer(QCoreApplication::applicationDirPath());
    dirServer.mkdir("log");
    dirServer.cd("log");
    bool ret = dirServer.cd(strServerDir);
    if (!ret)
    {
        ret = dirServer.mkdir(strServerDir);
        if (!ret)
        {
            qDebug() << "ProxyConnection::TcpSocket: Error: make dir" << strServerDir << "in " << dirServer << " failed.";
        }
        ret = dirServer.cd(strServerDir);
        if (!ret)
        {
            qDebug() << "ProxyConnection::TcpSocket: Error: cd to" << strServerDir << "in " << dirServer << " failed.";
        }
    }
    m_dirServer = dirServer;
}

void ProxyConnection::makeLogFile()
{
    //return;
    QString strPath = QDir::toNativeSeparators(newLogFilePath());
    //qDebug() << strPath;
    m_file_received.setFileName(strPath + "_received.log");
    m_file_send.setFileName(strPath + "_send.log");
    bool ret = m_file_received.open(QIODevice::Append);
    if (!ret)
    {
        qDebug() << "ProxyConnection::makeLogFile: Error open file" << m_file_received.fileName() << "failed."
                 << "Error:" << m_file_received.errorString();
    }
    ret = m_file_send.open(QIODevice::Append);
    if (!ret)
    {
        qDebug() << "ProxyConnection::makeLogFile: Error open file" << m_file_send.fileName() << "failed."
                 << "Error:" << m_file_send.errorString();
    }
}
