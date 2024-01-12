#include "variosound.h"

VarioSound::VarioSound(QObject *parent)
    : QObject(parent)
    , m_stop(false)
{
    currentVario = 1.0;
    QAudioDevice outputDevice;

    for (auto &device: QMediaDevices::audioOutputs()) {
        outputDevice = device;        
        break;
    }

    m_format.setSampleFormat(QAudioFormat::Int16);
    m_format.setSampleRate(sampleRate);
    m_format.setChannelCount(1);

    m_bufferLength = m_format.bytesForDuration(BufferDurationUs);
    m_buffer.resize(m_bufferLength);
    m_buffer.fill(0);

    if (outputDevice.isFormatSupported(m_format)) {
        m_audioOutput = new QAudioSink(outputDevice, m_format, this);
        connect(m_audioOutput,&QAudioSink::stateChanged, this, &VarioSound::handleStateChanged);
    }

    const Tone tone(frequency, 1, 1.0);
    m_tone = tone;

    playSound();
}

VarioSound::~VarioSound()
{
    delete m_audioOutput;
}

void VarioSound::handleStateChanged(QAudio::State newState)
{    
    switch (newState) {
    case QAudio::IdleState:
        m_audioOutput->stop();
        playSound();
        break;

    case QAudio::StoppedState:
        // Stopped for other reasons
        if (m_audioOutput->error() != QAudio::NoError) {
            // Error handling
        }
        break;

    default:
        // ... other cases as appropriate
        break;
    }
}

void VarioSound::updateVario(qreal vario)
{   
    currentVario = vario;
    m_tone.vario = vario * 100;
}

void VarioSound::playSound()
{
    if (!m_audioOutput)
        return;

    //generateSineWave(1, sampleRate, 2, 440.0, currentVario, m_buffer);
    generateTone(m_tone, m_format, m_buffer);
    m_audioOutputIODevice.close();
    m_audioOutputIODevice.setBuffer(&m_buffer);
    if (!m_audioOutputIODevice.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open audio output device.";
        return;
    }
    m_audioOutput->start(&m_audioOutputIODevice);
}

void VarioSound::setStop(bool newStop)
{
    m_stop = newStop;
}

