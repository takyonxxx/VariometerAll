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

// Implementation file

VarioSound::VarioSound(QObject *parent)
    : QObject(parent)
    , m_stop(false)
    , m_isPulseOn(true)
    , m_currentVario(0.0)
{
    // Audio device setup
    QAudioDevice outputDevice;
    for (auto &device: QMediaDevices::audioOutputs()) {
        outputDevice = device;
        break;
    }

    m_format.setSampleFormat(QAudioFormat::Int16);
    m_format.setSampleRate(m_sampleRate);
    m_format.setChannelCount(1);
    m_bufferLength = m_format.bytesForDuration(BufferDurationUs);
    m_buffer.resize(m_bufferLength);
    m_buffer.fill(0);

    if (outputDevice.isFormatSupported(m_format)) {
        m_audioOutput = new QAudioSink(outputDevice, m_format, this);
        connect(m_audioOutput, &QAudioSink::stateChanged, this, &VarioSound::handleStateChanged);
    }

    // Pulse timer setup
    m_pulseTimer = new QTimer(this);
    connect(m_pulseTimer, &QTimer::timeout, this, &VarioSound::updatePulseTimer);

    // Initial tone setup
    m_tone = Tone(m_baseFrequency, 0.0, 0.0);
    calculateToneParameters();
}

VarioSound::~VarioSound()
{
    delete m_audioOutput;
}

void VarioSound::generateTone(const Tone &tone, const QAudioFormat &format, QByteArray &buffer, bool isPulseOn)
{
    const int channelBytes = format.bytesPerSample();
    const int sampleBytes = format.channelCount() * channelBytes;
    int length = buffer.size();
    const int numSamples = buffer.size() / sampleBytes;

    unsigned char *ptr = reinterpret_cast<unsigned char *>(buffer.data());
    static qreal phase = 0.0;

    const qreal d = 2 * M_PI / format.sampleRate();
    qreal currentFreq = tone.baseFrequency + (tone.vario * 10); // Vario etkisi
    qreal currentAmplitude = isPulseOn ? tone.amplitude : 0.0;

    while (length) {
        // Frekans modülasyonu için sinüs dalgası üretimi
        const qreal x = currentAmplitude * qSin(phase);
        const qint16 value = realToPcm(x);

        for (int i = 0; i < format.channelCount(); ++i) {
            qToLittleEndian<qint16>(value, ptr);
            ptr += channelBytes;
            length -= channelBytes;
        }

        phase += d * currentFreq;
        while (phase > 2 * M_PI)
            phase -= 2 * M_PI;
    }
}

VarioSound::SoundCharacteristics VarioSound::calculateSoundCharacteristics(qreal vario)
{
    SoundCharacteristics chars;

    // Ölü bant kontrolü
    if (qAbs(vario) < m_deadband) {
        chars.amplitude = 0.0;
        chars.frequency = m_baseFrequency;
        chars.pulseLength = 0;
        chars.silenceLength = 1000;
        return chars;
    }

    // Yükselme durumu
    if (vario > 0) {
        chars.frequency = m_baseFrequency + (vario * 60);  // Her m/s için 60 Hz artış
        // Integer dönüşümlerini düzelttik
        int pulseLengthCalc = static_cast<int>(500 - (vario * 50));
        chars.pulseLength = std::max(100, pulseLengthCalc);  // qMax yerine std::max kullanıyoruz

        int silenceLengthCalc = static_cast<int>(300 - (vario * 30));
        chars.silenceLength = std::max(50, silenceLengthCalc); // qMax yerine std::max kullanıyoruz

        chars.amplitude = 0.8;
    }
    // Alçalma durumu
    else {
        chars.frequency = m_baseFrequency + (vario * 40);  // Her m/s için 40 Hz azalış
        chars.pulseLength = 1000;  // Sürekli ses
        chars.silenceLength = 0;
        chars.amplitude = std::min(0.8, 0.4 + qAbs(vario) * 0.1); // qMin yerine std::min kullanıyoruz
    }

    return chars;
}

void VarioSound::calculateToneParameters()
{
    auto chars = calculateSoundCharacteristics(m_currentVario);

    m_tone.baseFrequency = chars.frequency;
    m_tone.amplitude = chars.amplitude;
    m_tone.pulseLength = chars.pulseLength;
    m_tone.silenceLength = chars.silenceLength;

    // Timer güncelleme
    if (m_tone.pulseLength > 0) {
        m_pulseTimer->start(m_tone.pulseLength + m_tone.silenceLength);
    } else {
        m_pulseTimer->stop();
    }
}

void VarioSound::updateVario(qreal vario)
{
    m_currentVario = vario;
    calculateToneParameters();
}

void VarioSound::updatePulseTimer()
{
    m_isPulseOn = !m_isPulseOn;
    if (!m_stop) {
        playSound();
    }
}

void VarioSound::playSound()
{
    if (!m_audioOutput || m_stop)
        return;

    generateTone(m_tone, m_format, m_buffer, m_isPulseOn);
    m_audioOutputIODevice.close();
    m_audioOutputIODevice.setBuffer(&m_buffer);

    if (!m_audioOutputIODevice.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open audio output device.";
        return;
    }

    m_audioOutput->start(&m_audioOutputIODevice);
}

void VarioSound::handleStateChanged(QAudio::State newState)
{
    switch (newState) {
    case QAudio::IdleState:
        m_audioOutput->stop();
        if (!m_stop) {
            playSound();
        }
        break;
    case QAudio::StoppedState:
        if (m_audioOutput->error() != QAudio::NoError) {
            qDebug() << "Audio output error:" << m_audioOutput->error();
        }
        break;
    default:
        break;
    }
}

void VarioSound::setStop(bool newStop)
{
    m_stop = newStop;
    if (m_stop) {
        m_pulseTimer->stop();
        m_audioOutput->stop();
    } else {
        calculateToneParameters();
    }
}
