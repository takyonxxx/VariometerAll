#ifndef UTILS_H
#define UTILS_H
#include <QObject>

#define KF_VAR_ACCEL 0.0075 // Variance of pressure acceleration noise input.
#define KF_VAR_MEASUREMENT 0.05 // Variance of pressure measurement noise.
//#define KF_VAR_ACCEL 0.005 // Smaller variance for pressure acceleration noise input.
//#define KF_VAR_MEASUREMENT 0.02 // Smaller variance for pressure measurement noise.

#define SEA_LEVEL_PRESSURE 101325.0 // Pressure at sea level (Pa)
#define SEA_LEVEL_PRESSURE_HPA 1013.25 // Pressure at sea level (hPa)

#define SCREEN_ORIENTATION_LANDSCAPE 0
#define SCREEN_ORIENTATION_PORTRAIT 1
#define RADIANS_TO_DEGREES 57.2957795

QString listToString(const QList<qreal>& values) {
    QString result;
    for (int i = 0; i < values.size(); ++i) {
        result += QString::number(values[i]);

        if (i < values.size() - 1) {
            // Add a comma and space if it's not the last value
            result += ", ";
        }
    }
    return result;
}
#endif // UTILS_H
