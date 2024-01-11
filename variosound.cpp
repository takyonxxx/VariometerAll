#include "variosound.h"

VarioSound::VarioSound(QObject *parent)
    : QThread(parent), m_stop(false)
{       
    QAudioDevice outputDevice;

    for (auto &device: QMediaDevices::audioOutputs()) {
        outputDevice = device;
        if(outputDevice.description().contains("JBL"))
        break;
    }

    QAudioFormat audio_format;
    audio_format.setSampleFormat(QAudioFormat::Int16);
    audio_format.setSampleRate(sampleRate);
    audio_format.setChannelCount(2);

    if (outputDevice.isFormatSupported(audio_format)) {
        m_audioOutput = new QAudioSink(outputDevice, audio_format, this);
        connect(m_audioOutput,&QAudioSink::stateChanged, this, &VarioSound::handleStateChanged);
    }    
}

VarioSound::~VarioSound()
{
    delete m_audioOutput;
}

void VarioSound::handleStateChanged(QAudio::State newState)
{
    switch (newState) {
    case QAudio::IdleState:
        // Finished playing (no more data)
        if (m_audioOutputIODevice.isOpen())
            m_audioOutputIODevice.close();

        m_audioOutput->stop();
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
    qDebug() << vario;
    currentVario = vario;   
}

void VarioSound::playSound()
{
    if(!m_audioOutput)
        return;
    qDebug() << "playing: " << currentVario;
    QByteArray audioBuffer = generateSineWave(3, sampleRate, 2, 440.0, currentVario);

    if (m_audioOutputIODevice.isOpen())
        m_audioOutputIODevice.close();

    // m_audioOutputIODevice.setBuffer(&audioBuffer);
    m_audioOutputIODevice.setData(audioBuffer);
    m_audioOutputIODevice.open(QIODevice::ReadOnly);
    m_audioOutput->start(&m_audioOutputIODevice);
}

void VarioSound::setStop(bool newStop)
{
    m_stop = newStop;
}

void VarioSound::run()
{
    while (!m_stop)
    {
        if(m_stop)
            break;

        playSound();
        msleep(1000);
    }
}
