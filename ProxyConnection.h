#ifndef PROXYCONNECTION_H
#define PROXYCONNECTION_H

#include <QThread>
#include <QHostAddress>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTcpServer>
#include <QPointer>
#include <QFile>
#include <QDir>

class ProxyServer;

class ProxyConnection : public QThread
{
    Q_OBJECT
public:
    explicit ProxyConnection(QTcpServer *server,
                             qintptr entrySocketID,
                             QObject *parent = 0);
    ~ProxyConnection();

    void processConnection();
    QString serverHostName() const
    {
        if ((m_hostExit.isNull()) || (!m_strhostExit.isEmpty()))
        {
            return m_strhostExit;
        }
        else
        {
            return m_hostExit.toString();
        }
    }
    int connectionId() const
    {
        return m_id;
    }

protected:
    virtual void run();

private slots:
    void on_Entry_readyRead();
    void on_Entry_disconnected();
    void on_TcpExit_readyRead();
    void on_TcpExit_disconnected();
    void on_UdpExit_readyRead();

private:
    QString newLogFileName() const;
    QString newLogFilePath() const;
    void initLogDir();
    void makeLogFile();

    QDir m_dirServer;
    QFile m_file_received;
    QFile m_file_send;
    QPointer<QTcpServer> m_server;
    QPointer<QTcpSocket> m_socketEntry;
    QPointer<QTcpSocket> m_socketExit_Tcp;
    QPointer<QUdpSocket> m_socketExit_Udp;
    QHostAddress m_hostExit;
    QByteArray m_udpEntryHeader;
    QString m_strhostExit;
    quint16 m_portExit;
    const int m_entrySocketID;
    const int m_id;

    static int m_nextId;
};

#endif // PROXYCONNECTION_H
