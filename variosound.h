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
#include <algorithm>

const qint64 BufferDurationUs = 0.1 * 1000000;
const qint16 PCMS16MaxValue = 32767;
const quint16 PCMS16MaxAmplitude = 32768;

class VarioSound : public QObject
{
    Q_OBJECT
public:
    explicit VarioSound(QObject *parent = nullptr);
    ~VarioSound();

    struct Tone {
        Tone(qreal freq = 0.0, qreal amp = 0.0, qreal vario = 0.0) :
            baseFrequency(freq), amplitude(amp), vario(vario),
            pulseLength(0), silenceLength(0) { }
        qreal baseFrequency;
        qreal amplitude;
        qreal vario;
        int pulseLength;
        int silenceLength;
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

    QAudioSink *m_audioOutput{nullptr};
    QAudioFormat m_format;
    QBuffer m_audioOutputIODevice;
    qint64 m_bufferLength{0};
    QByteArray m_buffer;
    Tone m_tone;
    QTimer *m_pulseTimer{nullptr};
    bool m_isPulseOn{false};
    bool m_isPlaying{false};
    qreal m_currentVario{0.0};
    const qreal m_baseFrequency{440.0};
    const qreal m_deadband{0.2};
    const int m_sampleRate{44100};
    bool m_stop{false};

    struct SoundCharacteristics {
        qreal frequency{0.0};
        int pulseLength{0};
        int silenceLength{0};
        qreal amplitude{0.0};
    };

    SoundCharacteristics calculateSoundCharacteristics(qreal vario);
};

#endif // VARIOSOUND_H
