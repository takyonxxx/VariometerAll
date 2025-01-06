#ifndef SENSORMANAGER_H
#define SENSORMANAGER_H

#include <QObject>
#include <QList>
#include <QDebug>
#include <QThread>
#include <QSensor>
#include <QPressureSensor>
#include <QAccelerometer>
#include <QGyroscope>
#include <QCompass>
#include <QAmbientTemperatureSensor>
#include <QMetaProperty>

class SensorManager : public QThread
{
    Q_OBJECT

public:
    explicit SensorManager(QObject *parent = nullptr);
    ~SensorManager();

    Q_INVOKABLE QList <qreal> readPressure();
    Q_INVOKABLE QList <qreal> readGyro();
    Q_INVOKABLE QList <qreal> readAcc();
    Q_INVOKABLE QList <qreal> readCompass();
    Q_INVOKABLE QList <qreal> readTemperature();

    void findSensors();
    void startSensors();
    void readSensorValues();
    qreal calculateRoll();
    qreal calculatePitch();

    void setStop();

private:
    QPressureSensor* sensorPressure = nullptr;
    QAccelerometer* sensorAcc = nullptr;
    QGyroscope* sensorGyro = nullptr;
    QCompass* sensorCompass = nullptr;
    QAmbientTemperatureSensor* sensorTemperature = nullptr;

    void processAccelerometerData();
    static constexpr qreal RAD_TO_DEG = 180.0 / M_PI;

    // Add these member variables to store processed values
    qreal m_roll = 0.0;
    qreal m_pitch = 0.0;
    qreal m_accX = 0.0;
    qreal m_accY = 0.0;
    qreal m_accZ = 0.0;

    bool m_stop;

signals:
    void sendPressureInfo(QList <qreal>);
    void sendAccInfo(QList <qreal>);
    void sendGyroInfo(QList <qreal>);
    void sendCompassInfo(QList <qreal>);
private:
    QList<QSensor*> mySensorList;

protected:
    void run() override;
};


#endif // SENSORMANAGER_H
