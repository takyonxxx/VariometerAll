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

private slots:
    void positionUpdated(const QGeoPositionInfo &info);
    void handleError();
    void checkPermissionAndInitialize();

private:
    QGeoPositionInfoSource *source;
    QGeoPositionInfo positionInfo;
    QTimer *retryTimer;
    int retryCount;
    bool updatesStarted;
    static const int MAX_RETRIES = 3;

    void initializeGPS();
    void cleanupGPS();
    void stopRetryTimer();
};

#endif // READGPS_H
