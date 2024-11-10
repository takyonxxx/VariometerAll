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

        varioSound = new VarioSound(this);      
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

    connect(scrollBarMeasurement, &QScrollBar::valueChanged,
            this, &MainWindow::scrollBarMeasurement_valueChanged);
    connect(scrollBarAccel, &QScrollBar::valueChanged,
            this, &MainWindow::scrollBarAccel_valueChanged);
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

    resize(352, 511);

    // Create central widget
    centralwidget = new QWidget(this);
    centralwidget->setObjectName("centralwidget");
    setCentralWidget(centralwidget);

    // Create layouts
    gridLayout_2 = new QGridLayout(centralwidget);
    gridLayout_2->setObjectName("gridLayout_2");

    gridLayout = new QGridLayout();
    gridLayout->setObjectName("gridLayout");

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
    label_vario->setText("-");

    label_vario_unit = new QLabel(centralwidget);
    label_vario_unit->setObjectName("label_vario_unit");
    label_vario_unit->setText(" m/s ");

    scrollBarAccel = new QScrollBar(centralwidget);
    scrollBarAccel->setObjectName("scrollBarAccel");
    scrollBarAccel->setMinimumSize(QSize(0, 0));
    scrollBarAccel->setMaximum(100);
    scrollBarAccel->setOrientation(Qt::Horizontal);

    scrollBarMeasurement = new QScrollBar(centralwidget);
    scrollBarMeasurement->setObjectName("scrollBarMeasurement");
    scrollBarMeasurement->setMinimumSize(QSize(0, 0));
    scrollBarMeasurement->setMaximum(100);
    scrollBarMeasurement->setOrientation(Qt::Horizontal);

    // Create text browser
    m_textStatus = new QTextBrowser(centralwidget);
    m_textStatus->setObjectName("m_textStatus");
    m_textStatus->setMaximumSize(QSize(16777215, 35));

    // Create buttons
    pushReset = new QPushButton(centralwidget);
    pushReset->setObjectName("pushReset");
    pushReset->setText("Reset");

    pushExit = new QPushButton(centralwidget);
    pushExit->setObjectName("pushExit");
    pushExit->setText("Exit");

    // Add widgets to grid layout
    gridLayout->addWidget(labelMV, 10, 2, 1, 1);
    gridLayout->addWidget(scrollBarAccel, 11, 1, 1, 1);
    gridLayout->addWidget(scrollBarMeasurement, 10, 1, 1, 1);
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
    gridLayout->addWidget(label_vario, 2, 0, 1, 2);
    gridLayout->addWidget(label_vario_unit, 2, 2, 1, 1);
    gridLayout->addWidget(pushReset, 13, 0, 1, 3);
    gridLayout->addWidget(pushExit, 15, 0, 1, 3);

    // Add grid layout to main layout
    gridLayout_2->addLayout(gridLayout, 0, 0, 1, 1);

    // Set window title
    setWindowTitle("Variometer");
}

void MainWindow::configureDisplayStyles()
{
    // Common background and border styles
    QString commonBackground = QString("background-color: %1; border: 1px solid #333333;")
                                   .arg(DisplayColors::BACKGROUND_DARK);

    // Common padding and margin - reduced
    QString commonPadding = "padding: 5px; margin: 2px;";

    // Primary Display (Vario) - Reduced size
    QString primaryDisplayStyle = QString("font-size: 48pt; color: %1; %2 %3 border-radius: 5px; font-family: 'Digital-7', 'Segment7', monospace; font-weight: bold;")
                                      .arg(DisplayColors::DISPLAY_NEUTRAL)
                                      .arg(commonBackground)
                                      .arg(commonPadding);
    label_vario->setStyleSheet(primaryDisplayStyle);
    label_vario_unit->setStyleSheet(primaryDisplayStyle);

    // Secondary Displays - Smaller size
    QString secondaryDisplayStyle = QString("font-size: 32pt; color: %1; %2 %3 border-radius: 4px; font-family: 'Digital-7', 'Segment7', monospace; font-weight: bold;")
                                        .arg(DisplayColors::TEXT_LIGHT)
                                        .arg(commonBackground)
                                        .arg(commonPadding);

    label_pressure->setStyleSheet(secondaryDisplayStyle);
    label_baro_altitude->setStyleSheet(secondaryDisplayStyle);
    label_speed->setStyleSheet(secondaryDisplayStyle);
    label_gps_altitude->setStyleSheet(secondaryDisplayStyle);

    // Status Indicators - Smaller size
    QString statusIndicatorStyle = QString("font-size: 24pt; color: %1; %2 %3 border-radius: 3px; "
                                           "font-weight: bold; text-transform: uppercase;")
                                       .arg(DisplayColors::TEXT_LIGHT)
                                       .arg(QString("background-color: %1;").arg(DisplayColors::WARNING_RED))
                                       .arg(commonPadding);

    labelGps->setStyleSheet(statusIndicatorStyle);
    labelSensor->setStyleSheet(statusIndicatorStyle);

    // Control Buttons - Compact size
    QString buttonStyle = QString("font-size: 20pt; color: white; background-color: %1; "
                                  "border: 1px solid #666666; border-radius: 3px; %2 "
                                  "font-weight: bold; min-height: 40px;")
                              .arg(DisplayColors::BUTTBLUE)
                              .arg(commonPadding);

    pushExit->setStyleSheet(buttonStyle);
    pushReset->setStyleSheet(buttonStyle);

    // Status Text Area - Smaller font
    QString statusTextStyle = QString("font-size: 16pt; color: %1; %2 "
                                      "border-radius: 3px; font-family: monospace; %3")
                                  .arg(DisplayColors::TEXT_LIGHT)
                                  .arg(QString("background-color: %1;").arg("#002222"))
                                  .arg(commonPadding);

    m_textStatus->setStyleSheet(statusTextStyle);

    // Scrollbar labels için özel stil - genişletilmiş boyut
    QString scrollbarLabelStyle = QString("font-size: 18pt; color: %1; "
                                          "background-color: %2; "
                                          "border: 1px solid #444444; "
                                          "border-radius: 3px; "
                                          "padding: 3px 15px; "  // yatay padding arttırıldı
                                          "margin: 2px; "
                                          "font-family: 'Digital-7', 'Segment7', monospace; "
                                          "font-weight: bold; "
                                          "qproperty-alignment: AlignCenter;")
                                      .arg(DisplayColors::ADVISORY_BLUE)
                                      .arg(DisplayColors::BACKGROUND_DARK);

    // Label boyutlarını ayarla
    QFont labelFont("Digital-7", 16);  // font boyutu 18pt
    labelM->setFont(labelFont);
    labelMV->setFont(labelFont);
    labelA->setFont(labelFont);
    labelAV->setFont(labelFont);

    labelM->setMinimumWidth(120);
    labelMV->setMinimumWidth(120);
    labelA->setMinimumWidth(120);
    labelAV->setMinimumWidth(120);

    labelM->setStyleSheet(scrollbarLabelStyle);
    labelMV->setStyleSheet(scrollbarLabelStyle);
    labelA->setStyleSheet(scrollbarLabelStyle);
    labelAV->setStyleSheet(scrollbarLabelStyle);

    // Enhanced scrollbar styling - düzeltilmiş renkler
    QString scrollBarStyle = "QScrollBar:vertical {"
                             "    background-color: #1A1A1A;"
                             "    width: 50px;"  // genişlik arttırıldı
                             "    margin: 3px;"  // margin eklendi
                             "    border: 2px solid #444444;"  // border kalınlaştırıldı
                             "    border-radius: 4px;"
                             "}"
                             "QScrollBar::handle:vertical {"
                             "    background-color: #0066FF;"  // ana mavi renk
                             "    min-height: 50px;"  // minimum yükseklik arttırıldı
                             "    margin: 3px;"
                             "    border: 2px solid #0044CC;"  // border eklendi
                             "    border-radius: 4px;"
                             "}"
                             "QScrollBar::handle:vertical:hover {"
                             "    background-color: #0077FF;"  // hover rengi
                             "    border: 2px solid #0055DD;"
                             "}"
                             "QScrollBar::handle:vertical:pressed {"
                             "    background-color: #0044CC;"  // basılı tutma rengi
                             "}"
                             "QScrollBar::add-line:vertical {"
                             "    height: 0px;"
                             "    border: none;"
                             "}"
                             "QScrollBar::sub-line:vertical {"
                             "    height: 0px;"
                             "    border: none;"
                             "}"
                             "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
                             "    background: #151515;"  // boş alan rengi
                             "    border-radius: 4px;"
                             "}";


    scrollBarAccel->setMinimumWidth(120);        // minimum genişlik
    scrollBarMeasurement->setMinimumWidth(120);  // minimum genişlik

    scrollBarAccel->setStyleSheet(scrollBarStyle);
    scrollBarMeasurement->setStyleSheet(scrollBarStyle);
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
    QString varioString = QString::number(qAbs(vario), 'f', 2);
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
    scrollBarAccel->setRange(0, 100);
    scrollBarMeasurement->setRange(0, 100);

    // Set initial scrollbar positions
    int initialAccelValue = static_cast<int>(accelVariance * 1000.0);
    int initialMeasurementValue = static_cast<int>(measurementVariance * 100.0);

    scrollBarAccel->setValue(initialAccelValue);
    scrollBarMeasurement->setValue(initialMeasurementValue);

    // Style scrollbars
    QString scrollBarStyle = "QScrollBar:vertical { background: #222; width: 30px; }"
                             "QScrollBar::handle:vertical { background: #666; min-height: 30px; }";
    scrollBarAccel->setStyleSheet(scrollBarStyle);
    scrollBarMeasurement->setStyleSheet(scrollBarStyle);
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
    // readGps = new ReadGps(this);
    // connect(readGps, &ReadGps::sendInfo, this, &MainWindow::getGpsInfo);
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
    label_gps_altitude->setText(QString("%1 m").arg(QString::number(altitude, 'f', 1)));
    label_speed->setText(QString("%1 km/h").arg(QString::number(groundSpeed, 'f', 1)));

    // Update status display - Fixed ambiguous arg() calls
    QString gpsStatus = QString("GPS: Alt:%1m Lat:%2° Lon:%3° Speed:%4km/h")
                            .arg(QString::number(altitude, 'f', 1))
                            .arg(QString::number(latitude, 'f', 6))
                            .arg(QString::number(longitude, 'f', 6))
                            .arg(QString::number(groundSpeed, 'f', 1));
    printInfo(gpsStatus);
}

void MainWindow::scrollBarMeasurement_valueChanged(int value)
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

void MainWindow::scrollBarAccel_valueChanged(int value)
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
    scrollBarAccel->setValue(initialAccelValue);
    scrollBarMeasurement->setValue(initialMeasurementValue);

    m_textStatus->clear();
    updateVarianceDisplays();

    stopReading = false;
}

void MainWindow::pushExit_clicked()
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
    m_textStatus->setText(info);
}
