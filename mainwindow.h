#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "sensormanager.h"
#include "readgps.h"
#include "kalmanfilter.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    void on_pushExit_clicked();
    void on_scrollBarMeasurement_valueChanged(int value);
    void on_scrollBarAccel_valueChanged(int value);

private:
    void getSensorInfo(QList <qreal>);
    void getGpsInfo(QList <qreal>);
    void printInfo(QString);
    SensorManager *sensorManager{};
    ReadGps *readGps{};

    KalmanFilter *pressure_filter;
    qreal p_dt;
    quint64 lastPressTimestamp ;
    qreal pDeltaT ;
    QDateTime p_start;
    QDateTime p_end;

    KalmanFilter *altitude_filter;
    qreal accelVariance;
    qreal measurementVariance;

    qreal latitude;
    qreal longitude;
    int groundSpeed;
    qreal pressure;
    qreal temperature;
    qreal altitude;
    qreal baroaltitude;
    qreal vario;

    bool stopReading;

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
