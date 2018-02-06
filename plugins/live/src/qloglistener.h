#ifndef QLOGLISTENER_H
#define QLOGLISTENER_H

#include <QObject>
#include <QQmlParserStatus>

class QTcpServer;
class QLogListener : public QObject, public QQmlParserStatus{

    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QString address READ address WRITE setAddress NOTIFY addressChanged)
    Q_PROPERTY(int     port    READ port    WRITE setPort    NOTIFY portChanged)
    Q_PROPERTY(QStringList allowedIps READ allowedIps WRITE setAllowedIps NOTIFY allowedIpsChanged)

public:
    explicit QLogListener(QObject *parent = nullptr);
    ~QLogListener();

    QString address() const;
    int port() const;
    const QStringList& allowedIps() const;

    void setAddress(QString address);
    void setPort(int port);
    void setAllowedIps(const QStringList& allowedIps);

    void componentComplete();
    void classBegin(){}

public slots:
    void newConnection();

signals:
    void addressChanged();
    void portChanged();
    void allowedIpsChanged();
    void listening();

private:
    void startListening();

    QString     m_address;
    int         m_port;
    QStringList m_allowedIps;
    QTcpServer* m_server;

    bool        m_componentComplete;
};

inline QString QLogListener::address() const{
    return m_address;
}

inline int QLogListener::port() const{
    return m_port;
}

inline const QStringList &QLogListener::allowedIps() const{
    return m_allowedIps;
}

#endif // QLOGLISTENER_H
