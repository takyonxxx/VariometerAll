#include "readgps.h"

ReadGps::ReadGps(QObject *parent): QObject(parent)
{
    source = QGeoPositionInfoSource::createDefaultSource(this);
    if (source) {
        connect(source, SIGNAL(positionUpdated(QGeoPositionInfo)),
                this, SLOT(positionUpdated(QGeoPositionInfo)));
        source->setUpdateInterval(500);
        source->startUpdates();
    }

}

QList<qreal> ReadGps::captureGpsData()
{
    QList <qreal> temp;
    temp.clear();
    temp.append(positionInfo.coordinate().altitude());
    temp.append(positionInfo.coordinate().latitude());
    temp.append(positionInfo.coordinate().longitude());    
    temp.append(static_cast<int>(positionInfo.attribute(QGeoPositionInfo::GroundSpeed)*3.75));
    return temp;
}

void ReadGps::positionUpdated(const QGeoPositionInfo &info)
{
    positionInfo = info;
    emit sendInfo(captureGpsData());
}
