#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtMath>
#include <QString>
#include <QDebug>

// Color constants for avionic display
namespace DisplayColors {
const QString DISPLAY_POSITIVE = "#00FF3B";  // Aviation green (daha parlak yeşil)
const QString DISPLAY_NEGATIVE = "#FF1414";  // Aviation red (daha parlak kırmızı)
const QString DISPLAY_NEUTRAL = "#14FFFF";   // EFIS cyan (daha parlak cyan)
const QString BACKGROUND = "#0c677a";
const QString TEXT_LIGHT = "#EFFFFF";        // EFIS white (hafif cyan'lı beyaz)
const QString BUTTBLUE = "#0066FF";       // Aviation blue (daha parlak mavi)
const QString WARNING_RED = "#FF4400";       // Amber warning (turuncu-kırmızı)

// İkincil renkler (gerekirse kullanmak için)
const QString CAUTIAMBER = "#FFBF00";     // Amber (dikkat rengi)
const QString STATUS_GREEN = "#00FF44";      // Status OK yeşili
const QString ADVISORY_BLUE = "#00FFFF";     // Advisory mavi
const QString TERRAIN_BROWN = "#CD661D";     // Terrain uyarı rengi
}

#ifdef Q_OS_IOS
#include "LocationPermission.h"
#endif

#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QJniObject>
#include <QJniEnvironment>
#include <QtCore/qnativeinterface.h>

void MainWindow::requestAndroidPermissions()
{
    const QStringList permissions{
        "android.permission.ACCESS_FINE_LOCATION",
        "android.permission.ACCESS_COARSE_LOCATION",
        "android.permission.BODY_SENSORS"
    };

    QJniEnvironment env;
    QJniObject activity = QNativeInterface::QAndroidApplication::context();

    for(const QString &permission : permissions) {
        QJniObject jniPermission = QJniObject::fromString(permission);
        auto result = activity.callMethod<jint>("checkSelfPermission",
                                                "(Ljava/lang/String;)I",
                                                jniPermission.object());

        if (result != 0) { // PackageManager.PERMISSIGRANTED = 0
            QJniObject::callStaticMethod<void>("androidx/core/app/ActivityCompat",
                                               "requestPermissions",
                                               "(Landroid/app/Activity;[Ljava/lang/String;I)V",
                                               activity.object(),
                                               env->NewObjectArray(1, env->FindClass("java/lang/String"), jniPermission.object()),
                                               1);
        }
    }
}
#endif


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)    
    , pressure(SEA_LEVEL_PRESSURE)
    , altitude(0.0)
    , stopReading(false)
    , ui(new Ui::MainWindow)
{
    try {
        initializeUI();

#ifdef Q_OS_ANDROID
        requestAndroidPermissions();
#endif

#ifdef Q_OS_IOS
        requestIOSLocationPermission();
#endif

        initializeFilters();
        initializeSensors();

        varioSound = new VarioSound(this);
        varioSound->start();

        /*QTimer *simTimer = new QTimer(this);
        float currentVario = 0.0f;
        bool increasing = true;

        connect(simTimer, &QTimer::timeout, this, [this, &currentVario, &increasing]() {
            // Calculate vertical speed
            vario = currentVario;

            if(varioSound)
                varioSound->updateVario(vario);

            updateDisplays();

            // Adjust vario value
            if (increasing) {
                currentVario += 0.1f;
                if (currentVario >= 5.0f) {
                    increasing = false;
                    currentVario = 5.0f;
                }
            } else {
                currentVario -= 0.1f;
                if (currentVario <= -5.0f) {
                    increasing = true;
                    currentVario = -5.0f;
                }
            }
        });

        simTimer->start(1000);*/
    }
    catch (const std::exception& e) {
        qCritical() << "Fatal error during initialization:" << e.what();
        throw;
    }
}

void MainWindow::initializeUI()
{
    ui->setupUi(this);

    setupUi();

    connect(pushExit, &QPushButton::clicked,
            this, &MainWindow::pushExit_clicked);


    // Configure avionic-style display elements
    configureDisplayStyles();
}

void MainWindow::setupUi()
{
    // Set main window properties
    if (objectName().isEmpty())
        setObjectName("MainWindow");

    // Create central widget
    centralwidget = new QWidget(this);
    centralwidget->setObjectName("centralwidget");
    setCentralWidget(centralwidget);

    // Create main grid layout
    QGridLayout* gridLayout = new QGridLayout(centralwidget);
    gridLayout->setContentsMargins(5, 5, 5, 5);
    gridLayout->setSpacing(3);

    // Create and setup HSI widget
    hsiWidget = new HSICompassWidget(centralwidget);
    hsiWidget->setThickness(0.08f);
    hsiWidget->setHeadingTextOffset(0.4f);
    hsiWidget->setMinimumHeight(400);
    hsiWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Create labels with smaller font
    label_vario = new QLabel(centralwidget);
    label_vario->setObjectName("label_vario");
    label_vario->setAlignment(Qt::AlignCenter);
    label_vario->setText("- m/s");

    label_pressure = new QLabel(centralwidget);
    label_pressure->setObjectName("label_pressure");
    label_pressure->setAlignment(Qt::AlignCenter);
    label_pressure->setText("-");

    label_altitude = new QLabel(centralwidget);
    label_altitude->setObjectName("label_baro_altitude");
    label_altitude->setAlignment(Qt::AlignCenter);
    label_altitude->setText("-");

    label_speed = new QLabel(centralwidget);
    label_speed->setObjectName("label_speed");
    label_speed->setAlignment(Qt::AlignCenter);
    label_speed->setText("-");

    pushExit = new QPushButton(centralwidget);
    pushExit->setObjectName("pushExit");
    pushExit->setText("Exit");
    pushExit->setFixedHeight(35);

    // Add widgets to grid layout
    int row = 0;

    // HSI Widget at top
    gridLayout->addWidget(hsiWidget, row++, 0, 1, 3);

    // Add the remaining widgets in order
    gridLayout->addWidget(label_vario, row++, 0, 1, 3);    
    gridLayout->addWidget(label_altitude, row++, 0, 1, 3);
    gridLayout->addWidget(label_speed, row++, 0, 1, 3);
    gridLayout->addWidget(label_pressure, row++, 0, 1, 3);
    gridLayout->addWidget(pushExit, row++, 0, 1, 3);

    // Set stretch factors
    gridLayout->setRowStretch(0, 2);  // HSI gets more vertical space
    gridLayout->setRowStretch(row, 1); // Add some stretch at the bottom

    setWindowTitle("Variometer");
}

void MainWindow::configureDisplayStyles()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    int screenWidth = screen->geometry().width();

#ifdef Q_OS_WIN32
    screenWidth = screenWidth / 5;
#endif

    // Calculate font sizes based on screen width
    int primaryFontSize = screenWidth * 0.1;    // Primary displays

    // Common background and border styles
    QString commonBackground = QString("background-color: %1; border: 1px solid #333333;")
                                   .arg(DisplayColors::BACKGROUND);

    // Common padding and margin - adjusted for screen size
    QString commonPadding = QString("padding: %1px; margin: %2px;")
                                .arg(screenWidth * 0.01)
                                .arg(screenWidth * 0.005);

    // Primary Display (Vario)
    QString primaryStyle = QString("font-size: %1px; color: %2; %3 %4 border-radius: 5px; font-family: 'Digital-7', 'Segment7', monospace; font-weight: bold;")
                                      .arg(primaryFontSize)
                                      .arg(DisplayColors::DISPLAY_NEUTRAL)
                                      .arg(commonBackground)
                                      .arg(commonPadding);

    label_vario->setStyleSheet(primaryStyle);
    label_pressure->setStyleSheet(primaryStyle);
    label_altitude->setStyleSheet(primaryStyle);
    label_speed->setStyleSheet(primaryStyle);
    pushExit->setStyleSheet(primaryStyle);
    pushExit->setMinimumHeight(50);
}

void MainWindow::updateDisplays()
{
    // Update vario display with color coding
    QString varioString = QString::number(qAbs(vario), 'f', 2) + " m/s";
    QString varioColor = vario < 0 ? DisplayColors::DISPLAY_NEGATIVE : DisplayColors::DISPLAY_POSITIVE;

    QString varioStyle = QString("font-size: 48pt; color: %1; background-color: %2; "
                                 "font-weight: bold; border: 1px solid #333333; "
                                 "padding: 5px; margin: 2px; border-radius: 5px; "
                                 "font-family: 'Digital-7', 'Segment7', monospace;")
                             .arg(varioColor)
                             .arg(DisplayColors::BACKGROUND);

    label_vario->setStyleSheet(varioStyle);
    label_vario->setText(varioString);

    // Format numbers with consistent decimal places
    label_pressure->setText(QString("%1 hPa").arg(QString::number(pressure, 'f', 1)));
    label_altitude->setText(QString("%1 m").arg(QString::number(baroaltitude, 'f', 0)));
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
    p_end = p_start;
}

void MainWindow::initializeSensors()
{
    // Initialize sensor manager
    sensorManager = new SensorManager(this);
    connect(sensorManager, &SensorManager::sendPressureInfo, this, &MainWindow::getPressureInfo);
    connect(sensorManager, &SensorManager::sendAccInfo, this, &MainWindow::getAccInfo);
    connect(sensorManager, &SensorManager::sendGyroInfo, this, &MainWindow::getGyroInfo);
    connect(sensorManager, &SensorManager::sendCompassInfo, this, &MainWindow::getCompassInfo);
    sensorManager->start();
#ifdef Q_OS_ANDROID
    readGps = new ReadGps(this);
    connect(readGps, &ReadGps::sendInfo, this, &MainWindow::getGpsInfo);
#endif
}

void MainWindow::processPressureData(const QList<qreal>& info)
{    
    pressure = info.at(0);
    temperature = info.at(1);
    quint64 timestamp = static_cast<quint64>(info.at(2));

    if (lastPressTimestamp > 0) {
        pDeltaT = static_cast<qreal>(timestamp - lastPressTimestamp) / 1000000.0f;

        if (pDeltaT > 0) {
            updatePressureAndAltitude();
        }
    }
    lastPressTimestamp = timestamp;
}

void MainWindow::updatePressureAndAltitude()
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
    hsiWidget->setVerticalSpeed(vario);

    if(varioSound)
        varioSound->updateVario(vario);

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
    m_heading = info.at(1);
    latitude = info.at(2);
    longitude = info.at(3);
    groundSpeed = static_cast<int>(info.at(4));


    hsiWidget->setHeading(m_heading);

    // Update displays - Fixed ambiguous arg() calls
    //label_altitude->setText(QString("%1 m").arg(QString::number(altitude, 'f', 1)));
    label_speed->setText(QString("%1 km/h").arg(QString::number(groundSpeed, 'f', 1)));

    // Update status display - Fixed ambiguous arg() calls
    QString gpsStatus = QString("%1° - %2°")
                            .arg(QString::number(latitude, 'f', 6))
                            .arg(QString::number(longitude, 'f', 6));
    printInfo(gpsStatus);
}

void MainWindow::getPressureInfo(QList<qreal> info)
{
    if (stopReading || info.size() < 3) {
        return;
    }

    try {
        processPressureData(info);
    }
    catch (const std::exception& e) {
        qWarning() << "Error processing sensor data:" << e.what();
    }
}

void MainWindow::getAccInfo(QList<qreal> info)
{
    if (stopReading || info.size() < 2) {
        return;
    }

    m_roll = info.at(0);
    m_pitch = info.at(1) - 45;

    hsiWidget->setPitch(m_pitch);
    hsiWidget->setRoll(m_roll);
}

void MainWindow::getGyroInfo(QList<qreal> info)
{
    if (stopReading || info.size() < 3) {
        return;
    }

}

void MainWindow::getCompassInfo(QList<qreal> info)
{
    if (stopReading || info.size() < 1) {
        return;
    }

    m_heading = info.at(0);
    hsiWidget->setHeading(m_heading);
}

void MainWindow::pushExit_clicked()
{
    QCoreApplication::exit(0);
}

MainWindow::~MainWindow()
{
    if (sensorManager) {
        sensorManager->setStop();
        sensorManager->quit();
        sensorManager->wait();
        delete sensorManager;
    }

    if (readGps) {
        delete readGps;
    }

    if (varioSound) {
        varioSound->setStop();
        varioSound->quit();
        varioSound->wait();
        delete varioSound;
    }

    delete ui;
}

void MainWindow::printInfo(QString info)
{

}
