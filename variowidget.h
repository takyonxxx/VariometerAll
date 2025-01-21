#ifndef VARIOWIDGET_H
#define VARIOWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QPolygonF>

class VarioWidget : public QWidget {
    Q_OBJECT

public:
    explicit VarioWidget(QWidget* parent = nullptr)
        : QWidget(parent), m_heading(0), m_sizeRatio(1.0f), m_thickness(0.04f), m_padding(5) {
        QTimer* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, [this]() { this->update(); });
        timer->start(16); // Approximately 60 FPS
    }

    void setHeading(float heading) {
        m_heading = heading;
        update();
    }

    void setSizeRatio(float ratio) {
        m_sizeRatio = qMax(0.1f, ratio);  // Allow any positive value, but minimum 0.1
        update();
    }

    void setThickness(float thickness) {
        m_thickness = qBound(0.01f, thickness, 0.5f);  // Allow values between 0.01 and 0.5
        update();
    }

    void setHeadingTextOffset(float offset) {
        m_headingTextOffset = qBound(0.0f, offset, 0.5f);  // Limit offset between 0 and 0.5
        update();
    }

    void setVerticalSpeed(float vario) {
        m_verticalSpeed = qBound(MIN_VARIO, vario, MAX_VARIO);
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        int totalWidth = width() - m_padding;
        int totalHeight = height() - m_padding;

        // HSI size (80% of total height)
        int hsiHeight = static_cast<int>(totalHeight * 0.9);
        int hsiSize = qMin(totalWidth, hsiHeight);

        int offsetw = (width() - hsiSize) / 2;
        int offseth = (height() - hsiSize) / 2;

        // HSI rectangle centered both vertically and horizontally
        QRectF hsiRect(offsetw, offseth, hsiSize, hsiSize);
        drawCompassAndHSI(painter, hsiRect);
    }

    void resizeEvent(QResizeEvent* event) override
    {
        QWidget::resizeEvent(event);
        update(); // Trigger a repaint when the widget is resized
    }

private:
    float m_heading= 0.0f;
    float m_verticalSpeed= 0.0f;

    float m_sizeRatio;
    float m_thickness;
    float m_headingTextOffset;
    int m_padding;   

    const float MAX_VARIO = 10.0f;
    const float MIN_VARIO = -10.0f;

    void drawCompassAndHSI(QPainter& painter, const QRectF& rect)
    {
        int rectWidth = static_cast<int>(rect.width());
        int rectHeight = static_cast<int>(rect.height());

        // Use the smaller dimension to ensure the compass fits within the widget
        int availableSize = qMin(rectWidth, rectHeight);

        // Add some margin to prevent clipping
        int margin = static_cast<int>(availableSize * 0.05);  // 5% margin
        availableSize -= 2 * margin;

        int compassSize = static_cast<int>(availableSize * m_sizeRatio);
        int hsiSize = static_cast<int>(compassSize * (1 - m_thickness * 2));

        QPointF center = rect.center();
        QRectF compassRect(center.x() - compassSize / 2, center.y() - compassSize / 2, compassSize, compassSize);
        QRectF hsiRect(center.x() - hsiSize / 2, center.y() - hsiSize / 2, hsiSize, hsiSize);

        painter.save();
        painter.setClipRect(rect);  // Ensure drawing doesn't go outside the widget



        drawVariometer(painter, compassRect);
        drawCompass(painter, compassRect);

        painter.restore();
    }

    void drawCompass(QPainter& painter, const QRectF& rect)
    {
        QPointF center = rect.center();
        float outerRadius = rect.width() / 2.0f;
        float innerRadius = outerRadius * (1 - m_thickness);

        // Draw outer circle (hollow)
        painter.setPen(QPen(QColor(0, 0, 0, 255), rect.width() * m_thickness));
        painter.setBrush(Qt::NoBrush);

        // Define custom size factors
        float widthFactor = 0.95f;  // Adjust this factor as needed
        float heightFactor = 0.925f; // Adjust this factor as needed

        // Create a custom rectangle based on the original rect
        QRectF customRect(center.x() - (rect.width() * widthFactor) / 2.0f,
                          center.y() - (rect.height() * heightFactor) / 2.0f,
                          rect.width() * widthFactor,
                          rect.height() * heightFactor);

        painter.drawEllipse(customRect);

        // Draw compass ticks and triangles
        painter.save();
        painter.translate(center);
        painter.rotate(90);  // Rotate to bring 0 degrees to the top

        for (int i = 0; i < 360; i += 10) {
            if (i % 90 == 0) {
                // Draw large triangles at 0, 90, 180, 270 degrees
                painter.setPen(Qt::NoPen);
                painter.setBrush(Qt::white);
                QPolygonF triangle;
                float triangleBase = rect.width() * 0.04;
                float triangleHeight = rect.height() * 0.08;
                triangle << QPointF(-triangleBase / 2, -outerRadius)
                         << QPointF(triangleBase / 2, -outerRadius)
                         << QPointF(0, -outerRadius + triangleHeight);
                painter.drawPolygon(triangle);
            } else if (i % 30 == 0) {
                painter.setPen(QPen(Qt::white, rect.width() * 0.01));
                painter.drawLine(0, -outerRadius, 0, -innerRadius);
            } else {
                painter.setPen(QPen(Qt::lightGray, rect.width() * 0.005));
                painter.drawLine(0, -outerRadius, 0, -innerRadius * 1.05);
            }
            painter.rotate(-10);
        }
        painter.restore();

        QString headingText = QString::number(qRound(m_heading));
        QRectF digitalRect(center.x() - 60, center.y() + innerRadius * 0.55, 120, 40);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(250, 250, 250, 200));
        painter.drawRoundedRect(digitalRect, 10, 10);

        painter.setPen(Qt::yellow);
        QFont font("Tahoma", rect.width() / 10);
        font.setBold(true);
        painter.setFont(font);
        painter.drawText(digitalRect, Qt::AlignHCenter | Qt::AlignVCenter, headingText);

        // Draw heading indicator with increased height
        painter.save();
        painter.translate(center);
        painter.rotate(m_heading);
        float triangleWidth = rect.width() * 0.05;
        float triangleHeight = rect.width() * 0.08;  // Increased height
        QPolygonF triangle;
        triangle << QPointF(-triangleWidth, -outerRadius)
                 << QPointF(triangleWidth, -outerRadius)
                 << QPointF(0, -outerRadius + triangleHeight);
        painter.setBrush(Qt::red);
        painter.setPen(Qt::NoPen);
        painter.drawPolygon(triangle);
        painter.restore();
    }

    void drawVariometer(QPainter& painter, const QRectF& rect)
    {
        QPointF center = rect.center();
        float outerRadius = rect.width() / 2.0f;
        float innerRadius = outerRadius * 0.85f;

        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.save();
        painter.translate(center);

        // Draw scale background (semicircle)
        QPainterPath scalePath;
        float scaleWidth = rect.width() * 0.12f;
        scalePath.addEllipse(QRectF(-innerRadius, -innerRadius,
                                    innerRadius * 2, innerRadius * 2));
        scalePath.addEllipse(QRectF((-innerRadius + scaleWidth), (-innerRadius + scaleWidth),
                                    (innerRadius - scaleWidth) * 2, (innerRadius - scaleWidth) * 2));
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(216, 6, 104 , 150));
        painter.drawPath(scalePath);

        // Draw tick marks and numbers
        QFont markFont("Arial", rect.width() * 0.045);
        markFont.setBold(true);
        painter.setFont(markFont);

        const int numTicks = 11; // -5 to 5
        const float angleStep = 180.0f / (numTicks - 1); // Spread over 180 degrees

        for (int i = 0; i < numTicks; i++) {
            painter.save();
            float value = -5.0f + i; // Values from -5 to 5
            float angle = -90.0f + i * angleStep; // Map values to [-90, +90]
            painter.rotate(angle);

            // Draw ticks
            painter.setPen(QPen(QColor(232, 243, 245), qAbs(value) == 5.0f || value == 0 ? 3 : 2));
            float tickStart = innerRadius - scaleWidth;
            float tickLength = (qAbs(value) == 5.0f || value == 0) ? scaleWidth * 0.8f : scaleWidth * 0.4f;
            painter.drawLine(QPointF(0, -tickStart),
                             QPointF(0, -(tickStart + tickLength)));

            // Draw labels for -5.0, 0.0, and 5.0 only
            if (value == -5.0f || value == 0.0f || value == 5.0f) {
                painter.rotate(-angle);

                float textRadius = innerRadius - scaleWidth * 1.5;
                QPointF textPos(
                    textRadius * std::sin(qDegreesToRadians(angle)),
                    -textRadius * std::cos(qDegreesToRadians(angle))
                    );

                QRectF textRect(textPos.x() - 20, textPos.y() - 10, 40, 20);

                // Text background
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(250, 250, 250, 200));
                painter.drawRoundedRect(textRect, 5, 5);

                // Draw the label text
                painter.setPen(value >= 0 ? QColor(0, 150, 0) : QColor(200, 0, 0));
                QString label = QString::number(value, 'f', 1);
                painter.drawText(textRect, Qt::AlignCenter, label);

                painter.rotate(angle);
            }

            painter.restore();
        }

        // Define colors for positive and negative values
        QColor needleColor;
        QColor needleHighlight = QColor(255, 255, 150, 180); // Soft yellow highlight for a polished look
        QColor needleShadow = QColor(200, 180, 0, 100);    // Subtle shadow with a darker gold tone

        // Determine the color based on vertical speed
        if (m_verticalSpeed >= 0) {
            needleColor = QColor(0, 255, 0);  // Green for positive vertical speed (climbing)
        } else {
            needleColor = QColor(255, 0, 0);  // Red for negative vertical speed (descending)
        }

        // Draw indicator needle
        float angle = (m_verticalSpeed / 5.0f) * 90.0f; // Scale vertical speed to [-90, +90] degrees
        painter.rotate(angle);

        // Draw needle shaft (increase width for a thicker look)
        float shaftWidth = rect.width() * 0.0275f;  // Thicker shaft
        float shaftLength = innerRadius - scaleWidth * 0.8f;  // Shorten the shaft length
        QRectF needleShaft(-shaftWidth / 2, -shaftLength, shaftWidth, shaftLength);

        QLinearGradient shaftGradient(needleShaft.topLeft(), needleShaft.bottomRight());
        shaftGradient.setColorAt(0, needleHighlight);
        shaftGradient.setColorAt(0.5, needleColor);
        shaftGradient.setColorAt(1, needleShadow);

        painter.setBrush(shaftGradient);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(needleShaft, shaftWidth * 0.3, shaftWidth * 0.3);  // More rounded corners for a smooth look

        // Draw needle head (triangular tip)
        float headBaseWidth = rect.width() * 0.05f;  // Wider base for the head
        float headLength = rect.width() * 0.08f;     // Shorten the head length
        QPolygonF needleHead;
        needleHead << QPointF(0, -shaftLength - headLength)  // Tip of the needle
                   << QPointF(-headBaseWidth / 2, -shaftLength) // Left base of the triangle
                   << QPointF(headBaseWidth / 2, -shaftLength); // Right base of the triangle

        QLinearGradient headGradient(needleHead.boundingRect().topLeft(), needleHead.boundingRect().bottomRight());
        headGradient.setColorAt(0, needleHighlight);
        headGradient.setColorAt(0.5, needleColor);
        headGradient.setColorAt(1, needleShadow);

        painter.setBrush(headGradient);
        painter.drawPolygon(needleHead);

        // Restore painter state
        painter.restore();

        // Draw central hub
        QRadialGradient hubGradient(center, rect.width() * 0.08f);
        hubGradient.setColorAt(0, QColor(220, 220, 220));
        hubGradient.setColorAt(0.7, QColor(180, 180, 180));
        hubGradient.setColorAt(1, QColor(150, 150, 150));
        painter.setBrush(hubGradient);
        painter.setPen(QPen(QColor(120, 120, 120), 1));
        painter.drawEllipse(center, rect.width() * 0.06f, rect.width() * 0.06f);

        // Draw digital value display
        QFont valueFont("Arial", rect.width() * 0.12);
        valueFont.setBold(true);
        painter.setFont(valueFont);

        QString speedText = QString::number(m_verticalSpeed, 'f', 1);
        QRectF digitalRect(center.x() - 60, center.y() + innerRadius * 0.25, 120, 40);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(250, 250, 250, 200));
        painter.drawRoundedRect(digitalRect, 10, 10);

        painter.setPen(m_verticalSpeed >= 0 ? QColor(0, 150, 0) : QColor(200, 0, 0));
        painter.drawText(digitalRect, Qt::AlignCenter, speedText);
    }


};
#endif // VARIOWIDGET_H
