#ifndef VARIOSOUND_H
#define VARIOSOUND_H

#include <QAudioOutput>
#include <QMediaDevices>
#include <QAudioFormat>
#include <QAudioSink>
#include <QAudioSource>
#include <QBuffer>
#include <QThread>
#include <QtMath>
#include <QtEndian>
#include <QDebug>
#include <cmath>
#include <QByteArray>
#include <QTimer>

const qint64 BufferDurationUs = 0.1 * 1000000; // Daha kısa buffer süresi
const qint16 PCMS16MaxValue = 32767;
const quint16 PCMS16MaxAmplitude = 32768;

class VarioSound : public QObject
{
    Q_OBJECT
public:
    VarioSound(QObject *parent = nullptr);
    ~VarioSound();

    struct Tone {
        Tone(qreal freq = 0.0, qreal amp = 0.0, qreal vario = 0.0) :
            baseFrequency(freq), amplitude(amp), vario(vario),
            pulseLength(0), silenceLength(0) { }
        qreal baseFrequency;
        qreal amplitude;
        qreal vario;
        int pulseLength;    // ms cinsinden bip uzunluğu
        int silenceLength;  // ms cinsinden bipler arası sessizlik
    };

    void updateVario(qreal vario);
    void setStop(bool newStop);

private slots:
    void handleStateChanged(QAudio::State);
    void playSound();
    void updatePulseTimer();

private:
    qreal pcmToReal(qint16 pcm) {
        return qreal(pcm) / PCMS16MaxAmplitude;
    }

    qint16 realToPcm(qreal real) {
        return real * PCMS16MaxValue;
    }

    void generateTone(const Tone &tone, const QAudioFormat &format, QByteArray &buffer, bool isPulseOn);
    void calculateToneParameters();

    QAudioSink *m_audioOutput;
    QAudioFormat m_format;
    QBuffer m_audioOutputIODevice;
    qint64 m_bufferLength;
    QByteArray m_buffer;

    Tone m_tone;
    QTimer *m_pulseTimer;
    bool m_isPulseOn;

    // Vario parametreleri
    qreal m_currentVario;
    const qreal m_baseFrequency = 500.0;  // Temel frekans
    const qreal m_deadband = 0.2;         // m/s, ölü bant
    const int m_sampleRate = 44100;
    bool m_stop;

    // Ses karakteristikleri
    struct SoundCharacteristics {
        qreal frequency;
        int pulseLength;
        int silenceLength;
        qreal amplitude;
    };

    SoundCharacteristics calculateSoundCharacteristics(qreal vario);
};

#endif // VARIOSOUND_H
