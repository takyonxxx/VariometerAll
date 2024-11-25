#include "variosound.h"
#include <QtMath>
#include <QAudioDevice>
#include <QAudioFormat>

VarioSound::VarioSound(QObject *parent)
    : QObject(parent)
{
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
    format.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (!device.isFormatSupported(format)) {
        format = device.preferredFormat();
    }

    m_audioSink = std::make_unique<QAudioSink>(device, format);
    connect(m_audioSink.get(), &QAudioSink::stateChanged,
            this, &VarioSound::handleAudioStateChanged);
}

void VarioSound::generateTone(float frequency, int durationMs)
{
    const int numSamples = (SAMPLE_RATE * durationMs) / 1000;
    m_audioData.resize(numSamples * sizeof(qint16));
    qint16* data = reinterpret_cast<qint16*>(m_audioData.data());

    const float amplitude = 32767.0f * m_currentVolume;
    const float angularFrequency = 2.0f * M_PI * frequency;

    // Apply Hann window for smooth transitions
    for (int i = 0; i < numSamples; ++i) {
        float t = static_cast<float>(i) / SAMPLE_RATE;
        float window = 0.5f * (1.0f - qCos(2.0f * M_PI * i / (numSamples - 1)));
        data[i] = static_cast<qint16>(amplitude * qSin(angularFrequency * t) * window);
    }

    m_audioBuffer.setBuffer(&m_audioData);
    m_audioBuffer.open(QIODevice::ReadOnly);
    m_audioBuffer.seek(0);
}

void VarioSound::calculateSoundCharacteristics()
{
    // Frequency calculation with smooth transitions
    if (m_currentVario <= m_sinkToneOnThreshold) {
        m_frequency = 440.0f;
        m_duration = 1000;
        m_currentVolume = 1.0f;
    } else if (m_currentVario <= m_climbToneOnThreshold) {
        m_currentVolume = 0.0f;
        m_duration = 0;
    } else {
        m_frequency = qBound(300.0f,
                             static_cast<float>(2.94269 * pow(m_currentVario, 3) - 71.112246 * pow(m_currentVario, 2) + 614.136517 * m_currentVario + 30.845693),
                             1800.0f);
        m_duration = qBound(50,
                            static_cast<int>(-25.0 * m_currentVario + 300.0),
                            1000);
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
        if (m_audioBuffer.isOpen()) {
            m_audioBuffer.close();
        }
        generateTone(m_frequency, m_duration);
        if (m_audioSink->state() != QAudio::StoppedState) {
            m_audioSink->stop();
        }
        m_audioSink->start(&m_audioBuffer);
    }

    m_toneTimer.start(m_duration + m_duration / 2);
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
    m_audioBuffer.close();
}
