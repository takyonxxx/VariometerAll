#include "variosound.h"
#include <QApplication>
#include <QMutexLocker>

VarioSound::VarioSound(QObject *parent)
    : QThread(parent)
    , m_mediaPlayer(nullptr)
    , m_audioOutput(nullptr)
    , m_stop(false)
    , m_mediaLoaded(false)
{
    m_climbToneOnThreshold = 0.2f;
    m_sinkToneOnThreshold = -2.0f;

    m_currentVario = 0.0;
    m_currentPlaybackRate = 1.0;
    m_currentVolume = 1.0;
    m_duration = 600;
    m_frequency = 600;
}

VarioSound::~VarioSound()
{
    setStop();
    wait();

    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
        delete m_mediaPlayer;
    }
    if (m_audioOutput) {
        delete m_audioOutput;
    }
}

bool VarioSound::initializeAudio()
{
    // Verify resource exists
    if (!QFile::exists(":/sounds/vario_tone.wav")) {
        qDebug() << "Error: Sound file not found in resources!";
        return false;
    }

    m_mediaPlayer = new QMediaPlayer();
    m_audioOutput = new QAudioOutput();

    if (!m_mediaPlayer || !m_audioOutput) {
        qDebug() << "Error: Failed to create media objects!";
        return false;
    }

    m_mediaPlayer->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(1.0);

    // Connect signals before setting the source
    connect(m_mediaPlayer, &QMediaPlayer::errorOccurred,
            this, &VarioSound::handleErrorOccurred,
            Qt::DirectConnection);

    connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged,
            this, &VarioSound::handleMediaStatusChanged,
            Qt::DirectConnection);

    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged,
            this, &VarioSound::handlePlaybackStateChanged,
            Qt::DirectConnection);

    // Set the source
    m_mediaPlayer->setSource(QUrl("qrc:/sounds/vario_tone.wav"));

    // Wait a short time for media to load
    for (int i = 0; i < 50 && !m_mediaLoaded; ++i) {
        if (m_stop) return false;
        msleep(100);
        QCoreApplication::processEvents();
    }

    if (!m_mediaLoaded) {
        qDebug() << "Error: Media failed to load within timeout!";
        return false;
    }

    return true;
}

void VarioSound::calculateSoundCharacteristics()
{
    // Calculate frequency based on vario value
    m_frequency = 2.94269*m_currentVario*m_currentVario*m_currentVario - 71.112246*m_currentVario*m_currentVario +
                      614.136517*m_currentVario + 30.845693;

    // Calculate duration
    m_duration = -38.00*m_currentVario + 400.00; // Variable Pause

    m_frequency = int(m_frequency);
    m_duration = long(m_duration);

    // Calculate playback rate based on target frequency
    float baseFreq = 600.0f;  // Base frequency of our WAV file
    m_currentPlaybackRate = static_cast<float>(m_frequency) / baseFreq;

    // Set volume
    m_currentVolume = 1.0;

    m_currentVario = -3.0;

    if (m_currentVario <= m_sinkToneOnThreshold) {
        m_frequency = 300;
        m_duration = 500;
    }
    else if (m_currentVario <= m_climbToneOnThreshold) {
        m_currentVolume = 0.0;
        m_duration = 0;
    }

    // Debug output
    qDebug() << "Sound characteristics calculated:"
             << "\n  Vario:" << m_currentVario
             << "\n  Frequency:" << m_frequency
             << "\n  Duration:" << m_duration
             << "\n  Playback Rate:" << m_currentPlaybackRate
             << "\n  Volume:" << m_currentVolume;
}

void VarioSound::stopSound()
{
    if (m_mediaPlayer && m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_mediaPlayer->pause();
    }
}

void VarioSound::playSound()
{
    if (!m_mediaPlayer || m_stop || !m_mediaLoaded)
        return;

    calculateSoundCharacteristics();

    if (m_mediaPlayer && m_audioOutput) {
        m_mediaPlayer->setPlaybackRate(m_currentPlaybackRate);
        m_audioOutput->setVolume(m_currentVolume);

        m_mediaPlayer->setPosition(0);
        m_mediaPlayer->play();
    }
}

void VarioSound::handlePlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    if (state == QMediaPlayer::StoppedState && !m_stop) {
        if (m_duration == 0) {
            m_mediaPlayer->setPosition(0);
            m_mediaPlayer->play();
        }
    }
}

void VarioSound::handleMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::LoadedMedia) {
        m_mediaLoaded = true;
    }
}

void VarioSound::handleErrorOccurred(QMediaPlayer::Error error, const QString &errorString)
{
    qDebug() << "Media player error:" << error << errorString;
}

void VarioSound::updateVario(qreal vario)
{
    m_currentVario = vario;
}

void VarioSound::setStop()
{
    m_stop = true;
}

void VarioSound::run()
{
    if (!initializeAudio()) {
        qDebug() << "Failed to initialize audio system!";
        return;
    }

    while (!m_stop)
    {
        playSound();
        msleep(m_duration > 0 ? m_duration : 400);
        stopSound();
        msleep(m_duration > 0 ? m_duration : 400);
    }

    delete m_mediaPlayer;
    m_mediaPlayer = nullptr;
    delete m_audioOutput;
    m_audioOutput = nullptr;
}
