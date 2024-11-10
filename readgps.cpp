#include "readgps.h"

ReadGps::ReadGps(QObject *parent)
    : QObject(parent)
{
    source = QGeoPositionInfoSource::createDefaultSource(this);
    if (source) {
        // Modern signal-slot syntax
        connect(source, &QGeoPositionInfoSource::positionUpdated,
                this, &ReadGps::positionUpdated);
        source->setUpdateInterval(1000);
        source->startUpdates();
        qDebug() << "GPS Enabled";
    } else {
        qDebug() << "GPS Not Found";
    }
}

QList<qreal> ReadGps::captureGpsData()
{
    QList<qreal> temp;
    temp.append(positionInfo.coordinate().altitude());
    temp.append(positionInfo.coordinate().latitude());
    temp.append(positionInfo.coordinate().longitude());
    temp.append(static_cast<int>(positionInfo.attribute(QGeoPositionInfo::GroundSpeed) * 3.75));
    return temp;
}

void ReadGps::positionUpdated(const QGeoPositionInfo &info)
{
    positionInfo = info;
    emit sendInfo(captureGpsData());
}
