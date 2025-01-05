#include "variosound.h"
#include <QtMath>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QAudioSink>
#include <QTimer>
#include <QIODevice>
#include <QThread>
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
    m_duration(0), m_currentVolume(1.0), m_sinkToneOnThreshold(-1.0), m_climbToneOnThreshold(0.2)
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
    format.setSampleRate(48000);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Float);

    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (!device.isFormatSupported(format)) {
        qWarning() << "Requested format not supported, using preferred format";
        format = device.preferredFormat();
    }

    m_audioSink = std::make_unique<QAudioSink>(device, format);
    // Increase buffer size for smoother playback
    m_audioSink->setBufferSize(32768); // 4x larger buffer
    connect(m_audioSink.get(), &QAudioSink::stateChanged, this, &VarioSound::handleAudioStateChanged);
}

void VarioSound::generateTone(float frequency, int durationMs)
{
    const int sampleRate = 48000;
    const int channelCount = 2;
    const int bytesPerSample = sizeof(float);
    // Increase buffer size for smoother sound
    const int framesPerBuffer = 4096; // 4x larger buffer
    const int totalFrames = (sampleRate * durationMs) / 1000;
    const int bufferSize = framesPerBuffer * channelCount * bytesPerSample;

    QByteArray audioData;
    audioData.resize(bufferSize);
    float* data = reinterpret_cast<float*>(audioData.data());

    const float amplitude = 0.5f * m_currentVolume;
    const float angularFrequency = 2.0f * M_PI * frequency / sampleRate;

    // Add phase continuity
    static float lastPhase = 0.0f;
    float phase = lastPhase;

    // Generate samples with smoother envelope
    for (int i = 0; i < framesPerBuffer; ++i) {
        // Smooth envelope with longer attack/release
        float envelope = 1.0f;
        if (i < framesPerBuffer / 8) {
            envelope = static_cast<float>(i) / (framesPerBuffer / 8); // Attack
        } else if (i > framesPerBuffer * 7 / 8) {
            envelope = static_cast<float>(framesPerBuffer - i) / (framesPerBuffer / 8); // Release
        }

        // Add slight frequency modulation for richer sound
        float freqMod = 1.0f + 0.001f * qSin(2.0f * M_PI * 3.0f * i / framesPerBuffer);

        float sample = amplitude * qSin(phase) * envelope;
        phase += angularFrequency * freqMod;

        // Normalize phase to prevent floating point errors
        if (phase >= 2.0f * M_PI) {
            phase -= 2.0f * M_PI;
        }

        data[i * 2] = sample;     // Left channel
        data[i * 2 + 1] = sample; // Right channel
    }

    lastPhase = phase;

    if (m_audioBuffer->isOpen()) {
        m_audioBuffer->close();
    }

    m_audioBuffer->setAudioData(audioData);
    if (!m_audioBuffer->open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open audio buffer!";
        return;
    }
}

void VarioSound::calculateSoundCharacteristics()
{
    if (m_currentVario <= m_sinkToneOnThreshold) {
        m_frequency = 440.0f;
        m_duration = 800;
        m_currentVolume = 1.0f;
    } else if (m_currentVario <= m_climbToneOnThreshold) {
        m_currentVolume = 0.0f;
        m_duration = 0;
    } else {
        m_frequency = qBound(800.0f,
                             static_cast<float>(5.0 * pow(m_currentVario, 3) - 120.0 * pow(m_currentVario, 2) + 850.0 * m_currentVario + 100.0),
                             3000.0f);
        m_duration = qBound(20, static_cast<int>(-35.0 * m_currentVario + 200.0), 350);
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

    calculateSoundCharacteristics();
    if (m_currentVolume > 0.0f) {
        generateTone(m_frequency, m_duration);

        if (m_audioSink->state() != QAudio::ActiveState) {
            m_audioSink->start(m_audioBuffer);
        }
    }

    // Use a longer update interval for smoother transitions
    m_toneTimer.start(50);
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
