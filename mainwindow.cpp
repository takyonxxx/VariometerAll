#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utils.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Variometer");
    ui->labelGps->setStyleSheet("font-size: 32pt; color: #cccccc; background-color: #A73906;font-weight: bold;");
    ui->labelSensor->setStyleSheet("font-size: 32pt; color: #cccccc; background-color: #A73906;font-weight: bold;");
    ui->m_textStatus->setStyleSheet("font-size: 18pt; color: #cccccc; background-color: #003333;");    
    ui->pushExit->setStyleSheet("font-size: 24pt; font: bold; color: #ffffff; background-color:#041077;");
    ui->label_vario->setStyleSheet("font-size: 62pt; color: #0FCCCF ; background-color: #001a1a;font-weight: bold;");
    ui->label_pressure->setStyleSheet("font-size: 42pt; color: #cccccc; background-color: #001a1a;font-weight: bold;");
    ui->label_baro_altitude->setStyleSheet("font-size: 42pt; color: #cccccc; background-color: #001a1a;font-weight: bold;");
    ui->label_speed->setStyleSheet("font-size: 42pt; color: #0FCCCF ; background-color: #001a1a;font-weight: bold;");
    ui->label_gps_altitude->setStyleSheet("font-size: 42pt; color: #0FCCCF ; background-color: #001a1a;font-weight: bold;");

    accelVariance = KF_VAR_ACCEL;  // Initial value for KF_VAR_ACCEL
    measurementVariance = KF_VAR_MEASUREMENT;  // Initial value for KF_VAR_MEASUREMENT

    ui->scrollBarAccel->setRange(0, 100);  // Set the range from 0 to 100
    ui->scrollBarMeasurement->setRange(0, 100);  // Set the range from 0 to 100

    // Calculate the initial scrollbar values based on the initial variance values
    int initialScrollBarAccelValue = static_cast<int>(accelVariance * 100.0);
    int initialScrollBarMeasurementValue = static_cast<int>(measurementVariance * 100.0);

    // Set the initial scrollbar values
    ui->scrollBarAccel->setValue(initialScrollBarAccelValue);
    ui->scrollBarMeasurement->setValue(initialScrollBarMeasurementValue);

//    ui->scrollBarAccel->setStyleSheet("font-size: 18pt; font: bold; color: #ffffff; background-color: #061276 ;");
//    ui->scrollBarMeasurement->setStyleSheet("font-size: 18pt; font: bold; color: #ffffff; background-color: #061276 ;");
    ui->scrollBarAccel->setStyleSheet("QScrollBar::handle:vertical { background: #808080; }");
    ui->scrollBarMeasurement->setStyleSheet("QScrollBar::handle:vertical { background: #808080; }");

    stopReading = false;

    p_start = QDateTime::currentDateTime();
    altitude = 0;
    pressure = SEA_LEVEL_PRESSURE;
    pressure_filter = new KalmanFilter(accelVariance);
    pressure_filter->Reset(SEA_LEVEL_PRESSURE);
    altitude_filter = new KalmanFilter(accelVariance);
    altitude_filter->Reset(altitude);

    sensorManager = new SensorManager(this);
    connect(sensorManager, &SensorManager::sendInfo, this, &MainWindow::getSensorInfo);
    sensorManager->start();
    readGps = new ReadGps(this);
    connect(readGps, &ReadGps::sendInfo, this, &MainWindow::getGpsInfo);
}

MainWindow::~MainWindow()
{
    if (sensorManager)
    {
        sensorManager->quit();
        sensorManager->wait();
        delete sensorManager;
    }

    if (readGps)
        delete readGps;
    delete ui;
}


void MainWindow::getSensorInfo(QList<qreal> info)
{
    if(stopReading)
        return;

    pressure = info.at(0);
    temperature = info.at(1);
    quint64 timestamp = static_cast<quint64>(info.at(2));

    if (lastPressTimestamp > 0)
    {
        pDeltaT = ((qreal)(timestamp - lastPressTimestamp)) / 1000000.0f;

        if (pDeltaT > 0)
        {
            p_end = QDateTime::currentDateTime();
            qint64 elapsedTimeMillis = p_start.msecsTo(p_end);

            // Ensure elapsedTimeMillis is positive and greater than 0
            if (pressure != 0 && elapsedTimeMillis > 0)
            {
                p_dt = elapsedTimeMillis / 1000.0; // Convert milliseconds to seconds

                // Update pressure with Kalman filter
                pressure_filter->Update(pressure, KF_VAR_MEASUREMENT, p_dt);
                pressure = pressure_filter->GetXAbs();

                pressure = pressure * 0.01; // Convert pressure to hPa

                // Update barometric altitude with Kalman filter
                baroaltitude = 44330.0 * (1.0 - std::pow(pressure / SEA_LEVEL_PRESSURE_HPA, 0.19));
                altitude_filter->Update(baroaltitude, measurementVariance, p_dt);
                baroaltitude = altitude_filter->GetXAbs();

                // Update vario with Kalman filter using sensitivity and width parameters
                vario = altitude_filter->GetXVel();

                QString varioString = QString::number(qAbs(vario), 'f', 2) + "\nm/s";
                // Set text color based on the sign of vario
                if (vario < 0) {
                    ui->label_vario->setStyleSheet("font-size: 62pt; color: red; background-color: #001a1a; font-weight: bold;");
                } else {
                    ui->label_vario->setStyleSheet("font-size: 62pt; color: #0FCCCF; background-color: #001a1a; font-weight: bold;");
                }
                ui->label_vario->setText(varioString);

                QString pressureString = QString::number(pressure, 'f', 1) + " hPa";
                ui->label_pressure->setText(pressureString);

                QString baroAltString = QString::number(baroaltitude, 'f', 1) + " m";
                ui->label_baro_altitude->setText(baroAltString);

                p_start = p_end;
            }
        }
    }
    lastPressTimestamp = timestamp;
}


void MainWindow::getGpsInfo(QList<qreal> info)
{
    altitude = info.at(0);
    latitude = info.at(1);
    longitude = info.at(2);
    groundSpeed = static_cast<int>(info.at(3));

    QString altitudeString = QString::number(altitude , 'f', 1) + " m";
    ui->label_gps_altitude->setText(altitudeString);

    QString speedString = QString::number(groundSpeed, 'f', 1) + " km/h";
    ui->label_speed->setText(speedString);

    auto values = QString("Gps: %1").arg(listToString(info));
    printInfo(values);
}

void MainWindow::printInfo(QString info)
{
    ui->m_textStatus->setText(info);
}

void MainWindow::on_pushExit_clicked()
{
    sensorManager->setStop(true);
    // Quit the application
    QCoreApplication::exit();
}

void MainWindow::on_scrollBarMeasurement_valueChanged(int value)
{
     measurementVariance = static_cast<qreal>(value) / 100.0;
}

void MainWindow::on_scrollBarAccel_valueChanged(int value)
{
     stopReading = true;
     accelVariance = static_cast<qreal>(value) / 100.0;
     delete pressure_filter;
     delete altitude_filter;
     pressure_filter = new KalmanFilter(accelVariance);
     altitude_filter = new KalmanFilter(accelVariance);
     stopReading = false;
}

