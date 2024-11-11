#include "BeepThread.h"

PiecewiseLinearFunction::PiecewiseLinearFunction()
{
    posInfValue = 0.0;
    negInfValue = 0.0;
}

void PiecewiseLinearFunction::addNewPoint(QPointF point)
{
    if (point.x() == 0|| point.y() == 0) {
        return;
    } else if (points.size() == 0) {
        points.push_back(point);
        return;
    } else if (point.x() > (points.at(points.size() - 1)).x()) {
        points.push_back(point);
        return;
    } else {
        for (int i = 0; i < points.size(); i++) {
            if ((points.at(i)).x() > point.x()) {
                points.insert(i, point);
                return;
            }
        }
    }
}

qreal PiecewiseLinearFunction::getValue(qreal x)
{
    QPointF point;
    QPointF lastPoint = points.at(0);
    if (x <= lastPoint.x()) {
        return lastPoint.y();
    }
    for (int i = 1; i < points.size(); i++) {
        point = points.at(i);
        if (x <= point.x()) {
            double ratio = (x - lastPoint.x()) / (point.x() - lastPoint.x());
            return lastPoint.y() + ratio * (point.y() - lastPoint.y());
        }
        lastPoint = point;
    }
    return lastPoint.y();
}

int PiecewiseLinearFunction::getSize()
{
    return points.size();
}

BeepThread::BeepThread(QObject *parent)
    : QThread(parent)
    , m_buffer(BufferSize, 0)
{
    // Base settings
    m_toneSampleRateHz = 300.0;
    m_durationUSeconds = DURATION_MS * 1000.0;
    m_climbToneOnThreshold = 0.2f;
    m_sinkToneOnThreshold = -2.0f;

    // Vario timing function (cycle lengths in ms)
    // More gradual progression in lower ranges
    m_varioFunction = new PiecewiseLinearFunction();
    m_varioFunction->addNewPoint(QPointF(0.00, 0.200));  // Start slower for better sync
    m_varioFunction->addNewPoint(QPointF(0.20, 0.190));  // Very weak lift
    m_varioFunction->addNewPoint(QPointF(0.40, 0.180));  // Beginning lift
    m_varioFunction->addNewPoint(QPointF(0.60, 0.170));  // Weak lift
    m_varioFunction->addNewPoint(QPointF(0.80, 0.160));  // Building lift
    m_varioFunction->addNewPoint(QPointF(1.00, 0.150));  // Steady lift
    m_varioFunction->addNewPoint(QPointF(1.25, 0.140));  // Good lift starting
    m_varioFunction->addNewPoint(QPointF(1.50, 0.130));  // Good lift
    m_varioFunction->addNewPoint(QPointF(1.75, 0.120));  // Strong lift beginning
    m_varioFunction->addNewPoint(QPointF(2.00, 0.110));  // Strong lift
    m_varioFunction->addNewPoint(QPointF(2.25, 0.100));  // Very strong lift
    m_varioFunction->addNewPoint(QPointF(2.50, 0.090));  // Transition point
    // Keep existing good behavior for higher values
    m_varioFunction->addNewPoint(QPointF(3.50, 0.080));
    m_varioFunction->addNewPoint(QPointF(4.24, 0.070));
    m_varioFunction->addNewPoint(QPointF(5.00, 0.060));
    m_varioFunction->addNewPoint(QPointF(6.00, 0.050));
    m_varioFunction->addNewPoint(QPointF(7.00, 0.045));
    m_varioFunction->addNewPoint(QPointF(8.00, 0.040));
    m_varioFunction->addNewPoint(QPointF(9.00, 0.035));
    m_varioFunction->addNewPoint(QPointF(10.00, 0.030));

    // Tone frequency function (Hz)
    // More distinct frequency changes in lower ranges
    m_toneFunction = new PiecewiseLinearFunction();
    m_toneFunction->addNewPoint(QPointF(0.00, 300));     // Base frequency
    m_toneFunction->addNewPoint(QPointF(0.20, 350));     // Very weak lift
    m_toneFunction->addNewPoint(QPointF(0.40, 400));     // Beginning lift
    m_toneFunction->addNewPoint(QPointF(0.60, 450));     // Weak lift
    m_toneFunction->addNewPoint(QPointF(0.80, 500));     // Building lift
    m_toneFunction->addNewPoint(QPointF(1.00, 550));     // Steady lift
    m_toneFunction->addNewPoint(QPointF(1.25, 600));     // Good lift starting
    m_toneFunction->addNewPoint(QPointF(1.50, 650));     // Good lift
    m_toneFunction->addNewPoint(QPointF(1.75, 700));     // Strong lift beginning
    m_toneFunction->addNewPoint(QPointF(2.00, 750));     // Strong lift
    m_toneFunction->addNewPoint(QPointF(2.25, 800));     // Very strong lift
    m_toneFunction->addNewPoint(QPointF(2.50, 850));     // Transition point
    // Keep existing good behavior for higher values
    m_toneFunction->addNewPoint(QPointF(3.50, 950));
    m_toneFunction->addNewPoint(QPointF(4.24, 1050));
    m_toneFunction->addNewPoint(QPointF(5.00, 1150));
    m_toneFunction->addNewPoint(QPointF(6.00, 1300));
    m_toneFunction->addNewPoint(QPointF(7.00, 1450));
    m_toneFunction->addNewPoint(QPointF(8.00, 1600));
    m_toneFunction->addNewPoint(QPointF(9.00, 1700));
    m_toneFunction->addNewPoint(QPointF(10.00, 1800));
}

BeepThread::~BeepThread()
{
    m_running = false;
    wait();

    delete m_generator;
    delete m_audioOutput;
    delete m_varioFunction;
    delete m_toneFunction;
}

void BeepThread::run()
{
    initializeAudio();

    while (m_running)
    {
        if (m_vario >= m_climbToneOnThreshold)
        {
            qreal currentTone = m_toneFunction->getValue(qMin(m_vario, 10.0));
            qreal beepDuration = m_varioFunction->getValue(qMin(m_vario, 10.0)) * 1000;

            Generator* newGen = new Generator(m_format,
                                              static_cast<int>(currentTone),
                                              nullptr);

            if (m_generator) {
                m_generator->stop();
                delete m_generator;
            }
            m_generator = newGen;
            m_generator->start();

            if (m_audioOutput && m_generator) {
                m_audioOutput->start(m_generator);
                Sleeper::msleep(beepDuration);
                m_audioOutput->stop();

                qreal silenceDuration = beepDuration;
                Sleeper::msleep(silenceDuration);
            }
        }
        else if (m_vario <= m_sinkToneOnThreshold)
        {
            Generator* newGen = new Generator(m_format,
                                              static_cast<int>(m_toneSampleRateHz * 0.5),
                                              nullptr);
            newGen->start();

            if (m_generator) {
                m_generator->stop();
                delete m_generator;
            }
            m_generator = newGen;

            if (m_audioOutput) {
                m_audioOutput->start(m_generator);
                Sleeper::msleep(500);
            }
        }
        else
        {
            if (m_audioOutput) {
                m_audioOutput->stop();
            }
            Sleeper::msleep(100);
        }
    }

    if (m_generator) {
        m_generator->stop();
        delete m_generator;
        m_generator = nullptr;
    }

    if (m_audioOutput) {
        m_audioOutput->stop();
        delete m_audioOutput;
        m_audioOutput = nullptr;
    }
}

void BeepThread::initializeAudio()
{
    m_format.setSampleRate(48000);
    m_format.setChannelCount(2);
    m_format.setSampleFormat(QAudioFormat::Float);

    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (!device.isFormatSupported(m_format)) {
        qWarning() << "Default format not supported - using preferred format";
        m_format = device.preferredFormat();
    }

    delete m_audioOutput;
    m_audioOutput = new QAudioSink(device, m_format, nullptr);
    m_audioOutput->setVolume(1.0);
}

void BeepThread::startBeep()
{
    m_running = true;
}

void BeepThread::stopBeep()
{
    m_running = false;
    if (m_audioOutput) {
        m_audioOutput->stop();
    }
}

void BeepThread::resumeBeep()
{
    if (m_audioOutput) {
        m_audioOutput->resume();
    }
}

void BeepThread::setVolume(qreal volume)
{
    if (m_audioOutput) {
        m_audioOutput->setVolume(volume / 100.0);
    }
}

void BeepThread::SetVario(qreal vario, qreal tdiff)
{
    m_vario = vario;
    m_tdiff = tdiff;
}
