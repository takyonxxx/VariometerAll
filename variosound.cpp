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

    audio_format.setSampleFormat(QAudioFormat::Int16);
    audio_format.setSampleRate(sampleRate);
    audio_format.setChannelCount(2);

    if (outputDevice.isFormatSupported(audio_format)) {
        m_audioOutput = new QAudioSink(outputDevice, audio_format, this);
        connect(m_audioOutput,&QAudioSink::stateChanged, this, &VarioSound::handleStateChanged);
    }

    m_toneSampleRateHz = 750.0;
    m_durationUSeconds = DURATION_MS * 1000.0;
    m_varioFunction = new PiecewiseLinearFunction();
    m_varioFunction->addNewPoint(QPointF(0, 0.4763));
    m_varioFunction->addNewPoint(QPointF(0.441, 0.3619));
    m_varioFunction->addNewPoint(QPointF(1.029, 0.2238));
    m_varioFunction->addNewPoint(QPointF(1.559, 0.1565));
    m_varioFunction->addNewPoint(QPointF(2.471, 0.0985));
    m_varioFunction->addNewPoint(QPointF(3.571, 0.0741));
    m_varioFunction->addNewPoint(QPointF(5.0, 0.05));

    m_toneFunction = new PiecewiseLinearFunction();
    m_toneFunction->addNewPoint(QPointF(0, m_toneSampleRateHz));
    m_toneFunction->addNewPoint(QPointF(0.25, m_toneSampleRateHz + 100));
    m_toneFunction->addNewPoint(QPointF(1.0, m_toneSampleRateHz + 200));
    m_toneFunction->addNewPoint(QPointF(1.5, m_toneSampleRateHz + 300));
    m_toneFunction->addNewPoint(QPointF(2.0, m_toneSampleRateHz + 400));
    m_toneFunction->addNewPoint(QPointF(3.5, m_toneSampleRateHz + 500));
    m_toneFunction->addNewPoint(QPointF(4.0, m_toneSampleRateHz + 600));
    m_toneFunction->addNewPoint(QPointF(4.5, m_toneSampleRateHz + 700));
    m_toneFunction->addNewPoint(QPointF(6.0, m_toneSampleRateHz + 800));

    setFrequency(m_toneSampleRateHz);
}

VarioSound::~VarioSound()
{
    delete m_audioOutput;
}


void VarioSound::setFrequency(qreal value)
{
    tmp = new Generator(audio_format, m_durationUSeconds, value, this);
    tmp->start();
    delete m_generator;
    m_generator = tmp;
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

        if(currentVario > 0)
        {
            m_tone = m_toneFunction->getValue(currentVario);
        }
        else if(currentVario < 0)
        {
            m_tone = m_toneSampleRateHz;
        }

        tmp = new Generator(audio_format, m_durationUSeconds, m_tone, this);
        tmp->start();
        delete m_generator;
        m_generator = tmp;

//        playSound();
//        msleep(1000);
    }
}
