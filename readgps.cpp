#include "readgps.h"

ReadGps::ReadGps(QObject *parent)
    : QObject(parent)
    , source(nullptr)
    , retryTimer(nullptr)
    , retryCount(0)
    , updatesStarted(false)
{
    checkPermissionAndInitialize();
}

ReadGps::~ReadGps()
{
    cleanupGPS();
}

void ReadGps::checkPermissionAndInitialize()
{
    if (source) {
        cleanupGPS();
    }

    source = QGeoPositionInfoSource::createDefaultSource(this);

    if (source) {      
        connect(source, &QGeoPositionInfoSource::positionUpdated,
                this, &ReadGps::positionUpdated);
        connect(source, &QGeoPositionInfoSource::errorOccurred,
                this, &ReadGps::handleError);

        // Set high accuracy mode
        source->setPreferredPositioningMethods(QGeoPositionInfoSource::AllPositioningMethods);
        source->setUpdateInterval(1000);

        // Request a single update first to trigger the permission dialog
        source->requestUpdate(1000);
        updatesStarted = false;
    } else {
        qDebug() << "Error: Could not create position source";
    }
}

void ReadGps::initializeGPS()
{
    if (!source) return;

    if (!updatesStarted) {
        source->startUpdates();
        updatesStarted = true;        
    }
}

void ReadGps::cleanupGPS()
{
    stopRetryTimer();

    if (source) {
        source->stopUpdates();
        delete source;
        source = nullptr;
    }
    updatesStarted = false;
}

void ReadGps::stopRetryTimer()
{
    if (retryTimer) {
        retryTimer->stop();
        delete retryTimer;
        retryTimer = nullptr;
    }
    retryCount = 0;
}

QList<qreal> ReadGps::captureGpsData()
{
    QList<qreal> temp;

    if (!positionInfo.isValid()) {
        return temp;
    }

    temp.append(positionInfo.coordinate().altitude());
    temp.append(positionInfo.coordinate().latitude());
    temp.append(positionInfo.coordinate().longitude());

    qreal speed = positionInfo.hasAttribute(QGeoPositionInfo::GroundSpeed)
                      ? positionInfo.attribute(QGeoPositionInfo::GroundSpeed) * 3.75
                      : 0.0;
    temp.append(speed);

    return temp;
}

void ReadGps::positionUpdated(const QGeoPositionInfo &info)
{
    stopRetryTimer();

    positionInfo = info;
    emit sendInfo(captureGpsData());

    // Start continuous updates if we haven't already
    if (!updatesStarted) {
        initializeGPS();
    }
}

void ReadGps::handleError()
{
    if (!source) return;

    switch (source->error()) {
    case QGeoPositionInfoSource::AccessError:
        qDebug() << "Location permission denied - Please enable in Settings";
        updatesStarted = false;

        if (retryCount < MAX_RETRIES) {
            if (!retryTimer) {
                retryTimer = new QTimer(this);
                retryTimer->setSingleShot(true);
                connect(retryTimer, &QTimer::timeout, this, [this]() {
                    retryCount++;
                    checkPermissionAndInitialize();
                });
            }
            retryTimer->start(2000); // Retry after 2 seconds
        } else {
            emit permissionDenied();
            qDebug() << "Maximum retry attempts reached. Please enable location services in Settings.";
            qDebug() << "To enable location services:"
                     << "\n1. Open Settings"
                     << "\n2. Navigate to Privacy > Location Services"
                     << "\n3. Find your app and select 'Always' or 'While Using'";
        }
        break;

    case QGeoPositionInfoSource::ClosedError:
        qDebug() << "Position source closed - Attempting to restart";
        checkPermissionAndInitialize();
        break;

    default:
        qDebug() << "Position source error:" << source->error()
                 << "- Attempting to restart GPS";
        checkPermissionAndInitialize();
        break;
    }
}
