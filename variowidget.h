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

        // Draw heading text
        painter.setPen(Qt::white);
        QFont font("Tahoma", rect.width() / 8);
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
};
#endif // VARIOWIDGET_H
