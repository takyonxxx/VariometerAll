#include "sensormanager.h"

SensorManager::SensorManager(QObject *parent) :
    QThread(parent), m_stop(false)
{
    findSensors();
    startSensors();
}

SensorManager::~SensorManager()
{
    m_stop = true;
    wait(); // Wait for thread to finish

    // Stop all sensors
    if (sensorPressure) {
        sensorPressure->stop();
        delete sensorPressure;
    }
    if (sensorAcc) {
        sensorAcc->stop();
        delete sensorAcc;
    }
    if (sensorGyro) {
        sensorGyro->stop();
        delete sensorGyro;
    }
    if (sensorCompass) {
        sensorCompass->stop();
        delete sensorCompass;
    }

    // Clean up sensor list
    qDeleteAll(mySensorList);
    mySensorList.clear();
}

void SensorManager::findSensors()
{
    qDebug() << "Searching sensors...";
    for (const QByteArray &type : QSensor::sensorTypes()) {
        qDebug() << "Found a sensor type:" << type;
        for (const QByteArray &identifier : QSensor::sensorsForType(type)) {
            QSensor* sensor = new QSensor(type, this);
            sensor->setIdentifier(identifier);
            mySensorList.append(sensor);
        }
    }
    if (mySensorList.isEmpty())
        qDebug() << "No sensor found.";

}

void SensorManager::startSensors()
{
    bool hasPressureSensor = false;
    bool hasAccelerometerSensor = false;
    bool hasGyroscopeSensor = false;
    bool hasCompassSensor = false;

    for (QSensor* sensor : mySensorList) {
        if (sensor->type() == QPressureSensor::sensorType) {
            hasPressureSensor = true;
        }
        else if (sensor->type() == QAccelerometer::sensorType) {
            hasAccelerometerSensor = true;
        }
        else if (sensor->type() == QGyroscope::sensorType) {
            hasGyroscopeSensor = true;
        }
        else if (sensor->type() == QCompass::sensorType) {
            hasCompassSensor = true;
        }
    }

    if (hasPressureSensor) {
        sensorPressure = new QPressureSensor(this);
        if (sensorPressure->start())
            qDebug() << "QPressureSensor started.";
        else
            qDebug() << "Failed to start QPressureSensor.";
    } else {
        qDebug() << "QPressureSensor not found in SensorList.";
    }

    if (hasAccelerometerSensor) {
        sensorAcc = new QAccelerometer(this);
        if (sensorAcc->start())
            qDebug() << "QAccelerometer started.";
        else
            qDebug() << "Failed to start QAccelerometer.";
    } else {
        qDebug() << "QAccelerometer not found in SensorList.";
    }

    // if (hasGyroscopeSensor) {
    //     sensorGyro = new QGyroscope(this);
    //     if (sensorGyro->start())
    //         qDebug() << "QGyroscope started.";
    //     else
    //         qDebug() << "Failed to start QGyroscope.";
    // } else {
    //     qDebug() << "QGyroscope not found in SensorList.";
    // }

    // if (hasCompassSensor) {
    //     sensorCompass = new QCompass(this);
    //     if (sensorCompass->start())
    //         qDebug() << "QCompass started.";
    //     else
    //         qDebug() << "Failed to start QCompass.";
    // } else {
    //     qDebug() << "QCompass not found in SensorList.";
    // }
}

void SensorManager::readSensorValues()
{
    processAccelerometerData();
    emit sendPressureInfo(readPressure());
    emit sendAccInfo(readAcc());
    // emit sendGyroInfo(readGyro());
    // emit sendCompassInfo(readCompass());
}

void SensorManager::setStop()
{
    m_stop = true;
}

void SensorManager::run()
{
    while (!m_stop)
    {
        if(m_stop)
            break;

        processAccelerometerData();
        emit sendPressureInfo(readPressure());
        emit sendAccInfo(readAcc());
        msleep(250);
    }
}

QList<qreal> SensorManager::readPressure()
{
    QList <qreal> temp;
    if(!sensorPressure)
        return temp;

    temp.clear();
    temp.append(sensorPressure->reading()->property("pressure").value<qreal>());
    temp.append(sensorPressure->reading()->property("temperature").value<qreal>());
    temp.append(sensorPressure->reading()->property("timestamp").value<qreal>());
    return temp;
}

QList<qreal> SensorManager::readGyro()
{
    QList <qreal> temp;
    if(!sensorGyro)
        return temp;

    temp.clear();
    temp.append(sensorGyro->reading()->property("x").value<qreal>());
    temp.append(sensorGyro->reading()->property("z").value<qreal>());
    temp.append(sensorGyro->reading()->property("y").value<qreal>());
    return temp;
}

QList<qreal> SensorManager::readCompass()
{
    QList <qreal> temp;
    if(!sensorCompass)
        return temp;

    temp.clear();
    temp.append(sensorCompass->reading()->property("azimuth").value<qreal>());
    return temp;
}

// Modify your readAcc() function to also process roll and pitch
QList<qreal> SensorManager::readAcc()
{
    QList<qreal> temp;
    if (!sensorAcc)
        return temp;

    QAccelerometerReading* reading = sensorAcc->reading();
    if (!reading)
        return temp;
    // Process accelerometer data and update roll/pitch
    processAccelerometerData();

    // Add roll and pitch to the return values
    temp.append(m_roll);
    temp.append(m_pitch);
    temp.append(reading->x());
    temp.append(reading->y());
    temp.append(reading->z());

    return temp;
}

void SensorManager::processAccelerometerData()
{
    if (!sensorAcc)
        return;

    QAccelerometerReading* reading = sensorAcc->reading();
    if (!reading)
        return;

    const qreal alpha = 0.1;

    // Telefon dikey tutulduğunda:
    // x: sağ-sol yatırma (roll)
    // y: öne-arkaya eğme (pitch)
    // z: dikey eksen
    m_accX = m_accX * (1 - alpha) + reading->y() * alpha;  // Roll için Y ekseni
    m_accY = m_accY * (1 - alpha) - reading->x() * alpha;  // Pitch için X ekseni (ters)
    m_accZ = m_accZ * (1 - alpha) + reading->z() * alpha;

    m_roll = calculateRoll();
    m_pitch = calculatePitch();
}

qreal SensorManager::calculateRoll()
{
    // Pitch hesaplaması (öne-arkaya eğme)
    qreal acc_total = sqrt(m_accX * m_accX + m_accY * m_accY + m_accZ * m_accZ);

    if (acc_total == 0)
        return 0.0;

    qreal roll = atan2(m_accY, sqrt(m_accX * m_accX + m_accZ * m_accZ)) * RAD_TO_DEG;

    // -90 ile +90 derece arasına normalize et
    if (roll > 90) roll = 180 - roll;
    if (roll < -90) roll = -180 - roll;

    return roll;
}

qreal SensorManager::calculatePitch()
{
    // Roll hesaplaması (sağ-sol yatırma)
    qreal pitch = atan2(m_accX, m_accZ) * RAD_TO_DEG;

    // -180 ile +180 derece arasına normalize et
    if (pitch > 180) pitch -= 360;
    if (pitch < -180) pitch += 360;

    return pitch;
}
