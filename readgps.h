#ifndef READGPS_H
#define READGPS_H


#include <QObject>
#include <QGeoCoordinate>
#include <QGeoPositionInfoSource>
#include <QDebug>
#include <QList>

/////////////
///\brief Read gps data
///
class ReadGps : public QObject{
    Q_OBJECT
public:
    explicit ReadGps(QObject *parent = nullptr);
    QGeoPositionInfoSource *source;
    Q_INVOKABLE QList <qreal> captureGpsData();
    QGeoPositionInfo positionInfo;
signals:
    void sendInfo(QList <qreal>);
private slots:
    void positionUpdated(const QGeoPositionInfo &info);
};

#endif // READGPS_H
