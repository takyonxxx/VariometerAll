#ifndef HSIWIDGET_H
#define HSIWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QPolygonF>
#include <cmath>

class HSIWidget : public QWidget {
    Q_OBJECT

public:
    explicit HSIWidget(QWidget* parent = nullptr)
        : QWidget(parent), m_roll(0), m_pitch(0), m_heading(0), m_sizeRatio(1.0f), m_thickness(0.04f), m_padding(5) {
        QTimer* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, [this]() { this->update(); });
        timer->start(16); // Approximately 60 FPS
    }

    void setRoll(float roll) {
        m_roll = roll;
        update();
    }

    void setPitch(float pitch) {
        m_pitch = pitch;
        update();
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
    float m_roll= 0.0f;
    float m_pitch= 0.0f;
    float m_heading= 0.0f;
    float m_verticalSpeed= 0.0f;

    float m_sizeRatio;
    float m_thickness;
    float m_headingTextOffset;
    int m_padding;   

    const float MAX_VARIO = 10.0f;
    const float MIN_VARIO = -10.0f;

    void drawVarioIndicator(QPainter& painter, const QRectF& rect)
    {
        painter.save();

        const float MAX_SPEED = 10.0f;  // Maximum speed in m/s
        const float WARN_SPEED = 7.0f;  // Warning threshold

        // Bar dimensions
        const float barWidth = rect.width() * 0.7f;
        const float barHeight = rect.height();
        const float startX = rect.left() + (rect.width() - barWidth) / 2;
        const float centerY = rect.center().y();  // Center point for zero

        // Tick parameters
        const float majorTickLength = barWidth * 0.5f;
        const float minorTickLength = barWidth * 0.3f;
        const float textOffset = barWidth * 0.7f;
        const float pixelPerSpeed = (barHeight/2) / MAX_SPEED;  // Half height for positive, half for negative

        // Draw background
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 128));
        painter.drawRect(rect);

        // Draw warning zones
        // Positive (climb) warning zone
        QRectF climbWarnRect(startX, centerY - (MAX_SPEED * pixelPerSpeed),
                             barWidth, ((MAX_SPEED - WARN_SPEED) * pixelPerSpeed));
        // Negative (sink) warning zone
        QRectF sinkWarnRect(startX, centerY + (WARN_SPEED * pixelPerSpeed),
                            barWidth, ((MAX_SPEED - WARN_SPEED) * pixelPerSpeed));

        // Draw safe zones
        QRectF climbSafeRect(startX, centerY - (WARN_SPEED * pixelPerSpeed),
                             barWidth, WARN_SPEED * pixelPerSpeed);
        QRectF sinkSafeRect(startX, centerY,
                            barWidth, WARN_SPEED * pixelPerSpeed);

        // Draw zones with colors
        painter.setBrush(QColor(255, 0, 0, 50));  // Red for warning zones
        painter.drawRect(climbWarnRect);
        painter.drawRect(sinkWarnRect);

        painter.setBrush(QColor(0, 255, 0, 50));  // Green for safe zones
        painter.drawRect(climbSafeRect);
        painter.drawRect(sinkSafeRect);

        // Draw scale lines and values
        QFont scaleFont = painter.font();
        scaleFont.setPixelSize(rect.width() * 0.125);
        painter.setFont(scaleFont);

        // Draw tick marks every 1 m/s, major ticks every 5 m/s
        for(int speed = -10; speed <= 10; speed++) {
            float y = centerY - (speed * pixelPerSpeed);
            bool isMajor = (speed % 5 == 0);
            float tickLength = isMajor ? majorTickLength : minorTickLength;

            // Set color based on speed
            painter.setPen(QPen(abs(speed) >= WARN_SPEED ? Qt::red : Qt::white,
                                isMajor ? 2 : 1));

            // Draw tick marks
            painter.drawLine(QPointF(startX, y),
                             QPointF(startX + tickLength, y));

            // Draw values for major ticks
            if(isMajor) {
                QString speedText = QString::number(speed);
                painter.drawText(QRectF(startX + textOffset, y - rect.width() * 0.1,
                                        rect.width() * 0.6, rect.width() * 0.2),
                                 Qt::AlignLeft | Qt::AlignVCenter,
                                 speedText);
            }
        }

        // Draw current speed indicator
        const float indicatorY = centerY - (m_verticalSpeed * pixelPerSpeed);

        // Yellow indicator line
        QPen indicatorPen(Qt::yellow, 3);
        painter.setPen(indicatorPen);
        painter.drawLine(QPointF(startX, indicatorY),
                         QPointF(startX + barWidth, indicatorY));

        // Current speed value
        QFont speedFont = painter.font();
        speedFont.setPixelSize(rect.width() * 0.35);
        speedFont.setBold(true);
        painter.setFont(speedFont);

        QString speedText = QString::number(m_verticalSpeed, 'f', 1);
        painter.drawText(QRectF(startX + 2*textOffset, indicatorY - rect.width() * 0.2,
                                rect.width(), rect.width() * 0.4),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         speedText);

        // Draw zero line
        painter.setPen(QPen(Qt::white, 2));
        painter.drawLine(QPointF(startX, centerY),
                         QPointF(startX + barWidth, centerY));

        painter.restore();
    }

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

        drawCompass(painter, compassRect);
        drawHSI(painter, hsiRect);

        painter.restore();
    }

    void drawHSI(QPainter& painter, const QRectF& rect) {
        int pitchInterval = 20;
        int pitchLineSpacing = 10;

        float outerRadius = rect.width() / 2.0f;

        QPointF center = rect.center();

        QFont font("Tahoma", rect.width() / 30, QFont::Bold);
        painter.setFont(font);

        // Save the current state of the painter
        painter.save();

        // Translate the painter to the center
        painter.translate(center);

        // Create circular clip path
        QPainterPath clipPath;
        clipPath.addEllipse(QRectF(-rect.width()/2, -rect.height()/2, rect.width(), rect.height()));
        painter.setClipPath(clipPath);

        // Apply rotation
        painter.rotate(-m_roll);

        float verticalMargin = rect.height() * 0.1f;
        float effectiveHeight = rect.height() - 2 * verticalMargin;
        float degreesPerPixel = 100.0f / effectiveHeight;

        // Calculate horizon position based on pitch
        float horizonY = m_pitch / degreesPerPixel;

        // Draw the sky (blue)
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(39, 173, 200, 200));
        painter.drawRect(QRectF(-rect.width()/2, -rect.height()/2, rect.width(), rect.height()/2 + horizonY));

        // Draw the ground (green)
        painter.setBrush(QColor(19, 196, 19, 200));
        painter.drawRect(QRectF(-rect.width()/2, horizonY, rect.width(), rect.height()/2 - horizonY));

        // Define the upper and lower limits for drawing
        float upperLimit = -rect.height() / 4.0f;
        float lowerLimit = rect.height() / 4.0f;

        // Draw pitch lines and labels
        float longLineLength = rect.width() / 4.0f;
        float shortLineLength = rect.width() / 6.0f;
        QFont labelFont("Tahoma", rect.width() / 18, QFont::Bold);

        for (int angle = -90; angle <= 90; angle += pitchLineSpacing) {
            float adjustedY = (m_pitch - angle) / degreesPerPixel + (angle % pitchLineSpacing) * rect.height() / 1000.0f;

            if (adjustedY >= upperLimit && adjustedY <= lowerLimit) {
                bool isMainInterval = (angle % pitchInterval == 0);
                float lineLength = isMainInterval ? longLineLength : shortLineLength;

                // Draw line
                painter.setPen(QPen(Qt::white, 2));
                painter.drawLine(QPointF(-lineLength / 2, adjustedY), QPointF(lineLength / 2, adjustedY));

                // Draw labels for main intervals
                if (isMainInterval) {
                    painter.setFont(labelFont);
                    painter.setPen(Qt::white);
                    QString text = QString::number(qAbs(angle));

                    // Left label
                    QRectF leftTextRect(-lineLength / 2 - rect.width() / 8.0f, adjustedY - rect.height() / 30.0f, rect.width() / 8.0f, rect.height() / 15.0f);
                    painter.drawText(leftTextRect, Qt::AlignRight | Qt::AlignVCenter, text);

                    // Right label
                    QRectF rightTextRect(lineLength / 2, adjustedY - rect.height() / 30.0f, rect.width() / 8.0f, rect.height() / 15.0f);
                    painter.drawText(rightTextRect, Qt::AlignLeft | Qt::AlignVCenter, text);
                }
            }
        }

        // Draw horizon line
        painter.setPen(QPen(Qt::white, 1));
        painter.drawLine(QPointF(-rect.width()/2, horizonY), QPointF(rect.width()/2, horizonY));

        // Draw horizontal arc
        painter.setPen(QPen(Qt::white, 2));
        QRectF arcRect(-outerRadius, -outerRadius, outerRadius * 2, outerRadius * 2);
        painter.drawArc(arcRect, 60 * 16, 60 * 16);  // 60 degrees on each side

        // Draw roll indicator
        painter.setPen(QPen(Qt::white, 3));
        for (int i = -60; i <= 60; i += 15) {
            painter.save();
            painter.rotate(i);
            if (i == 0) {
                painter.setPen(QPen(Qt::white, 2));
                painter.drawLine(QPointF(0, -outerRadius), QPointF(0, -outerRadius + rect.height() / 12.0f));
            } else {
                painter.drawLine(QPointF(0, -outerRadius), QPointF(0, -outerRadius + rect.height() / 15.0f));
            }
            painter.restore();
        }

        // Restore the painter state to remove rotation
        painter.restore();

        // Save painter state again for center lines and triangle
        painter.save();
        painter.translate(center);

        // Draw center lines and triangle
        QPen brownPen(QColor(139, 69, 19), 5);
        painter.setPen(brownPen);
        painter.setBrush(Qt::NoBrush);

        float centerLineLength = outerRadius * 0.8f;
        float centerTriangleHeight = rect.height() * 0.03f;
        float centerTriangleBase = centerTriangleHeight * 7.5f;
        float gapBetweenLineAndTriangle = rect.width() * 0.1f;

        // Left line
        painter.drawLine(QPointF(-centerLineLength, 0),
                         QPointF(-centerTriangleBase / 2 - gapBetweenLineAndTriangle, 0));

        // Right line
        painter.drawLine(QPointF(centerTriangleBase / 2 + gapBetweenLineAndTriangle, 0),
                         QPointF(centerLineLength, 0));

        // Half triangle (only top line)
        painter.drawLine(QPointF(-centerTriangleBase / 2, centerTriangleHeight),
                         QPointF(0, 0));
        painter.drawLine(QPointF(0, 0),
                         QPointF(centerTriangleBase / 2, centerTriangleHeight));

        // Restore the painter state
        painter.restore();

        // Draw north indicator
        painter.save();
        painter.translate(center);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::red);

        float triangleHeight = rect.height() * 0.2f;  // Adjust size as needed
        float triangleBase = triangleHeight / 2;
        float triangleOffset = rect.height() * 0.02f;  // Adjust offset as needed

        QPolygonF northTriangle;
        northTriangle << QPointF(0, -outerRadius + triangleHeight / 2 + triangleOffset)
                      << QPointF(-triangleBase / 2, -outerRadius + triangleOffset)
                      << QPointF(triangleBase / 2, -outerRadius + triangleOffset);

        painter.drawPolygon(northTriangle);
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
        float heightFactor = 0.95f; // Adjust this factor as needed

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

        // Draw heading text
        painter.setPen(Qt::white);
        QFont font("Tahoma", rect.width() / 16);
        font.setBold(true);
        painter.setFont(font);
        QRectF textRect = rect.adjusted(rect.width() * m_thickness, rect.height() * m_thickness, -rect.width() * m_thickness, -rect.height() * m_thickness);

        // Adjust the vertical position of the text rect
        float verticalOffset = textRect.height() * m_headingTextOffset;
        textRect.moveTop(textRect.top() + verticalOffset);

        QString headingText = QString::number(qRound(m_heading));        
        painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignVCenter, headingText);

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

    void drawHeadingCompass(QPainter& painter, const QRectF& rect) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);

        QPointF center = rect.center();
        double maxRadius = qMin(rect.width(), rect.height()) / 2;
        double sizeRatio = maxRadius / 360.0;
        double outerRadius = maxRadius - sizeRatio * 60;
        double innerRadius = maxRadius - sizeRatio * 90;
        double markingsRadius = maxRadius - sizeRatio * 135;

        // Enhanced gradient background
        QRadialGradient gradient(center, maxRadius);
        gradient.setColorAt(0, QColor(40, 40, 50));
        gradient.setColorAt(0.85, QColor(30, 30, 40));
        gradient.setColorAt(1, QColor(20, 20, 30));
        painter.setBrush(gradient);
        painter.setPen(QPen(QColor(70, 70, 90), 2));
        painter.drawEllipse(center, maxRadius, maxRadius);

        // Inner decorative circles
        painter.setPen(QPen(QColor(60, 60, 80), 1));
        painter.drawEllipse(center, outerRadius + sizeRatio * 10, outerRadius + sizeRatio * 10);
        painter.drawEllipse(center, innerRadius - sizeRatio * 10, innerRadius - sizeRatio * 10);

        painter.translate(center);
        painter.rotate(-m_heading);

        // Fonts with proper scaling and smoothing
        QFont markingsFont("Arial", qMax(20.0 * sizeRatio, 8.0));
        markingsFont.setStyleStrategy(QFont::PreferAntialias);
        QFont directionsFont("Arial", qMax(24.0 * sizeRatio, 10.0), QFont::Bold);
        directionsFont.setStyleStrategy(QFont::PreferAntialias);

        QPen thickPen(QColor(220, 220, 240), qMax(sizeRatio * 1.5, 2.0));
        QPen thinPen(QColor(180, 180, 200), qMax(sizeRatio * 0.8, 1.0));

        QFontMetrics fm(markingsFont);
        QFontMetrics fmDir(directionsFont);

        // Draw degree markings with smooth gradients
        for (int deg = 0; deg < 360; deg += 5) {
            double angle = (deg - 90) * M_PI / 180.0;
            QPointF outerPt(outerRadius * cos(angle), outerRadius * sin(angle));
            QPointF innerPt(innerRadius * cos(angle), innerRadius * sin(angle));

            if (deg % 10 == 0) {
                // Enhanced thick lines with gradient
                QLinearGradient lineGrad(outerPt, innerPt);
                lineGrad.setColorAt(0, QColor(220, 220, 240));
                lineGrad.setColorAt(1, QColor(180, 180, 200));
                painter.setPen(QPen(QBrush(lineGrad), thickPen.width()));
                painter.drawLine(outerPt, innerPt);

                if (deg % 30 == 0) {
                    QString text;
                    bool isCardinal = deg % 90 == 0;

                    if (isCardinal) {
                        switch(deg) {
                        case 0:
                            text = "N";
                            painter.setPen(QPen(QColor(255, 100, 100), 2));
                            break;
                        case 90:
                            text = "E";
                            painter.setPen(QPen(QColor(220, 220, 240), 2));
                            break;
                        case 180:
                            text = "S";
                            painter.setPen(QPen(QColor(220, 220, 240), 2));
                            break;
                        case 270:
                            text = "W";
                            painter.setPen(QPen(QColor(220, 220, 240), 2));
                            break;
                        }
                        painter.setFont(directionsFont);
                        int textWidth = fmDir.horizontalAdvance(text);
                        int textHeight = fmDir.height();
                        QPointF textPos(
                            markingsRadius * cos(angle) - textWidth/2,
                            markingsRadius * sin(angle) + textHeight/4
                            );
                        // Draw text shadow for better visibility
                        painter.setPen(QPen(QColor(0, 0, 0, 100)));
                        painter.drawText(textPos + QPointF(1, 1), text);
                        painter.setPen(isCardinal && deg == 0 ? Qt::red : Qt::white);
                        painter.drawText(textPos, text);
                    } else {
                        text = QString::number(deg);
                        painter.setFont(markingsFont);
                        painter.setPen(Qt::white);
                        int textWidth = fm.horizontalAdvance(text);
                        int textHeight = fm.height();
                        QPointF textPos(
                            markingsRadius * cos(angle) - textWidth/2,
                            markingsRadius * sin(angle) + textHeight/4
                            );
                        // Draw text shadow
                        painter.setPen(QPen(QColor(0, 0, 0, 100)));
                        painter.drawText(textPos + QPointF(1, 1), text);
                        painter.setPen(Qt::white);
                        painter.drawText(textPos, text);
                    }
                }
            } else {
                painter.setPen(thinPen);
                painter.drawLine(outerPt, innerPt);
            }
        }

        // Draw center heading value with enhanced styling
        painter.rotate(m_heading);
        QFont headingFont("Arial", maxRadius * 0.15, QFont::Bold);
        headingFont.setStyleStrategy(QFont::PreferAntialias);
        painter.setFont(headingFont);
        QString headingStr = QString::number(static_cast<int>(m_heading)).rightJustified(3, '0') + "°";

        // Draw central background circle
        double centerCircleRadius = maxRadius * 0.25;
        QRadialGradient centerGradient(QPointF(0, 0), centerCircleRadius);
        centerGradient.setColorAt(0, QColor(50, 50, 60));
        centerGradient.setColorAt(1, QColor(30, 30, 40));
        painter.setPen(QPen(QColor(70, 70, 90), 1));
        painter.setBrush(centerGradient);
        painter.drawEllipse(QPointF(0, 0), centerCircleRadius, centerCircleRadius);

        // Draw heading text with shadow
        QRectF textRect(-maxRadius * 0.3, -maxRadius * 0.1, maxRadius * 0.6, maxRadius * 0.2);
        painter.setPen(QPen(QColor(0, 0, 0, 100)));
        painter.drawText(textRect.translated(1, 1), Qt::AlignCenter, headingStr);
        painter.setPen(Qt::white);
        painter.drawText(textRect, Qt::AlignCenter, headingStr);

        painter.restore();
    }
};
#endif // HSIWIDGET_H
