#include "variosound.h"
#include <QtMath>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QAudioSink>
#include <QTimer>
#include <QIODevice>
#include <QDebug>

class ContinuousAudioBuffer : public QIODevice
{
public:
    explicit ContinuousAudioBuffer(QObject *parent = nullptr)
        : QIODevice(parent), m_readPosition(0)
    {
    }

    qint64 readData(char *data, qint64 maxSize) override {
        if (m_buffer.isEmpty()) {
            return 0;
        }

        qint64 total = 0;
        while (total < maxSize) {
            if (m_readPosition >= m_buffer.size()) {
                m_readPosition = 0;  // Loop back to start
            }

            qint64 chunk = qMin(maxSize - total, static_cast<qint64>(m_buffer.size() - m_readPosition));
            memcpy(data + total, m_buffer.constData() + m_readPosition, chunk);
            m_readPosition += chunk;
            total += chunk;
        }

        return total;
    }

    qint64 writeData(const char *data, qint64 len) override {
        Q_UNUSED(data);
        Q_UNUSED(len);
        return 0;
    }

    bool isSequential() const override {
        return true;
    }

    void setAudioData(const QByteArray &data) {
        m_buffer = data;
        m_readPosition = 0;
    }

    qint64 bytesAvailable() const override {
        return m_buffer.size() + QIODevice::bytesAvailable();
    }

private:
    QByteArray m_buffer;
    qint64 m_readPosition;
};

VarioSound::VarioSound(QObject *parent)
    : QObject(parent), m_isRunning(false), m_currentVario(0.0), m_frequency(0.0),
    m_duration(0), m_currentVolume(1.0), m_sinkToneOnThreshold(-1.0), m_climbToneOnThreshold(0.1)
{
    m_audioBuffer = new ContinuousAudioBuffer(this);
    initializeAudio();
    connect(&m_toneTimer, &QTimer::timeout, this, &VarioSound::generateNextBuffer);
}

VarioSound::~VarioSound()
{
    stop();
}

void VarioSound::initializeAudio()
{
    QAudioFormat format;
    format.setSampleRate(SAMPLE_RATE);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Float);

    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (!device.isFormatSupported(format)) {
        qWarning() << "Requested format not supported, using preferred format";
        format = device.preferredFormat();
    }

    m_audioSink = std::make_unique<QAudioSink>(device, format);
    m_audioSink->setBufferSize(32768); // larger buffer
    connect(m_audioSink.get(), &QAudioSink::stateChanged, this, &VarioSound::handleAudioStateChanged);
}

void VarioSound::generateTone(float frequency, int durationMs)
{
    if (m_currentVolume <= 0.0f || frequency <= 0.0f) {
        return;
    }

    const int sampleRate = SAMPLE_RATE;
    const int channelCount = CHANNELS;
    const int bytesPerSample = sizeof(float);
    const int totalFrames = (sampleRate * durationMs) / 1000;
    // Generate full duration of samples
    const int bufferSize = totalFrames * channelCount * bytesPerSample;

    QByteArray audioData;
    audioData.resize(bufferSize);
    float* data = reinterpret_cast<float*>(audioData.data());

    const float amplitude = m_currentVolume;
    const float angularFrequency = 2.0f * M_PI * frequency / sampleRate;

    static float lastPhase = 0.0f;
    float phase = lastPhase;

    // Generate full duration of samples
    for (int i = 0; i < totalFrames; ++i) {
        // Longer attack/release for smoother transitions
        float envelope = 1.0f;
        if (i < sampleRate / 100) { // 10ms attack
            envelope = static_cast<float>(i) / (sampleRate / 100);
        } else if (i > totalFrames - sampleRate / 100) { // 10ms release
            envelope = static_cast<float>(totalFrames - i) / (sampleRate / 100);
        }

        float freqMod = 1.0f + 0.001f * qSin(2.0f * M_PI * 3.0f * i / totalFrames);
        float sample = amplitude * qSin(phase) * envelope;
        phase += angularFrequency * freqMod;

        if (phase >= 2.0f * M_PI) {
            phase -= 2.0f * M_PI;
        }

        data[i * 2] = sample;
        data[i * 2 + 1] = sample;
    }

    lastPhase = phase;

    if (m_audioBuffer->isOpen()) {
        m_audioBuffer->close();
    }

    m_audioBuffer->setAudioData(audioData);
    if (!m_audioBuffer->open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open audio buffer!";
    }
}

void VarioSound::calculateSoundCharacteristics()
{
    // First check if we're in the silent zone (including 0)
    if (m_currentVario > m_sinkToneOnThreshold && m_currentVario < m_climbToneOnThreshold) {
        // Silent zone
        m_duration = 50;
        m_currentVolume = 0.0f;
        m_frequency = 0.0f;
    }
    // Then check sink tone
    else if (m_currentVario <= m_sinkToneOnThreshold) {
        m_frequency = 440.0f;
        m_duration = 800;
        m_currentVolume = 1.0f;
    }
    // Finally check climb tone
    else if (m_currentVario >= m_climbToneOnThreshold) {

        m_frequency = qBound(750.0f,
                             static_cast<float>(5.0 * pow(m_currentVario, 3) - 120.0 * pow(m_currentVario, 2) + 850.0 * m_currentVario + 100.0),
                             2200.0f);

        // Calculate duration based on climb rate
        // Faster climb = shorter duration
        // Map climb rate to duration: 0.2 m/s -> 600ms, 5.0 m/s -> 200ms
        // Use linear interpolation
        float normalizedVario = qBound(0.2f, static_cast<float>(m_currentVario), 5.0f);
        float durationRange = 750.0f - 100.0f;  // 800ms - 100ms
        float varioRange = 5.0f - 0.2f;         // 5.0 m/s - 0.2 m/s

        m_duration = static_cast<int>(750.0f - (durationRange * (normalizedVario - 0.2f) / varioRange)) / 5.0f;
        m_currentVolume = 1.0f;
    }
}

void VarioSound::handleAudioStateChanged(QAudio::State state)
{
    if (state == QAudio::IdleState && m_isRunning) {
        generateNextBuffer();
    }
}

void VarioSound::generateNextBuffer()
{
    if (!m_isRunning) return;

    static bool isBeeping = false;
    calculateSoundCharacteristics();

    // Handle climb tones with beep pattern
    if (m_currentVario >= m_climbToneOnThreshold) {
        if (isBeeping) {
            if (m_currentVolume > 0.0f && m_frequency > 0.0f) {
                generateTone(m_frequency, m_duration);
                if (m_audioSink->state() == QAudio::StoppedState ||
                    m_audioSink->state() == QAudio::IdleState) {
                    m_audioSink->start(m_audioBuffer);
                }
            }
        } else {
            if (m_audioSink->state() == QAudio::ActiveState) {
                m_audioSink->stop();
            }
        }
        isBeeping = !isBeeping;
    }
    // Handle sink tone (continuous)
    else if (m_currentVario <= m_sinkToneOnThreshold) {
        if (m_currentVolume > 0.0f && m_frequency > 0.0f) {
            generateTone(m_frequency, m_duration);
            if (m_audioSink->state() == QAudio::StoppedState ||
                m_audioSink->state() == QAudio::IdleState) {
                m_audioSink->start(m_audioBuffer);
            }
        }
    }
    // Handle silent zone
    else {
        if (m_audioSink->state() == QAudio::ActiveState) {
            m_audioSink->stop();
        }
        isBeeping = false;
    }

    // Always restart timer with duration
    m_toneTimer.start(m_duration);
}

void VarioSound::updateVario(qreal vario)
{
    m_currentVario = vario;
}

void VarioSound::start()
{
    if (!m_isRunning) {
        m_isRunning = true;
        generateNextBuffer();
    }
}

void VarioSound::stop()
{
    m_isRunning = false;
    m_toneTimer.stop();
    if (m_audioSink) {
        m_audioSink->stop();
    }
    if (m_audioBuffer) {
        m_audioBuffer->close();
    }
}
