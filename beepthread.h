#ifndef BEEPTHREAD_H
#define BEEPTHREAD_H

#include <QThread>
#include <QObject>
#include <QAudioSink>
#include <QMediaDevices>
#include <QAudioFormat>
#include <QByteArray>
#include <QIODevice>
#include <QVector>
#include <QPoint>
#include <QDebug>

class Generator : public QIODevice
{
public:
    Generator(const QAudioFormat &format,
              int frequency,
              QObject *parent)
        : QIODevice(parent)
        , m_format(format)
        , m_frequency(frequency)
        , m_phase(0)
    {
        m_omega = 2.0 * M_PI * frequency / m_format.sampleRate();
    }

    void start() {
        open(QIODevice::ReadOnly);
    }

    void stop() {
        close();
    }

protected:
    qint64 readData(char *data, qint64 maxSize) override
    {
        // Always generate exactly 1024 stereo samples (8192 bytes)
        const int samplesPerPacket = 1024;
        const int bytesPerPacket = samplesPerPacket * sizeof(float) * m_format.channelCount();

        if (maxSize < bytesPerPacket) {
            return 0;
        }

        float* ptr = reinterpret_cast<float*>(data);

        // Generate one packet of samples
        for (int i = 0; i < samplesPerPacket; ++i) {
            float value = 0.5f * std::sin(m_phase);
            m_phase += m_omega;

            // Keep phase in reasonable range
            if (m_phase > 2.0 * M_PI) {
                m_phase -= 2.0 * M_PI;
            }

            // Write to both channels
            *ptr++ = value;  // Left
            *ptr++ = value;  // Right
        }

        return bytesPerPacket;
    }

    qint64 writeData(const char *data, qint64 len) override
    {
        Q_UNUSED(data);
        Q_UNUSED(len);
        return 0;
    }

    qint64 bytesAvailable() const override
    {
        return 1024 * sizeof(float) * m_format.channelCount();
    }

private:
    QAudioFormat m_format;
    int m_frequency;
    double m_omega;
    double m_phase;
};

class PiecewiseLinearFunction
{
public:
    PiecewiseLinearFunction();

    QVector <QPointF> points;
    double posInfValue;
    double negInfValue;

    void addNewPoint(QPointF point);
    qreal getValue(qreal x);
    int getSize();

};

#define DURATION_MS 1000
static const int DataSampleRateHz = 44100;
static const int BufferSize = 32768;

class Sleeper : public QThread
{
public:
    static void usleep(unsigned long usecs){QThread::usleep(usecs);}
    static void msleep(unsigned long msecs){QThread::msleep(msecs);}
    static void sleep(unsigned long secs){QThread::sleep(secs);}
};

class BeepThread : public QThread
{
    Q_OBJECT
public:
    explicit BeepThread(QObject *parent = nullptr);
    ~BeepThread();

    void run() override;
    void startBeep();
    void stopBeep();
    void resumeBeep();
    void setVolume(qreal volume);
    void SetVario(qreal vario, qreal tdiff);

private:
    void initializeAudio();

    QAudioSink *m_audioOutput{nullptr};
    Generator *m_generator{nullptr};
    QByteArray m_buffer;
    QAudioFormat m_format;

    PiecewiseLinearFunction *m_varioFunction{nullptr};
    PiecewiseLinearFunction *m_toneFunction{nullptr};

    qreal m_vario{0.0};
    qreal m_tone{0.0};
    qreal m_tdiff{0.0};
    qreal m_toneSampleRateHz{0.0};
    qreal m_durationUSeconds{0.0};

    float m_climbToneOnThreshold{0.0};
    float m_sinkToneOnThreshold{0.0};

    bool m_running{false};
};

#endif // BEEPTHREAD_H
