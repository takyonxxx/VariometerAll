#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utils.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    pressure (SEA_LEVEL_PRESSURE),
    altitude (0),
    stopReading (false),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Variometer");
    ui->labelGps->setStyleSheet("font-size: 32pt; color: #cccccc; background-color: #A73906;font-weight: bold;");
    ui->labelSensor->setStyleSheet("font-size: 32pt; color: #cccccc; background-color: #A73906;font-weight: bold;");
    ui->m_textStatus->setStyleSheet("font-size: 18pt; color: #cccccc; background-color: #003333;");    
    ui->pushExit->setStyleSheet("font-size: 24pt; font: bold; color: #ffffff; background-color:#041077;");
    ui->pushReset->setStyleSheet("font-size: 24pt; font: bold; color: #ffffff; background-color:#041077;");
    ui->label_vario->setStyleSheet("font-size: 62pt; color: #0FCCCF ; background-color: #001a1a;font-weight: bold;");
    ui->label_vario_unit->setStyleSheet("font-size: 62pt; color: #0FCCCF ; background-color: #001a1a;font-weight: bold;");
    ui->label_pressure->setStyleSheet("font-size: 42pt; color: #cccccc; background-color: #001a1a;font-weight: bold;");
    ui->label_baro_altitude->setStyleSheet("font-size: 42pt; color: #cccccc; background-color: #001a1a;font-weight: bold;");
    ui->label_speed->setStyleSheet("font-size: 42pt; color: #0FCCCF ; background-color: #001a1a;font-weight: bold;");
    ui->label_gps_altitude->setStyleSheet("font-size: 42pt; color: #0FCCCF ; background-color: #001a1a;font-weight: bold;");
    ui->labelMV->setStyleSheet("font-size: 16pt; color: #ffffff; background-color: #FF4633;");
    ui->labelAV->setStyleSheet("font-size: 16pt; color: #ffffff; background-color: #FF4633;");
    ui->labelM->setStyleSheet("font-size: 16pt; color: #ffffff; background-color: #001a1a;;font-weight: bold;");
    ui->labelA->setStyleSheet("font-size: 16pt; color: #ffffff; background-color: #001a1a;font-weight: bold;");

    accelVariance = static_cast<qreal>(KF_VAR_ACCEL);  // Initial value for KF_VAR_ACCEL
    measurementVariance = static_cast<qreal>(KF_VAR_MEASUREMENT);  // Initial value for KF_VAR_MEASUREMENT

    ui->scrollBarAccel->setRange(0, 100);  // Set the range from 0 to 100
    ui->scrollBarMeasurement->setRange(0, 100);  // Set the range from 0 to 100

    // Calculate the initial scrollbar values based on the initial variance values
    int initialScrollBarAccelValue = static_cast<int>(accelVariance * 1000.0);
    int initialScrollBarMeasurementValue = static_cast<int>(measurementVariance * 100.0);

//    // Set the initial scrollbar values
    ui->scrollBarAccel->setValue(initialScrollBarAccelValue);
    ui->scrollBarMeasurement->setValue(initialScrollBarMeasurementValue);

    ui->scrollBarAccel->setStyleSheet("QScrollBar::handle:vertical { background: #808080; }");
    ui->scrollBarMeasurement->setStyleSheet("QScrollBar::handle:vertical { background: #808080; }");

    p_start = QDateTime::currentDateTime();

    pressure_filter = std::make_shared<KalmanFilter>(accelVariance);
    altitude_filter = std::make_shared<KalmanFilter>(accelVariance);
    pressure_filter->Reset(SEA_LEVEL_PRESSURE);
    altitude_filter->Reset(altitude);

    sensorManager = new SensorManager(this);
    connect(sensorManager, &SensorManager::sendInfo, this, &MainWindow::getSensorInfo);
    sensorManager->start();
    readGps = new ReadGps(this);
    connect(readGps, &ReadGps::sendInfo, this, &MainWindow::getGpsInfo);
    varioSound = new VarioSound(this);
    varioSound->start();
}

MainWindow::~MainWindow()
{
    if (sensorManager)
    {
        sensorManager->quit();
        sensorManager->wait();
        delete sensorManager;
    }

    if (varioSound)
    {
        varioSound->quit();
        varioSound->wait();
        delete varioSound;
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

                QString varioString = QString::number(qAbs(vario), 'f', 2);
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

void MainWindow::on_scrollBarMeasurement_valueChanged(int value)
{
    if(value == 0)
        return;

     measurementVariance = static_cast<qreal>(value) / 100.0;
     ui->labelMV->setText(QString::number(static_cast<double>(measurementVariance), 'f', 2));
}

void MainWindow::on_scrollBarAccel_valueChanged(int value)
{
     if(value == 0)
        return;

     stopReading = true;
     accelVariance = static_cast<qreal>(value) / 1000.0;
     pressure_filter = std::make_shared<KalmanFilter>(accelVariance);
     altitude_filter = std::make_shared<KalmanFilter>(accelVariance);
     ui->labelAV->setText(QString::number(static_cast<double>(accelVariance), 'f', 3));
     stopReading = false;
}

void MainWindow::on_pushReset_clicked()
{
     varioSound->updateVario(350.0);

     stopReading = true;
     accelVariance = static_cast<qreal>(KF_VAR_ACCEL);
     measurementVariance = static_cast<qreal>(KF_VAR_MEASUREMENT);
     pressure_filter = std::make_shared<KalmanFilter>(accelVariance);
     altitude_filter = std::make_shared<KalmanFilter>(accelVariance);
     int initialScrollBarAccelValue = static_cast<int>(accelVariance * 1000.0);
     int initialScrollBarMeasurementValue = static_cast<int>(measurementVariance * 100.0);
     ui->scrollBarAccel->setValue(initialScrollBarAccelValue);
     ui->scrollBarMeasurement->setValue(initialScrollBarMeasurementValue);
     ui->m_textStatus->clearHistory();
     stopReading = false;
}

void MainWindow::on_pushExit_clicked()
{
    varioSound->setStop(true);
    sensorManager->setStop(true);
    // Quit the application
    QCoreApplication::exit();
}
