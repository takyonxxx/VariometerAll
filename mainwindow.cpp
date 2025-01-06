#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtMath>
#include <QString>
#include <QDebug>

// Color constants for avionic display
namespace DisplayColors {
const QString DISPLAY_POSITIVE = "#00CED1";  // Aviation green (daha parlak yeşil)
const QString DISPLAY_NEGATIVE = "#FF1414";  // Aviation red (daha parlak kırmızı)
const QString BACKGROUND = "#022136;";
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

void MainWindow::setupAndroidFlags()
{
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    activity = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative",
        "activity",
        "()Landroid/app/Activity;");

    if (activity.isValid()) {
        QJniObject window = activity.callObjectMethod(
            "getWindow",
            "()Landroid/view/Window;");

        if (window.isValid()) {
            const int FLAG_KEEP_SCREEN_ON = 128;
            window.callMethod<void>(
                "addFlags",
                "(I)V",
                FLAG_KEEP_SCREEN_ON);
        }
    }
}

void MainWindow::keepScreenOn()
{
    // Android'de WakeLock kullanımı
    QJniEnvironment env;
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QJniObject context = activity.callObjectMethod(
        "getApplicationContext",
        "()Landroid/content/Context;");

    QJniObject powerManager = context.callObjectMethod(
        "getSystemService",
        "(Ljava/lang/String;)Ljava/lang/Object;",
        QJniObject::fromString("power").object());

    if (powerManager.isValid()) {
        const int SCREEN_BRIGHT_WAKE_LOCK = 10;
        const int ON_AFTER_RELEASE = 0x20000000;

        QJniObject wakeLock = powerManager.callObjectMethod(
            "newWakeLock",
            "(ILjava/lang/String;)Landroid/os/PowerManager$WakeLock;",
            SCREEN_BRIGHT_WAKE_LOCK | ON_AFTER_RELEASE,
            QJniObject::fromString("MyApp::WakeLock").object());

        if (wakeLock.isValid()) {
            wakeLock.callMethod<void>("acquire");
        }
    }
}

#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)    
    , pressure(SEA_LEVEL_PRESSURE)
    , accelVariance(KF_VAR_ACCEL)
    , measurementVariance(KF_VAR_MEASUREMENT)
    , gpsaltitude(0.0)
    , baroaltitude(0.0)
    , vario(0.0)
    , stopReading(false)
    , ui(new Ui::MainWindow)
{
    try {
        initializeUI();

#ifdef Q_OS_ANDROID
        requestAndroidPermissions();
        setupAndroidFlags();
        keepScreenOn();
#endif

#ifdef Q_OS_IOS
        requestIOSLocationPermission();
#endif

        initializeFilters();
        initializeSensors();

        varioSound = new VarioSound(this);
        varioSound->start();

        QTimer *simTimer = new QTimer(this);
        bool increasing = true;

        connect(simTimer, &QTimer::timeout, this, [this, &increasing]() {
            // Adjust currentVario based on direction
            if (increasing) {
                this->vario += 0.1f;
                if (this->vario >= 2.0f) {
                    increasing = false;  // Switch to decreasing
                }
            } else {
                this->vario -= 0.1f;
                if (this->vario <= 0.0f) {
                    increasing = true;  // Switch to increasing
                }
            }

            this->updateDisplays();
            this->varioSound->updateVario(this->vario);
        });

        //simTimer->start(1000);
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
    setupStyles();

    // Set window properties
    setWindowTitle("Variometer");
    setWindowFlags(Qt::FramelessWindowHint); // Optional: for fullscreen avionics
}

void MainWindow::setupUi()
{
    auto centralWidget = new QWidget(this);
    centralWidget->setStyleSheet(R"(
       background-color: #03395d;
       background-image: linear-gradient(rgba(0, 255, 0, 0.03) 1px, transparent 1px),
                        linear-gradient(90deg, rgba(0, 255, 0, 0.03) 1px, transparent 1px);
   )");
    setCentralWidget(centralWidget);

    setMenuBar(nullptr);
    setStatusBar(nullptr);

    gridLayout = new QGridLayout(centralWidget);
    gridLayout->setSpacing(10);
    gridLayout->setContentsMargins(5, 5, 5, 5);

    hsiWidget = new HSIWidget(this);
    hsiWidget->setThickness(0.08f);
    hsiWidget->setHeadingTextOffset(0.4f);
    hsiWidget->setStyleSheet("background: rgba(22, 39, 54, 0.95);");

    QScreen *screen = QGuiApplication::primaryScreen();
    int pixelWidth = screen->size().width();
    qreal dpi = screen->logicalDotsPerInch();
    hsiWidget->setMinimumHeight(400);

    label_vario = new QLabel("0.0 m/s", this);
    label_altitude = new QLabel("0.0 m", this);
    label_speed = new QLabel("0 km/s", this);
    label_pressure = new QLabel("0.0 kPa", this);

    pushExit = new QPushButton("EXIT", this);

    int row = 0;
    gridLayout->addWidget(hsiWidget, row++, 0, 1, 3);
    gridLayout->addWidget(label_vario, row++, 0, 1, 3);
    gridLayout->addWidget(label_altitude, row++, 0, 1, 3);
    gridLayout->addWidget(label_speed, row++, 0, 1, 3);
    gridLayout->addWidget(label_pressure, row++, 0, 1, 3);

    // Add a spacer for the empty space above the Exit button
    gridLayout->addItem(new QSpacerItem(0, 20, QSizePolicy::Minimum, QSizePolicy::Expanding), row++, 0, 1, 3);

    gridLayout->addWidget(pushExit, row++, 0, 1, 3);
    gridLayout->setRowStretch(0, 1);
    gridLayout->setRowStretch(row, 1);

    connect(pushExit, &QPushButton::clicked, this, &MainWindow::handleExit);
}

void MainWindow::setupStyles()
{
    // Set dark theme colors
    QString baseStyle = R"(
        QMainWindow {
            background-color: #1a1a1a;
        }
        QLabel {
            color: #FBFBF9;
            font-size: 40px;
            font-weight: bold;
            padding: 10px;
            background-color: #2d2d2d;
            border-radius: 5px;
            qproperty-alignment: AlignCenter;    /* Centers text horizontally and vertically */
        }
    )";

    // For the vario label with dynamic color
    QString varioColor = vario < 0 ? DisplayColors::DISPLAY_NEGATIVE : DisplayColors::DISPLAY_POSITIVE;
    QString varioStyle = QString("font-size: 48pt; "
                                 "color: %1; "
                                 "background-color: %2; "
                                 "font-weight: bold; "
                                 "border: 1px solid #333333; "
                                 "padding: 5px; "
                                 "margin: 2px; "
                                 "border-radius: 5px; "
                                 "font-family: 'Digital-7', 'Segment7', monospace; "
                                 "qproperty-alignment: AlignCenter;")  // Centers vario text
                             .arg(varioColor)
                             .arg(DisplayColors::BACKGROUND);

    // Apply styles
    setStyleSheet(baseStyle);
    label_vario->setStyleSheet(varioStyle);

    pushExit->setStyleSheet(R"(
       QPushButton {
           color: #FFFFFF;
           background-color: #C70039;
           font-family: 'Consolas';
           font-size: 36px;
           font-weight: bold;
           padding: 10px;
           border-radius: 5px;
       }
   )");

    // Additional label styles with centering
    QString commonLabelStyle = "background-color: #022136; qproperty-alignment: AlignCenter;";
    label_altitude->setStyleSheet(commonLabelStyle);
    label_speed->setStyleSheet(commonLabelStyle);
    label_pressure->setStyleSheet(commonLabelStyle);
}

void MainWindow::updateDisplays()
{
    // Update vario display with color coding
    QString varioString = QString::number(vario, 'f', 1) + " m/s";
    QString varioColor = vario < 0 ? DisplayColors::DISPLAY_NEGATIVE : DisplayColors::DISPLAY_POSITIVE;
    QString varioStyle = QString("font-size: 48pt; "
                                 "color: %1; "
                                 "background-color: %2; "
                                 "font-weight: bold; "
                                 "border: 1px solid #333333; "
                                 "padding: 5px; "
                                 "margin: 2px; "
                                 "border-radius: 5px; "
                                 "font-family: 'Digital-7', 'Segment7', monospace; "
                                 "qproperty-alignment: AlignCenter;")  // Centers vario text
                             .arg(varioColor)
                             .arg(DisplayColors::BACKGROUND);


    label_vario->setStyleSheet(varioStyle);
    label_vario->setText(varioString);

    // Format numbers with consistent decimal places
    label_pressure->setText(QString("%1 hPa").arg(QString::number(pressure, 'f', 1)));
    if(gpsaltitude == 0)
        label_altitude->setText(QString("%1 m").arg(QString::number(baroaltitude, 'f', 1)));
}

void MainWindow::initializeFilters()
{
    // Initialize Kalman filters with default parameters
    pressure_filter = std::make_shared<KalmanFilter>(accelVariance);
    altitude_filter = std::make_shared<KalmanFilter>(accelVariance);
    // Set initial states
    pressure_filter->Reset(SEA_LEVEL_PRESSURE);
    altitude_filter->Reset(baroaltitude);
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

    readGps = new ReadGps(this);
    connect(readGps, &ReadGps::sendInfo, this, &MainWindow::getGpsInfo);
}

void MainWindow::processPressureData(const QList<qreal>& info)
{    
    pressure = info.at(0);
    temperature = info.at(1);

    quint64 timestamp = static_cast<quint64>(info.at(2));

    if (lastPressTimestamp > 0) {
        updatePressureAndAltitude();
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
    gpsaltitude = info.at(0);
    m_heading = info.at(1);
    latitude = info.at(2);
    longitude = info.at(3);
    groundSpeed = static_cast<int>(info.at(4));


    hsiWidget->setHeading(m_heading);

    // Update displays - Fixed ambiguous arg() calls
    if(gpsaltitude != 0)
        label_altitude->setText(QString("Gps: %1 m").arg(QString::number(gpsaltitude, 'f', 0)));
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
    m_pitch = info.at(1);

    hsiWidget->setPitch(m_pitch - 90);
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

void MainWindow::handleExit()
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
        varioSound->stop();
        delete varioSound;
    }

    delete ui;
}

void MainWindow::printInfo(QString info)
{

}
