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
#include "generator.h"

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

signals:
    void finished();
};

#endif // BEEPTHREAD_H
