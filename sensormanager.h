#ifndef SENSORMANAGER_H
#define SENSORMANAGER_H

#include <QObject>
#include <QList>
#include <QDebug>
#include <QThread>
#include <QSensor>
#include <QPressureSensor>

class SensorManager : public QThread
{
    Q_OBJECT

public:
    explicit SensorManager(QObject *parent = nullptr);
    ~SensorManager();

    Q_INVOKABLE QList <qreal> readPressure();
    Q_INVOKABLE QList <qreal> readGyro();
    Q_INVOKABLE QList <qreal> readMagnetometer();
    Q_INVOKABLE QList <qreal> readCompass();

    void findSensors();
    void startSensors();
    void readSensorValues();

    void setStop(bool newStop);

private:
    QSensor *sensorPressure;
    QSensor *sensorAcc;
    QSensor *sensorGyro;
    QSensor *sensorMag;
    QSensor *sensorTilt;
    QSensor *sensorCompass;
    QSensor *sensorOrientation;
    QSensor *sensorProximity;
    QSensor *sensorRotation;

    bool m_stop;

signals:
    void sendInfo(QList <qreal>);
private:
    QList<QSensor*> mySensorList;

protected:
    void run() override;
};


#endif // SENSORMANAGER_H
