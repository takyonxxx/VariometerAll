#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtMath>
#include <QDebug>

// Color constants for avionic display
namespace DisplayColors {
const QString DISPLAY_POSITIVE = "#00FF3B";  // Aviation green (daha parlak yeşil)
const QString DISPLAY_NEGATIVE = "#FF1414";  // Aviation red (daha parlak kırmızı)
const QString DISPLAY_NEUTRAL = "#14FFFF";   // EFIS cyan (daha parlak cyan)
const QString BACKGROUND_DARK = "#0A0A0A";   // EFIS black (daha koyu siyah)
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

        VarioSound* varioSound = new VarioSound(this);
        varioSound->start();

        // Test with some values
        varioSound->updateVario(1.0);  // Should trigger climb tone

        // QTimer *simTimer = new QTimer(this);
        // float currentVario = 0.0f;
        // bool increasing = true;
        // p_dt = 1.0;

        // // Connect timer to lambda function that updates vario values
        // connect(simTimer, &QTimer::timeout, this, [this, &currentVario, &increasing]() {
        //     // Update vario with current value
        //     if(varioBeep)
        //         varioBeep->setVario(currentVario, p_dt);

        //     // Adjust vario value
        //     if (increasing) {
        //         currentVario += 0.1f;
        //         if (currentVario >= 5.0f) {
        //             increasing = false;
        //             currentVario = 5.0f;
        //         }
        //     } else {
        //         currentVario -= 0.5f;
        //         if (currentVario <= -3.0f) {
        //             increasing = true;
        //             currentVario = -3.0f;
        //         }
        //     }

        //     // // Format and display current vario value (optional)
        //     qDebug() << "Current vario:" << QString::number(currentVario, 'f', 1) << "m/s";
        // });

        // // Start the simulation timer (updates every 500ms)
        // simTimer->start(1000);
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

    connect(sliderMeasurement, &QSlider::valueChanged,
            this, &MainWindow::sliderMeasurement_valueChanged);
    connect(sliderAccel, &QSlider::valueChanged,
            this, &MainWindow::sliderAccel_valueChanged);
    connect(pushReset, &QPushButton::clicked,
            this, &MainWindow::pushReset_clicked);
    connect(pushExit, &QPushButton::clicked,
            this, &MainWindow::pushExit_clicked);


    // Configure avionic-style display elements
    configureDisplayStyles();
    configureScrollBars();

    // Initialize status indicators
    labelGps->setText("GPS ACTIVE");
    labelSensor->setText("SENSORS READY");

    // Set initial values for variance displays
    updateVarianceDisplays();
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

    // Create layouts with proper margins for iOS
    gridLayout_2 = new QGridLayout(centralwidget);
    gridLayout_2->setObjectName("gridLayout_2");
    gridLayout_2->setContentsMargins(10, 10, 10, 10);
    gridLayout_2->setSpacing(5);

    gridLayout = new QGridLayout();
    gridLayout->setObjectName("gridLayout");
    gridLayout->setSpacing(5);

    // Create all labels
    labelMV = new QLabel(centralwidget);
    labelMV->setObjectName("labelMV");
    labelMV->setText("0");

    labelAV = new QLabel(centralwidget);
    labelAV->setObjectName("labelAV");
    labelAV->setText("0");

    labelM = new QLabel(centralwidget);
    labelM->setObjectName("labelM");
    labelM->setText("Measure : ");

    labelA = new QLabel(centralwidget);
    labelA->setObjectName("labelA");
    labelA->setText("Accel : ");

    labelSensor = new QLabel(centralwidget);
    labelSensor->setObjectName("labelSensor");
    labelSensor->setAlignment(Qt::AlignCenter);
    labelSensor->setText("Barometer");

    label_pressure = new QLabel(centralwidget);
    label_pressure->setObjectName("label_pressure");
    label_pressure->setAlignment(Qt::AlignCenter);
    label_pressure->setText("-");

    label_baro_altitude = new QLabel(centralwidget);
    label_baro_altitude->setObjectName("label_baro_altitude");
    label_baro_altitude->setAlignment(Qt::AlignCenter);
    label_baro_altitude->setText("-");

    labelGps = new QLabel(centralwidget);
    labelGps->setObjectName("labelGps");
    labelGps->setAlignment(Qt::AlignCenter);
    labelGps->setText("Gps");

    label_gps_altitude = new QLabel(centralwidget);
    label_gps_altitude->setObjectName("label_gps_altitude");
    label_gps_altitude->setAlignment(Qt::AlignCenter);
    label_gps_altitude->setText("-");

    label_speed = new QLabel(centralwidget);
    label_speed->setObjectName("label_speed");
    label_speed->setAlignment(Qt::AlignCenter);
    label_speed->setText("-");

    label_vario = new QLabel(centralwidget);
    label_vario->setObjectName("label_vario");
    label_vario->setAlignment(Qt::AlignCenter);
    label_vario->setText("- m/s");

    // Create scroll bars
    sliderAccel = new QSlider(centralwidget);
    sliderAccel->setObjectName("sliderAccel");
    sliderAccel->setMaximum(100);
    sliderAccel->setOrientation(Qt::Horizontal);

    sliderMeasurement = new QSlider(centralwidget);
    sliderMeasurement->setObjectName("sliderMeasurement");
    sliderMeasurement->setMaximum(100);
    sliderMeasurement->setOrientation(Qt::Horizontal);

    // Create text browser with adjusted height
    m_textStatus = new QTextBrowser(centralwidget);
    m_textStatus->setObjectName("m_textStatus");
    m_textStatus->setAlignment(Qt::AlignCenter);

    // Create buttons with adjusted height
    pushReset = new QPushButton(centralwidget);
    pushReset->setObjectName("pushReset");
    pushReset->setText("Reset");

    pushExit = new QPushButton(centralwidget);
    pushExit->setObjectName("pushExit");
    pushExit->setText("Exit");

    // Add widgets to grid layout
    gridLayout->addWidget(labelMV, 10, 2, 1, 1);
    gridLayout->addWidget(sliderAccel, 11, 1, 1, 1);
    gridLayout->addWidget(sliderMeasurement, 10, 1, 1, 1);
    gridLayout->addWidget(labelAV, 11, 2, 1, 1);
    gridLayout->addWidget(labelM, 10, 0, 1, 1);
    gridLayout->addWidget(labelA, 11, 0, 1, 1);
    gridLayout->addWidget(labelSensor, 1, 0, 1, 3);
    gridLayout->addWidget(label_pressure, 3, 0, 1, 3);
    gridLayout->addWidget(label_baro_altitude, 4, 0, 1, 3);
    gridLayout->addWidget(labelGps, 5, 0, 1, 3);
    gridLayout->addWidget(label_gps_altitude, 6, 0, 1, 3);
    gridLayout->addWidget(label_speed, 9, 0, 1, 3);
    gridLayout->addWidget(m_textStatus, 12, 0, 1, 3);
    gridLayout->addWidget(label_vario, 2, 0, 1, 3);
    gridLayout->addWidget(pushReset, 13, 0, 1, 3);
    gridLayout->addWidget(pushExit, 15, 0, 1, 3);

    // Add grid layout to main layout
    gridLayout_2->addLayout(gridLayout, 0, 0, 1, 1);

    setWindowTitle("Variometer");
}

void MainWindow::configureDisplayStyles()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    int screenWidth = screen->geometry().width();

    screenWidth = screenWidth / 5;

    // Calculate font sizes based on screen width
    int primaryFontSize = screenWidth * 0.12;    // Primary displays
    int secondaryFontSize = screenWidth * 0.08;  // Secondary displays
    int statusFontSize = screenWidth * 0.08;     // Status indicators
    int buttonFontSize = screenWidth * 0.05;     // Buttons
    int scrollbarLabelFontSize = screenWidth * 0.04; // Scrollbar labels

    // Common background and border styles
    QString commonBackground = QString("background-color: %1; border: 1px solid #333333;")
                                   .arg(DisplayColors::BACKGROUND_DARK);

    // Common padding and margin - adjusted for screen size
    QString commonPadding = QString("padding: %1px; margin: %2px;")
                                .arg(screenWidth * 0.01)
                                .arg(screenWidth * 0.005);

    // Primary Display (Vario)
    QString primaryDisplayStyle = QString("font-size: %1px; color: %2; %3 %4 border-radius: 5px; font-family: 'Digital-7', 'Segment7', monospace; font-weight: bold;")
                                      .arg(primaryFontSize)
                                      .arg(DisplayColors::DISPLAY_NEUTRAL)
                                      .arg(commonBackground)
                                      .arg(commonPadding);
    label_vario->setStyleSheet(primaryDisplayStyle);

    // Secondary Displays
    QString secondaryDisplayStyle = QString("font-size: %1px; color: %2; %3 %4 border-radius: 4px; font-family: 'Digital-7', 'Segment7', monospace; font-weight: bold;")
                                        .arg(secondaryFontSize)
                                        .arg(DisplayColors::TEXT_LIGHT)
                                        .arg(commonBackground)
                                        .arg(commonPadding);

    label_pressure->setStyleSheet(secondaryDisplayStyle);
    label_baro_altitude->setStyleSheet(secondaryDisplayStyle);
    label_speed->setStyleSheet(secondaryDisplayStyle);
    label_gps_altitude->setStyleSheet(secondaryDisplayStyle);

    // Status Indicators
    QString statusIndicatorStyle = QString("font-size: %1px; color: %2; %3 %4 border-radius: 3px; font-weight: bold; text-transform: uppercase;")
                                       .arg(statusFontSize)
                                       .arg(DisplayColors::TEXT_LIGHT)
                                       .arg(QString("background-color: %1;").arg(DisplayColors::WARNING_RED))
                                       .arg(commonPadding);

    labelGps->setStyleSheet(statusIndicatorStyle);
    labelSensor->setStyleSheet(statusIndicatorStyle);

    // Control Buttons
    QString buttonStyle = QString("font-size: %1px; color: white; background-color: %2; border: 1px solid #666666; border-radius: 3px; %3 font-weight: bold;")
                              .arg(buttonFontSize)
                              .arg(DisplayColors::BUTTBLUE)
                              .arg(commonPadding);

    pushExit->setStyleSheet(buttonStyle);
    pushReset->setStyleSheet(buttonStyle);

    // Status Text Area
    QString statusTextStyle = QString("font-size: %1px; color: %2; %3 border-radius: 3px; font-family: monospace; %4")
                                  .arg(statusFontSize)
                                  .arg(DisplayColors::TEXT_LIGHT)
                                  .arg(QString("background-color: %1;").arg("#002222"))
                                  .arg(commonPadding);

    m_textStatus->setStyleSheet(statusTextStyle);

    // Scrollbar labels
    QString scrollbarLabelStyle = QString("font-size: %1px; color: %2; background-color: %3; border: 1px solid #444444; border-radius: 3px; padding: 3px 15px; margin: 2px; font-family: 'Digital-7', 'Segment7', monospace; font-weight: bold; qproperty-alignment: AlignCenter;")
                                      .arg(scrollbarLabelFontSize)
                                      .arg(DisplayColors::ADVISORY_BLUE)
                                      .arg(DisplayColors::BACKGROUND_DARK);

    labelM->setStyleSheet(scrollbarLabelStyle);
    labelMV->setStyleSheet(scrollbarLabelStyle);
    labelA->setStyleSheet(scrollbarLabelStyle);
    labelAV->setStyleSheet(scrollbarLabelStyle);

    const QString PRIMARY_COLOR = "#005999";      // Lighter marine blue
    const QString ACCENT_COLOR = "#0073BF";       // Bright marine blue
    const QString BORDER_COLOR = "#004C80";       // Mid marine blue

    QString sliderStyle =QString(
                                 "QSlider::groove:horizontal {"
                                 "    border: none;"
                                 "    height: 20px;"
                                 "    background: %1;"
                                 "    border-radius: 5px;"
                                 "    margin: 0px;"
                                 "}"
                                 "QSlider::handle:horizontal {"
                                 "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
                                 "        stop:0 %2, stop:1 %3);"
                                 "    border: none;"
                                 "    width: 30px;"
                                 "    margin: -5px 0;"
                                 "    border-radius: 10px;"
                                 "}"
                                 "QSlider::sub-page:horizontal {"
                                 "    background: %4;"
                                 "    border-radius: 5px;"
                                 "}"
                                 "QSlider::add-page:horizontal {"
                                 "    background: %5;"
                                 "    border-radius: 5px;"
                                 "}"
                                 "QSlider::tick-mark {"
                                 "    background: %6;"
                                 "    width: 2px;"
                                 "    height: 5px;"
                                 "    margin-top: 5px;"
                                 "}"
                                 ).arg(
                                     "#001F33",              // Groove background
                                     ACCENT_COLOR,           // Handle gradient start
                                     PRIMARY_COLOR,          // Handle gradient end
                                     PRIMARY_COLOR,          // Sub-page (filled)
                                     "#001F33",              // Add-page (empty)
                                     BORDER_COLOR            // Tick marks
                                  );

    sliderAccel->setStyleSheet(sliderStyle);
    sliderMeasurement->setStyleSheet(sliderStyle);
}

void MainWindow::updateVarianceDisplays()
{
    // Değerleri formatla (sabit genişlikte gösterim için)
    QString mvText = QString("%1").arg(QString::number(static_cast<double>(measurementVariance), 'f', 2), 6, ' ');
    QString avText = QString("%1").arg(QString::number(static_cast<double>(accelVariance), 'f', 3), 7, ' ');

    // Değerlere göre renk değişimi
    QString valueStyle = QString("font-size: 18pt; "
                                 "color: %1; "
                                 "background-color: %2; "
                                 "border: 1px solid #444444; "
                                 "border-radius: 3px; "
                                 "padding: 3px 15px; "
                                 "margin: 2px; "
                                 "font-family: 'Digital-7', 'Segment7', monospace; "
                                 "font-weight: bold; "
                                 "qproperty-alignment: AlignCenter;");

    QString mvStyle = valueStyle.arg(
                                    measurementVariance > 0.5 ? DisplayColors::CAUTIAMBER : DisplayColors::STATUS_GREEN
                                    ).arg(DisplayColors::BACKGROUND_DARK);

    QString avStyle = valueStyle.arg(
                                    accelVariance > 0.05 ? DisplayColors::CAUTIAMBER : DisplayColors::STATUS_GREEN
                                    ).arg(DisplayColors::BACKGROUND_DARK);

    labelMV->setText(mvText);
    labelAV->setText(avText);
    labelMV->setStyleSheet(mvStyle);
    labelAV->setStyleSheet(avStyle);
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
                             .arg(DisplayColors::BACKGROUND_DARK);

    label_vario->setStyleSheet(varioStyle);
    label_vario->setText(varioString);

    // Format numbers with consistent decimal places
    label_pressure->setText(QString("%1 hPa").arg(QString::number(pressure, 'f', 1)));
    label_baro_altitude->setText(QString("%1 m").arg(QString::number(baroaltitude, 'f', 0)));
}

void MainWindow::configureScrollBars()
{
    // Configure variance control scrollbars
    sliderAccel->setRange(0, 100);
    sliderMeasurement->setRange(0, 100);

    // Set initial scrollbar positions
    int initialAccelValue = static_cast<int>(accelVariance * 1000.0);
    int initialMeasurementValue = static_cast<int>(measurementVariance * 100.0);

    sliderAccel->setValue(initialAccelValue);
    sliderMeasurement->setValue(initialMeasurementValue);

    // Style scrollbars
    QString scrollBarStyle = "QScrollBar:vertical { background: #222; width: 30px; }"
                             "QScrollBar::handle:vertical { background: #666; min-height: 30px; }";
    sliderAccel->setStyleSheet(scrollBarStyle);
    sliderMeasurement->setStyleSheet(scrollBarStyle);
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
    connect(sensorManager, &SensorManager::sendInfo, this, &MainWindow::getSensorInfo);
    sensorManager->start();
#ifdef Q_OS_ANDROID
    readGps = new ReadGps(this);
    connect(readGps, &ReadGps::sendInfo, this, &MainWindow::getGpsInfo);
#endif
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
    latitude = info.at(1);
    longitude = info.at(2);
    groundSpeed = static_cast<int>(info.at(3));

    // Update displays - Fixed ambiguous arg() calls
    label_gps_altitude->setText(QString("%1 m").arg(QString::number(altitude, 'f', 1)));
    label_speed->setText(QString("%1 km/h").arg(QString::number(groundSpeed, 'f', 1)));

    // Update status display - Fixed ambiguous arg() calls
    QString gpsStatus = QString("%1° - %2°")
                            .arg(QString::number(latitude, 'f', 6))
                            .arg(QString::number(longitude, 'f', 6));
    printInfo(gpsStatus);
}

void MainWindow::sliderMeasurement_valueChanged(int value)
{
    if (value == 0) return;

    measurementVariance = static_cast<qreal>(value) / 100.0;
    updateVarianceDisplays();
}

void MainWindow::sliderAccel_valueChanged(int value)
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

void MainWindow::pushReset_clicked()
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
    sliderAccel->setValue(initialAccelValue);
    sliderMeasurement->setValue(initialMeasurementValue);

    m_textStatus->clear();
    updateVarianceDisplays();

    stopReading = false;
}

void MainWindow::pushExit_clicked()
{
    // if (sensorManager) {
    //     sensorManager->setStop();
    //     sensorManager->quit();
    //     sensorManager->wait();
    //     delete sensorManager;
    // }

    // if (readGps) {
    //     delete readGps;
    // }

    // if (varioSound) {
    //     varioSound->setStop();
    //     varioSound->quit();
    //     varioSound->wait();
    //     delete varioSound;
    // }

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
    m_textStatus->setText(info);
}
