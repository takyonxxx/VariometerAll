#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt Framework includes
#include <QMainWindow>
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

// Constants for atmospheric calculations
#define SEA_LEVEL_PRESSURE 101325.0f        // Standard sea level pressure in Pascals
#define SEA_LEVEL_PRESSURE_HPA 1013.25f     // Standard sea level pressure in hPa
#define KF_VAR_ACCEL 0.024f                 // Default Kalman filter acceleration variance
#define KF_VAR_MEASUREMENT 0.5f             // Default Kalman filter measurement variance

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

    void on_pushExit_clicked();
    void on_scrollBarMeasurement_valueChanged(int value);
    void on_scrollBarAccel_valueChanged(int value);
    void on_pushReset_clicked();

private:
    void initializeUI();
    void configureDisplayStyles();
    void configureScrollBars();
    void initializeFilters();
    void initializeSensors();
    void processSensorData(const QList<qreal>& info);
    void updatePressureAndAltitude(quint64 timestamp);
    void updateDisplays();
    void updateVarianceDisplays();
    void getSensorInfo(QList<qreal> info);
    void getGpsInfo(QList<qreal> info);
    void printInfo(QString info);
#ifdef Q_OS_ANDROID
    void requestAndroidPermissions();
#endif

    // Device managers
    SensorManager* sensorManager{nullptr};    // Pressure and temperature sensor manager
    ReadGps* readGps{nullptr};               // GPS data manager
    VarioSound* varioSound{nullptr};         // Audio feedback manager

    // Kalman filters
    std::shared_ptr<KalmanFilter> pressure_filter;    // Filter for pressure readings
    std::shared_ptr<KalmanFilter> altitude_filter;    // Filter for altitude calculations

    // Timing variables
    qreal p_dt{0.0};                        // Time delta for calculations
    quint64 lastPressTimestamp{0};          // Timestamp of last pressure reading
    qreal pDeltaT{0.0};                     // Time delta between pressure readings
    QDateTime p_start;                      // Start time for calculations
    QDateTime p_end;                        // End time for calculations

    // Kalman filter parameters
    qreal accelVariance{KF_VAR_ACCEL};             // Acceleration variance
    qreal measurementVariance{KF_VAR_MEASUREMENT}; // Measurement variance

    // GPS data
    qreal latitude{0.0};                    // Current latitude in degrees
    qreal longitude{0.0};                   // Current longitude in degrees
    int groundSpeed{0};                     // Ground speed in km/h

    // Sensor data
    qreal pressure{SEA_LEVEL_PRESSURE};     // Current pressure in Pa
    qreal temperature{0.0};                 // Current temperature in Celsius
    qreal altitude{0.0};                    // GPS altitude in meters
    qreal baroaltitude{0.0};               // Barometric altitude in meters
    qreal vario{0.0};                      // Vertical speed in m/s

    // Control flags
    bool stopReading{false};                // Flag to control sensor reading

    // UI
    Ui::MainWindow* ui;                     // User interface pointer
};

#endif // MAINWINDOW_H
