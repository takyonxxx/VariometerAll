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
            memset(data, 0, maxSize);
            return maxSize;
        }

        // Ensure we're reading complete frames
        const qint64 frameSize = sizeof(float) * 2; // 2 channels
        maxSize = (maxSize / frameSize) * frameSize;

        qint64 total = 0;
        float* outputData = reinterpret_cast<float*>(data);
        const float* inputData = reinterpret_cast<const float*>(m_buffer.constData());
        const qint64 framesPerChannel = m_buffer.size() / (sizeof(float) * 2);

        while (total < maxSize) {
            if (m_readPosition >= framesPerChannel) {
                m_readPosition = 0;
            }

            // Calculate how many frames we can copy
            qint64 framesToCopy = qMin((maxSize - total) / frameSize,
                                       framesPerChannel - m_readPosition);

            const float* leftSrc = inputData;
            const float* rightSrc = inputData + framesPerChannel;

            // Copy maintaining deinterleaved format
            for (qint64 i = 0; i < framesToCopy; ++i) {
                outputData[i * 2] = leftSrc[m_readPosition + i];
                outputData[i * 2 + 1] = rightSrc[m_readPosition + i];
            }

            m_readPosition += framesToCopy;
            total += framesToCopy * frameSize;
            outputData += framesToCopy * 2;
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
    format.setChannelCount(CHANNELS);
    format.setSampleFormat(QAudioFormat::Float);

    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (!device.isFormatSupported(format)) {
        qWarning() << "Requested format not supported, trying to use preferred format";
        format = device.preferredFormat();
    }

    m_audioSink = std::make_unique<QAudioSink>(device, format);

    // For iOS, we want a larger buffer size to prevent underruns
    const int preferredBufferSize = 32768;  // Adjust this value if needed
    m_audioSink->setBufferSize(preferredBufferSize);

    connect(m_audioSink.get(), &QAudioSink::stateChanged,
            this, &VarioSound::handleAudioStateChanged);
}

void VarioSound::calculateSoundCharacteristics()
{
    if (m_currentVario > m_sinkToneOnThreshold && m_currentVario < m_climbToneOnThreshold) {
        m_duration = 50;
        m_currentVolume = 0.0f;
        m_frequency = 0.0f;
    } else if (m_currentVario <= m_sinkToneOnThreshold) {
        m_frequency = 440.0f;
        m_duration = 800;
        m_currentVolume = 1.0f;
    } else if (m_currentVario >= m_climbToneOnThreshold) {
        m_frequency = qBound(750.0f,
                             750.0f + 1450.0f * (m_currentVario / 5.0f),
                             2200.0f);
        float durationRange = 400.0f - 50.0f;
        m_duration = 400.0f - (durationRange * (m_currentVario / 5.0f));
        m_currentVolume = 1.0f;
    }
}

void VarioSound::handleAudioStateChanged(QAudio::State state)
{
    if (state == QAudio::IdleState && m_isRunning) {
        generateNextBuffer();
    }
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

    // For iOS, we need to create deinterleaved buffers
    // Each channel gets its own continuous block of memory
    const int singleChannelSize = totalFrames * bytesPerSample;
    const int totalSize = singleChannelSize * channelCount;

    QByteArray audioData;
    audioData.resize(totalSize);
    float* data = reinterpret_cast<float*>(audioData.data());

    // Get pointers to the start of each channel's data block
    float* leftChannel = data;
    float* rightChannel = data + totalFrames;  // Point to second half of buffer

    const float amplitude = m_currentVolume * 0.5f;
    const float angularFrequency = 2.0f * M_PI * frequency / sampleRate;

    static float lastPhase = 0.0f;
    float phase = lastPhase;

    // Pre-calculate envelope
    QVector<float> envelope(totalFrames);
    const int attackSamples = sampleRate / 100;
    const int releaseSamples = sampleRate / 100;

    for (int i = 0; i < totalFrames; ++i) {
        if (i < attackSamples) {
            envelope[i] = static_cast<float>(i) / attackSamples;
        } else if (i > totalFrames - releaseSamples) {
            envelope[i] = static_cast<float>(totalFrames - i) / releaseSamples;
        } else {
            envelope[i] = 1.0f;
        }
    }

    // Generate samples in deinterleaved format
    for (int i = 0; i < totalFrames; ++i) {
        float freqMod = 1.0f + 0.001f * qSin(2.0f * M_PI * 3.0f * i / totalFrames);
        float sample = amplitude * qSin(phase) * envelope[i];
        phase += angularFrequency * freqMod;

        while (phase >= 2.0f * M_PI) {
            phase -= 2.0f * M_PI;
        }

        // Write the same sample to both channels, but in their separate memory blocks
        leftChannel[i] = sample;
        rightChannel[i] = sample;
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

void VarioSound::generateNextBuffer()
{
    if (!m_isRunning) return;

    static bool isBeeping = false;
    calculateSoundCharacteristics();

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
    } else if (m_currentVario <= m_sinkToneOnThreshold) {
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
        isBeeping = false;
    }

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
