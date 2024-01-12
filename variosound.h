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
#include <QtMath>
#include <QDebug>
#include <cmath>
#include <QByteArray>

const qint64 BufferDurationUs = 0.5 * 1000000;
const qint16 PCMS16MaxValue = 32767;
const quint16 PCMS16MaxAmplitude = 32768; // because minimum is -32768

class VarioSound : public QObject
{
    Q_OBJECT

public:
    VarioSound(QObject *parent = nullptr);
    ~VarioSound();    

    struct Tone
    {
        Tone(qreal freq = 0.0, qreal amp = 0.0 , qreal vario = 0.0) :
            frequency(freq), amplitude(amp), vario(vario) { }
        qreal frequency;
        // Amplitude in range [0.0, 1.0]
        qreal amplitude;
        qreal vario;
    };

    qreal pcmToReal(qint16 pcm)
    {
        return qreal(pcm) / PCMS16MaxAmplitude;
    }

    qint16 realToPcm(qreal real)
    {
        return real * PCMS16MaxValue;
    }

    Tone m_tone;

    void generateTone(const Tone &tone, const QAudioFormat &format, QByteArray &buffer)
    {
        Q_ASSERT(format.sampleFormat() == QAudioFormat::Int16);

        const int channelBytes = format.bytesPerSample();
        const int sampleBytes = format.channelCount() * channelBytes;
        int length = buffer.size();
        const int numSamples = buffer.size() / sampleBytes;

        Q_ASSERT(length % sampleBytes == 0);
        Q_UNUSED(sampleBytes); // suppress warning in release builds

        unsigned char *ptr = reinterpret_cast<unsigned char *>(buffer.data());

        qreal phase = 0.0;

        const qreal d = 2 * M_PI / format.sampleRate();

        // We can't generate a zero-frequency sine wave
        const qreal startFreq = tone.frequency + tone.vario;

        // Amount by which phase increases on each sample
        qreal phaseStep = d * startFreq;

        // Amount by which phaseStep increases on each sample
        // If this is non-zero, the output is a frequency-swept tone
        const qreal phaseStepStep = d * (tone.frequency + tone.vario) / numSamples;

        while (length) {
            const qreal x = tone.amplitude * qSin(phase);
            const qint16 value = realToPcm(x);
            for (int i = 0; i < format.channelCount(); ++i) {
                qToLittleEndian<qint16>(value, ptr);
                ptr += channelBytes;
                length -= channelBytes;
            }

            phase += phaseStep;
            while (phase > 2 * M_PI)
                phase -= 2 * M_PI;
            phaseStep += phaseStepStep;
        }
    }

    void updateVario(qreal vario);
    void setFrequency(qreal value);
    void setStop(bool newStop);

private slots:
    void handleStateChanged(QAudio::State);
    void playSound();

private:
    QAudioSink *m_audioOutput;
    QAudioFormat m_format;
    QBuffer m_audioOutputIODevice;
    qint64 m_bufferLength;
    QByteArray m_buffer;
    qreal frequency = 440.0;
    int phase = 0;
    qreal currentVario = 0.0;
    int sampleRate = 44100;
    bool m_stop;

//protected:
//    void run() override;
};

#endif // VARIOSOUND_H
