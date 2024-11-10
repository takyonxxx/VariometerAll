#ifndef READGPS_H
#define READGPS_H

#include <QObject>
#include <QGeoPositionInfoSource>
#include <QGeoPositionInfo>
#include <QDebug>

class ReadGps : public QObject
{
    Q_OBJECT

public:
    explicit ReadGps(QObject *parent = nullptr);
    QList<qreal> captureGpsData();

signals:
    void sendInfo(const QList<qreal> &data);

private slots:
    void positionUpdated(const QGeoPositionInfo &info);

private:
    QGeoPositionInfoSource *source;
    QGeoPositionInfo positionInfo;
};

#endif // READGPS_H
