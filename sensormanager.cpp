#include "sensormanager.h"

SensorManager::SensorManager(QObject *parent) :
    QThread(parent), m_stop(false)
{
    findSensors();
    startSensors();
}

SensorManager::~SensorManager()
{
    // Clean up mySensorList and other resources if needed
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
    for (QSensor* sensor : mySensorList) {
        if (sensor->type() == QPressureSensor::sensorType) {
            hasPressureSensor = true;
            break;
        }
    }

    if (hasPressureSensor) {
        sensorPressure = new QSensor("QPressureSensor", this);
        sensorPressure->start();
        qDebug() << "QPressureSensor started.";
    } else {
        qDebug() << "QPressureSensor not found in mySensorList.";
    }

    //    sensorAcc = new QSensor("QAccelerometer");
    //    sensorAcc->start();
    //    sensorGyro = new QSensor("QGyroscope");
    //    sensorGyro->start();
    //    sensorMag = new QSensor("QMagnetometer");
    //    sensorMag->start();
    //    sensorCompass = new QSensor("QCompass");
    //    sensorCompass->start();
    //    sensorTilt = new QSensor("QTiltSensor");
    //    sensorTilt->start();
    //    sensorOrientation = new QSensor("QOrientationSensor");
    //    sensorOrientation->start();
    //    sensorProximity = new QSensor("QProximitySensor");
    //    sensorProximity->start();
    //    sensorRotation = new QSensor("QRotationSensor");
    //    sensorRotation->start();
}

void SensorManager::readSensorValues()
{
    emit sendInfo(readPressure());
}

void SensorManager::setStop(bool newStop)
{
    m_stop = newStop;
}

void SensorManager::run()
{
     while (!m_stop)
     {
         if(m_stop)
             break;

         emit sendInfo(readPressure());
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

QList<qreal> SensorManager::readMagnetometer()
{
    QList <qreal> temp;
    if(!sensorMag)
        return temp;

    temp.clear();
    temp.append(sensorMag->reading()->property("x").value<qreal>());
    temp.append(sensorMag->reading()->property("y").value<qreal>());
    temp.append(sensorMag->reading()->property("z").value<qreal>());
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
