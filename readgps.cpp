#include "readgps.h"

ReadGps::ReadGps(QObject *parent)
    : QObject(parent)
    , source(nullptr)
    , retryTimer(nullptr)
    , retryCount(0)
    , updatesStarted(false)
    , isWaitingForFix(false)
{
    // Delay initialization slightly to avoid UI blocking
    QTimer::singleShot(100, this, &ReadGps::checkPermissionAndInitialize);
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
        source->setUpdateInterval(NORMAL_TIMEOUT);

        // Start with a single update request
        requestSingleUpdate();
    } else {
        qDebug() << "Error: Could not create position source. Verify location permissions in Info.plist";
    }
}

void ReadGps::requestSingleUpdate()
{
    if (!source) return;

    isWaitingForFix = true;
    source->requestUpdate(INITIAL_TIMEOUT);

    // Start retry timer for timeout handling
    startRetryTimer();
}

void ReadGps::startRetryTimer()
{
    if (!retryTimer) {
        retryTimer = new QTimer(this);
        retryTimer->setSingleShot(true);
        connect(retryTimer, &QTimer::timeout, this, &ReadGps::retryUpdate);
    }
    retryTimer->start(INITIAL_TIMEOUT + 1000); // Slightly longer than the update request
}

void ReadGps::retryUpdate()
{
    if (!isWaitingForFix) return;

    retryCount++;
    qDebug() << "GPS timeout - Attempt" << retryCount << "of" << MAX_RETRIES;

    if (retryCount >= MAX_RETRIES) {
        emit gpsTimeout();
        retryCount = 0;
        isWaitingForFix = false;

        // Try restarting the GPS
        cleanupGPS();
        QTimer::singleShot(1000, this, &ReadGps::checkPermissionAndInitialize);
        return;
    }

    // Try another single update
    requestSingleUpdate();
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
    isWaitingForFix = false;
    retryCount = 0;
}

void ReadGps::stopRetryTimer()
{
    if (retryTimer) {
        retryTimer->stop();
        delete retryTimer;
        retryTimer = nullptr;
    }
}

QList<qreal> ReadGps::captureGpsData()
{
    QList<qreal> temp;

    if (!positionInfo.isValid()) {
        return temp;
    }

    temp.append(positionInfo.coordinate().altitude());

    qreal heading = positionInfo.hasAttribute(QGeoPositionInfo::Direction)
                        ? positionInfo.attribute(QGeoPositionInfo::Direction)
                        : 0.0;
    temp.append(heading);

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
    isWaitingForFix = false;
    retryCount = 0;

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
        isWaitingForFix = false;
        emit permissionDenied();
        break;

    case QGeoPositionInfoSource::UpdateTimeoutError:
        if (!retryTimer || !retryTimer->isActive()) {
            retryUpdate();
        }
        break;

    case QGeoPositionInfoSource::ClosedError:
        qDebug() << "Position source closed - Attempting to restart";
        cleanupGPS();
        QTimer::singleShot(1000, this, &ReadGps::checkPermissionAndInitialize);
        break;

    default:
        qDebug() << "Position source error:" << source->error() << "- Attempting to restart";
        cleanupGPS();
        QTimer::singleShot(1000, this, &ReadGps::checkPermissionAndInitialize);
        break;
    }
}
