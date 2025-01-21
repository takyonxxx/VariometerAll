#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt Framework includes
#include <QMainWindow>
#include <QScreen>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTextBrowser>
#include <QDateTime>
#include <QString>
#include <QList>
#include <QThread>
#include <QDebug>
#include <QtMath>
#include <memory>

// Custom component includes
#include "sensormanager.h"
#include "readgps.h"
#include "kalmanfilter.h"
#include "variosound.h"
#include "variowidget.h"

// Constants for atmospheric calculations
#define SEA_LEVEL_PRESSURE 101325.0f        // Standard sea level pressure in Pascals
#define SEA_LEVEL_PRESSURE_HPA 1013.25f     // Standard sea level pressure in hPa

#define KF_VAR_ACCEL 0.75f                  // Process noise (acceleration variance)
#define KF_VAR_MEASUREMENT 0.25f           // Measurement noise variance

// Display color constants
namespace DisplayColors {
extern const QString DISPLAY_POSITIVE;   // Green for positive values
extern const QString DISPLAY_NEGATIVE;   // Red for negative values
extern const QString DISPLAY_NEUTRAL;    // Cyan for neutral values
extern const QString BACKGROUND_DARK;    // Dark background
extern const QString TEXT_LIGHT;         // Light text
extern const QString BUTTON_BLUE;        // Navy blue for buttons
extern const QString WARNING_RED;        // Warning indicator
}

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void handleExit();
    void getGpsInfo(QList<qreal> info);
    void getPressureInfo(QList<qreal> info);
    void getAccInfo(QList<qreal> info);
    void getGyroInfo(QList<qreal> info);
    void getCompassInfo(QList<qreal> info);

private:
    void initializeUI();
    void setupStyles();
    void initializeFilters();
    void initializeSensors();
    void processPressureData(const QList<qreal>& info);
    void updatePressureAndAltitude();
    void updateDisplays();
    void smoothPressure(qreal newPressure);

    void printInfo(QString info);
#ifdef Q_OS_ANDROID
    void requestAndroidPermissions();
    void keepScreenOn();
    void setupAndroidFlags();
#endif

    void setupUi();

    QWidget *centralwidget;
    QGridLayout *gridLayout_2;
    QGridLayout *gridLayout;
    QLabel *label_pressure;
    QLabel *label_altitude;
    QLabel *label_speed;
    QLabel *label_vario;
    QPushButton *pushExit;

    VarioWidget *varioWidget{nullptr};

    // Device managers
    SensorManager* sensorManager{nullptr};   // Pressure and temperature sensor manager
    ReadGps* readGps{nullptr};               // GPS data manager    
    VarioSound* varioSound{nullptr};         // Audio feedback manager

    // Kalman filters
    std::shared_ptr<KalmanFilter> pressure_filter;    // Filter for pressure readings
    std::shared_ptr<KalmanFilter> altitude_filter;    // Filter for altitude calculations

    // Timing variables
    qreal p_dt{0.0};                        // Time delta for calculations
    quint64 lastPressTimestamp{0};          // Timestamp of last pressure reading

    QDateTime p_end;                        // End time for calculations
    QDateTime p_start;


    // Kalman filter parameters
    qreal accelVariance{0};             // Acceleration variance
    qreal measurementVariance{0}; // Measurement variance

    // GPS data
    qreal latitude{0.0};                    // Current latitude in degrees
    qreal longitude{0.0};                   // Current longitude in degrees
    int groundSpeed{0};                     // Ground speed in km/h

    // Sensor data
    qreal pressure{SEA_LEVEL_PRESSURE};     // Current pressure in Pa
    qreal temperature{0.0};                 // Current temperature in Celsius
    qreal baroaltitude{0.0};               // Barometric altitude in meters
    qreal gpsaltitude{0.0};
    qreal vario{0.0};                      // Vertical speed in m/s
    qreal m_roll = 0.0;
    qreal m_pitch = 0.0;
    qreal m_heading = 0.0;

    // Control flags
    bool stopReading{false};                // Flag to control sensor reading
    // UI
    Ui::MainWindow* ui;                     // User interface pointer
};

#endif // MAINWINDOW_H
