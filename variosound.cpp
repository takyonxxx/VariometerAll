
#include "variosound.h"

VarioSound::VarioSound(QObject *parent)
    : QObject(parent)
{
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

    m_pulseTimer = new QTimer(this);
    connect(m_pulseTimer, &QTimer::timeout, this, &VarioSound::updatePulseTimer);

    m_tone = Tone(m_baseFrequency, 0.8, 0.0);
}

VarioSound::~VarioSound()
{
    if (m_audioOutput) {
        m_audioOutput->stop();
    }
}

void VarioSound::generateTone(const Tone &tone, const QAudioFormat &format, QByteArray &buffer, bool isPulseOn)
{
    const int channelBytes = format.bytesPerSample();
    const int sampleBytes = format.channelCount() * channelBytes;
    int length = buffer.size();
    const int numSamples = buffer.size() / sampleBytes;
    unsigned char *ptr = reinterpret_cast<unsigned char *>(buffer.data());
    static qreal phase = 0.0;
    static qreal lastFreq = tone.baseFrequency;
    static qreal lastAmplitude = 0.0;

    const qreal d = 2 * M_PI / format.sampleRate();
    qreal targetFreq = tone.baseFrequency + (tone.vario * 60);
    qreal targetAmplitude = isPulseOn ? tone.amplitude : 0.0;

    // Fade parametreleri
    const int fadeLength = numSamples / 10;  // İlk ve son %10'luk kısım için fade
    const qreal freqChangeRate = 0.1;        // Frekans değişim hızı (0-1 arası)

    int currentSample = 0;
    while (length) {
        // Yumuşak frekans geçişi
        lastFreq = lastFreq + (targetFreq - lastFreq) * freqChangeRate;

        // Fade in/out ve amplitude yumuşatma
        qreal fadeMultiplier = 1.0;
        if (currentSample < fadeLength) {
            // Fade in
            fadeMultiplier = static_cast<qreal>(currentSample) / fadeLength;
        } else if (currentSample > numSamples - fadeLength) {
            // Fade out
            fadeMultiplier = static_cast<qreal>(numSamples - currentSample) / fadeLength;
        }

        // Amplitude yumuşatma
        lastAmplitude = lastAmplitude + (targetAmplitude - lastAmplitude) * 0.1;

        // Son amplitude değeri
        qreal currentAmplitude = lastAmplitude * fadeMultiplier;

        // Sinüs dalgası üretimi
        const qreal x = currentAmplitude * qSin(phase);
        const qint16 value = realToPcm(x);

        for (int i = 0; i < format.channelCount(); ++i) {
            qToLittleEndian<qint16>(value, ptr);
            ptr += channelBytes;
            length -= channelBytes;
        }

        phase += d * lastFreq;
        while (phase > 2 * M_PI)
            phase -= 2 * M_PI;

        currentSample++;
    }
}

VarioSound::SoundCharacteristics VarioSound::calculateSoundCharacteristics(qreal vario)
{
    SoundCharacteristics chars;

    if (qAbs(vario) < m_deadband) {
        chars.amplitude = 0.0;
        chars.frequency = m_baseFrequency;
        chars.pulseLength = 0;
        chars.silenceLength = 1000;
        return chars;
    }

    if (vario > 0) {
        chars.frequency = m_baseFrequency + (vario * 100);
        chars.pulseLength = 50;  // Yüksek hızda daha kısa bipler
        chars.silenceLength = static_cast<int>(75 - (vario * 10)); // Hızla azalan sessizlik
        chars.amplitude = 0.8;
    } else {
        chars.frequency = m_baseFrequency + (vario * 40);
        chars.pulseLength = 1000;
        chars.silenceLength = 0;
        chars.amplitude = 0.8;
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

    if (m_tone.pulseLength > 0) {
        m_pulseTimer->start(m_tone.pulseLength + m_tone.silenceLength);
    } else {
        m_pulseTimer->stop();
    }
}

void VarioSound::updateVario(qreal vario)
{
    m_currentVario = vario;
    m_tone.vario = vario;
    calculateToneParameters();
    playSound();
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
        return;
    }

    m_audioOutput->start(&m_audioOutputIODevice);
}

void VarioSound::handleStateChanged(QAudio::State newState)
{
    switch (newState) {
    case QAudio::IdleState:
        m_audioOutput->stop();
        break;
    case QAudio::StoppedState:
        if (m_audioOutput->error() != QAudio::NoError) {
            qDebug() << "Audio error:" << m_audioOutput->error();
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
