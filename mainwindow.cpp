#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtMath>
#include <QDebug>
#include <stdexcept>

// Color constants for avionic display
namespace DisplayColors {
const QString DISPLAY_POSITIVE = "#00FF00";  // Green for positive values
const QString DISPLAY_NEGATIVE = "#FF0000";  // Red for negative values
const QString DISPLAY_NEUTRAL = "#0FCCCF";   // Cyan for neutral values
const QString BACKGROUND_DARK = "#001a1a";   // Dark background
const QString TEXT_LIGHT = "#cccccc";        // Light text
const QString BUTTON_BLUE = "#041077";       // Navy blue for buttons
const QString WARNING_RED = "#A73906";       // Warning indicator
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , pressure(SEA_LEVEL_PRESSURE)
    , altitude(0.0)
    , stopReading(false)
{
    try {
        initializeUI();
        initializeFilters();
        initializeSensors();

        varioSound = new VarioSound(this);
        // auto* timer = new QTimer(this);
        // double currentVario = 0.0;
        // bool ascending = true;

        // connect(timer, &QTimer::timeout, this, [=]() mutable {
        //     varioSound->updateVario(currentVario);
        //     qDebug() << QString("Vario: %1 m/s").arg(currentVario, 0, 'f', 1);

        //     if (ascending) {
        //         currentVario += 0.25;
        //         if (currentVario >= 5.0) {
        //             ascending = false;
        //         }
        //     } else {
        //         currentVario -= 0.25;
        //         if (currentVario <= -5.0) {
        //             ascending = true;
        //         }
        //     }
        // });

        // // 300ms aralıklarla güncelle
        // timer->setInterval(1000);
        // timer->start();
    }
    catch (const std::exception& e) {
        qCritical() << "Fatal error during initialization:" << e.what();
        throw;
    }
}
void MainWindow::startVarioSimulation()
{

    // Yükseliş simülasyonu (0 -> 5 m/s)
    for (double vario = 0.0; vario <= 5.0; vario += 0.1) {
        varioSound->updateVario(vario);
        QThread::msleep(100);  // Her adımda 100ms bekle
        QApplication::processEvents();  // UI'ın donmaması için event loop'u işlet
    }

    // Alçalış simülasyonu (5 -> -5 m/s)
    for (double vario = 5.0; vario >= -5.0; vario -= 0.1) {
        varioSound->updateVario(vario);
        QThread::msleep(100);
        QApplication::processEvents();
    }

    // Tekrar yükseliş (isteğe bağlı)
    for (double vario = -5.0; vario <= 0.0; vario += 0.1) {
        varioSound->updateVario(vario);
        QThread::msleep(100);
        QApplication::processEvents();
    }
}

void MainWindow::initializeUI()
{
    ui->setupUi(this);
    setWindowTitle("Professional Avionic Variometer v1.0");

    // Configure avionic-style display elements
    configureDisplayStyles();
    configureScrollBars();

    // Initialize status indicators
    ui->labelGps->setText("GPS ACTIVE");
    ui->labelSensor->setText("SENSORS READY");

    // Set initial values for variance displays
    updateVarianceDisplays();
}

void MainWindow::configureDisplayStyles()
{
    // Main display elements - Aviation style
    ui->label_vario->setStyleSheet(QString("font-size: 62pt; color: %1; background-color: %2; font-weight: bold; border: 2px solid #333;")
                                       .arg(DisplayColors::DISPLAY_NEUTRAL).arg(DisplayColors::BACKGROUND_DARK));
    ui->label_vario_unit->setStyleSheet(QString("font-size: 62pt; color: %1; background-color: %2; font-weight: bold;")
                                            .arg(DisplayColors::DISPLAY_NEUTRAL).arg(DisplayColors::BACKGROUND_DARK));

    // Secondary displays
    ui->label_pressure->setStyleSheet(QString("font-size: 42pt; color: %1; background-color: %2; font-weight: bold;")
                                          .arg(DisplayColors::TEXT_LIGHT).arg(DisplayColors::BACKGROUND_DARK));
    ui->label_baro_altitude->setStyleSheet(QString("font-size: 42pt; color: %1; background-color: %2; font-weight: bold;")
                                               .arg(DisplayColors::TEXT_LIGHT).arg(DisplayColors::BACKGROUND_DARK));

    // GPS Information displays
    ui->label_speed->setStyleSheet(QString("font-size: 42pt; color: %1; background-color: %2; font-weight: bold;")
                                       .arg(DisplayColors::DISPLAY_NEUTRAL).arg(DisplayColors::BACKGROUND_DARK));
    ui->label_gps_altitude->setStyleSheet(QString("font-size: 42pt; color: %1; background-color: %2; font-weight: bold;")
                                              .arg(DisplayColors::DISPLAY_NEUTRAL).arg(DisplayColors::BACKGROUND_DARK));

    // Status indicators
    ui->labelGps->setStyleSheet(QString("font-size: 32pt; color: %1; background-color: %2; font-weight: bold;")
                                    .arg(DisplayColors::TEXT_LIGHT).arg(DisplayColors::WARNING_RED));
    ui->labelSensor->setStyleSheet(QString("font-size: 32pt; color: %1; background-color: %2; font-weight: bold;")
                                       .arg(DisplayColors::TEXT_LIGHT).arg(DisplayColors::WARNING_RED));

    // Control elements
    ui->pushExit->setStyleSheet(QString("font-size: 24pt; font: bold; color: white; background-color: %1; border: 2px solid #666;")
                                    .arg(DisplayColors::BUTTON_BLUE));
    ui->pushReset->setStyleSheet(QString("font-size: 24pt; font: bold; color: white; background-color: %1; border: 2px solid #666;")
                                     .arg(DisplayColors::BUTTON_BLUE));

    // Status text area
    ui->m_textStatus->setStyleSheet("font-size: 18pt; color: #cccccc; background-color: #003333; border: 1px solid #666;");
}

void MainWindow::configureScrollBars()
{
    // Configure variance control scrollbars
    ui->scrollBarAccel->setRange(0, 100);
    ui->scrollBarMeasurement->setRange(0, 100);

    // Set initial scrollbar positions
    int initialAccelValue = static_cast<int>(accelVariance * 1000.0);
    int initialMeasurementValue = static_cast<int>(measurementVariance * 100.0);

    ui->scrollBarAccel->setValue(initialAccelValue);
    ui->scrollBarMeasurement->setValue(initialMeasurementValue);

    // Style scrollbars
    QString scrollBarStyle = "QScrollBar:vertical { background: #222; width: 30px; }"
                             "QScrollBar::handle:vertical { background: #666; min-height: 30px; }";
    ui->scrollBarAccel->setStyleSheet(scrollBarStyle);
    ui->scrollBarMeasurement->setStyleSheet(scrollBarStyle);
}

void MainWindow::initializeFilters()
{
    // Initialize Kalman filters with default parameters
    pressure_filter = std::make_shared<KalmanFilter>(accelVariance);
    altitude_filter = std::make_shared<KalmanFilter>(accelVariance);

    // Set initial states
    pressure_filter->Reset(SEA_LEVEL_PRESSURE);
    altitude_filter->Reset(altitude);

    // Initialize timing
    p_start = QDateTime::currentDateTime();
}

void MainWindow::initializeSensors()
{
    // Initialize sensor manager
    sensorManager = new SensorManager(this);
    connect(sensorManager, &SensorManager::sendInfo, this, &MainWindow::getSensorInfo);
    sensorManager->start();

    // Initialize GPS reader
    readGps = new ReadGps(this);
    connect(readGps, &ReadGps::sendInfo, this, &MainWindow::getGpsInfo);
}

void MainWindow::getSensorInfo(QList<qreal> info)
{
    if (stopReading || info.size() < 3) {
        return;
    }

    try {
        processSensorData(info);
    }
    catch (const std::exception& e) {
        qWarning() << "Error processing sensor data:" << e.what();
    }
}

void MainWindow::processSensorData(const QList<qreal>& info)
{
    pressure = info.at(0);
    temperature = info.at(1);
    quint64 timestamp = static_cast<quint64>(info.at(2));

    if (lastPressTimestamp > 0) {
        pDeltaT = static_cast<qreal>(timestamp - lastPressTimestamp) / 1000000.0f;

        if (pDeltaT > 0) {
            updatePressureAndAltitude(timestamp);
        }
    }
    lastPressTimestamp = timestamp;
}

void MainWindow::updatePressureAndAltitude(quint64 timestamp)
{
    p_end = QDateTime::currentDateTime();
    qint64 elapsedTimeMillis = p_start.msecsTo(p_end);

    if (pressure == 0 || elapsedTimeMillis <= 0) {
        return;
    }

    p_dt = elapsedTimeMillis / 1000.0;

    // Update pressure with Kalman filter
    pressure_filter->Update(pressure, KF_VAR_MEASUREMENT, p_dt);
    pressure = pressure_filter->GetXAbs() * 0.01; // Convert to hPa

    // Calculate and filter barometric altitude
    baroaltitude = 44330.0 * (1.0 - std::pow(pressure / SEA_LEVEL_PRESSURE_HPA, 0.19));
    altitude_filter->Update(baroaltitude, measurementVariance, p_dt);
    baroaltitude = altitude_filter->GetXAbs();

    // Calculate vertical speed
    vario = altitude_filter->GetXVel();

    updateDisplays();
    p_start = p_end;
}

void MainWindow::getGpsInfo(QList<qreal> info)
{
    if (info.size() < 4) {
        qWarning() << "Invalid GPS data received";
        return;
    }

    // Update GPS data
    altitude = info.at(0);
    latitude = info.at(1);
    longitude = info.at(2);
    groundSpeed = static_cast<int>(info.at(3));

    // Update displays - Fixed ambiguous arg() calls
    ui->label_gps_altitude->setText(QString("%1 m").arg(QString::number(altitude, 'f', 1)));
    ui->label_speed->setText(QString("%1 km/h").arg(QString::number(groundSpeed, 'f', 1)));

    // Update status display - Fixed ambiguous arg() calls
    QString gpsStatus = QString("GPS: Alt:%1m Lat:%2° Lon:%3° Speed:%4km/h")
                            .arg(QString::number(altitude, 'f', 1))
                            .arg(QString::number(latitude, 'f', 6))
                            .arg(QString::number(longitude, 'f', 6))
                            .arg(QString::number(groundSpeed, 'f', 1));
    printInfo(gpsStatus);
}

void MainWindow::updateDisplays()
{
    // Update vario display with color coding
    QString varioString = QString::number(qAbs(vario), 'f', 2);
    QString varioColor = vario < 0 ? DisplayColors::DISPLAY_NEGATIVE : DisplayColors::DISPLAY_POSITIVE;
    ui->label_vario->setStyleSheet(QString("font-size: 62pt; color: %1; background-color: %2; font-weight: bold;")
                                       .arg(varioColor)
                                       .arg(DisplayColors::BACKGROUND_DARK));
    ui->label_vario->setText(varioString);

    // Update pressure and altitude displays - Fixed ambiguous arg() calls
    ui->label_pressure->setText(QString("%1 hPa").arg(QString::number(pressure, 'f', 1)));
    ui->label_baro_altitude->setText(QString("%1 m").arg(QString::number(baroaltitude, 'f', 1)));
}

void MainWindow::on_scrollBarMeasurement_valueChanged(int value)
{
    if (value == 0) return;

    qreal mappedValue;
    if (value <= 50) {
        mappedValue = -50 + static_cast<qreal>(value);
    } else {
        mappedValue = static_cast<qreal>(value - 50);
    }

    qreal normalizedValue = mappedValue / 50.0;
    if (varioSound) {
        varioSound->updateVario(normalizedValue);
    }

    measurementVariance = static_cast<qreal>(value) / 100.0;
    updateVarianceDisplays();
}

void MainWindow::on_scrollBarAccel_valueChanged(int value)
{
    if (value == 0) return;

    stopReading = true;
    accelVariance = static_cast<qreal>(value) / 1000.0;

    // Reinitialize filters with new variance
    pressure_filter = std::make_shared<KalmanFilter>(accelVariance);
    altitude_filter = std::make_shared<KalmanFilter>(accelVariance);

    updateVarianceDisplays();
    stopReading = false;
}

void MainWindow::updateVarianceDisplays()
{
    ui->labelMV->setText(QString::number(static_cast<double>(measurementVariance), 'f', 2));
    ui->labelAV->setText(QString::number(static_cast<double>(accelVariance), 'f', 3));
}

void MainWindow::on_pushReset_clicked()
{
    stopReading = true;

    // Reset filter parameters
    accelVariance = static_cast<qreal>(KF_VAR_ACCEL);
    measurementVariance = static_cast<qreal>(KF_VAR_MEASUREMENT);

    // Reinitialize filters
    pressure_filter = std::make_shared<KalmanFilter>(accelVariance);
    altitude_filter = std::make_shared<KalmanFilter>(accelVariance);

    // Reset UI elements
    int initialAccelValue = static_cast<int>(accelVariance * 1000.0);
    int initialMeasurementValue = static_cast<int>(measurementVariance * 100.0);
    ui->scrollBarAccel->setValue(initialAccelValue);
    ui->scrollBarMeasurement->setValue(initialMeasurementValue);

    ui->m_textStatus->clear();
    updateVarianceDisplays();

    stopReading = false;
}

void MainWindow::on_pushExit_clicked()
{
    // Graceful shutdown
    if (varioSound) {
        varioSound->setStop(true);
    }
    if (sensorManager) {
        sensorManager->setStop(true);
    }

    // Allow time for threads to stop
    QThread::msleep(100);

    QCoreApplication::exit(0);
}

MainWindow::~MainWindow()
{
    // Clean shutdown of threads and resources
    if (sensorManager) {
        sensorManager->quit();
        sensorManager->wait();
        delete sensorManager;
    }

    if (varioSound) {
        delete varioSound;
    }

    if (readGps) {
        delete readGps;
    }

    delete ui;
}

void MainWindow::printInfo(QString info)
{
    ui->m_textStatus->setText(info);
}
