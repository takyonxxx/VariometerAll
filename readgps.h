#ifndef READGPS_H
#define READGPS_H

#include <QObject>
#include <QGeoPositionInfoSource>
#include <QGeoPositionInfo>
#include <QDebug>
#include <QTimer>

class ReadGps : public QObject
{
    Q_OBJECT
public:
    explicit ReadGps(QObject *parent = nullptr);
    ~ReadGps();
    QList<qreal> captureGpsData();

signals:
    void sendInfo(QList<qreal>);
    void permissionDenied();
    void gpsTimeout();

private slots:
    void positionUpdated(const QGeoPositionInfo &info);
    void handleError();
    void checkPermissionAndInitialize();
    void retryUpdate();

private:
    QGeoPositionInfoSource *source;
    QGeoPositionInfo positionInfo;
    QTimer *retryTimer;
    int retryCount;
    bool updatesStarted;
    bool isWaitingForFix;

    static const int MAX_RETRIES = 5;
    static const int INITIAL_TIMEOUT = 30000;    // 30 seconds for first fix
    static const int NORMAL_TIMEOUT = 1000;      // 1 second for updates
    static const int RETRY_INTERVAL = 5000;      // 5 seconds between retries

    void initializeGPS();
    void cleanupGPS();
    void stopRetryTimer();
    void startRetryTimer();
    void requestSingleUpdate();
};

#endif // READGPS_H
