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
    const float baseAngularFrequency = 2.0f * M_PI * frequency;

    for (int i = 0; i < numSamples; ++i) {
        float t = static_cast<float>(i) / SAMPLE_RATE;
        float window = 0.5f * (1.0f - qCos(2.0f * M_PI * i / (numSamples - 1)));

        // Daha güçlü frekans modülasyonu
        // float modulatedFrequency = baseAngularFrequency * (1.0f + 0.03f * qSin(2.0f * M_PI * 4.0f * t)); // 4 Hz modülasyon
        float modulatedFrequency = baseAngularFrequency * (1.0f + 0.01f * qSin(2.0f * M_PI * 2.0f * t)); // 2 Hz modülasyon

        // Ek harmonikler
        float harmonic1 = 0.4f * qSin(2.0f * modulatedFrequency * t); // İlk harmonik
        float harmonic2 = 0.2f * qSin(3.0f * modulatedFrequency * t); // İkinci harmonik

        // Dinamik genlik değişimi ile zenginleştirilmiş ses
        data[i] = static_cast<qint16>((amplitude * qSin(modulatedFrequency * t) + harmonic1 + harmonic2) * window);
    }

    m_audioBuffer.setBuffer(&m_audioData);
    m_audioBuffer.open(QIODevice::ReadOnly);
    m_audioBuffer.seek(0);
}

void VarioSound::calculateSoundCharacteristics()
{
    if (m_currentVario <= m_sinkToneOnThreshold) {
        m_frequency = 440.0f; // Sabit batış frekansı
        m_duration = 800;    // Uzun batış sesi
        m_currentVolume = 1.3f; // Daha güçlü ses
    } else if (m_currentVario <= m_climbToneOnThreshold) {
        m_currentVolume = 0.0f; // Sessizlik
        m_duration = 0;
    } else {
        // Frekansın daha hızlı artışı
        m_frequency = qBound(800.0f,
                             static_cast<float>(5.0 * pow(m_currentVario, 3) - 120.0 * pow(m_currentVario, 2) + 850.0 * m_currentVario + 100.0),
                             3000.0f);

        // Süreler daha kısa ve dinamik
        m_duration = qBound(20, static_cast<int>(-35.0 * m_currentVario + 200.0), 350);

        // Ses seviyesi modülasyonu
        m_currentVolume = 1.25f + 0.1f * qSin(2.0f * M_PI * 2.0f * m_currentVario); // Hafif dalgalanma
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
