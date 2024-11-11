#include "variosound.h"

VarioSound::VarioSound(QObject *parent)
    : QObject(parent)
{
    // Initialize media player
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);

    // Load WAV file from resources
    m_mediaPlayer->setSource(QUrl("qrc:/sounds/vario_tone.wav"));
    m_audioOutput->setVolume(0.75);

    // Create timer for managing silence between beeps
    m_delayTimer = new QTimer(this);
    m_delayTimer->setSingleShot(true);
    connect(m_delayTimer, &QTimer::timeout, this, &VarioSound::onDelayFinished);

    // Create stop timer for precise control
    m_stopTimer = new QTimer(this);
    m_stopTimer->setSingleShot(true);
    connect(m_stopTimer, &QTimer::timeout, this, &VarioSound::stopSound);

    // Set initial position and durations
    m_startPosition = 100;
    m_initial_duration = 400;
    m_initial_silenceDuration = 50;
    m_duration = m_initial_duration;
    m_silenceDuration = m_initial_silenceDuration;
}

VarioSound::~VarioSound()
{
    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
        delete m_mediaPlayer;
    }
    if (m_delayTimer) {
        m_delayTimer->stop();
        delete m_delayTimer;
    }
    if (m_stopTimer) {
        m_stopTimer->stop();
        delete m_stopTimer;
    }
    if (m_audioOutput) {
        delete m_audioOutput;
    }
}

void VarioSound::calculateSoundCharacteristics(qreal vario)
{
    if (qAbs(vario) < m_deadband) {
        m_currentPlaybackRate = 1.0;
        m_currentVolume = 0.0;
        m_duration = m_initial_duration;
        m_silenceDuration = m_initial_silenceDuration;
        return;
    }

    if (vario > 0) {
        // Playback rate changes
        m_currentPlaybackRate = 1.0 + (vario * 0.12);
        m_currentVolume = 1.0;

        // Dynamic sound duration for climbing
        m_duration = static_cast<qint64>(m_initial_duration - (vario * 40));
        m_duration = std::max(static_cast<qint64>(10), m_duration);

        // Dynamic silence duration for climbing
        m_silenceDuration = static_cast<int>(m_initial_silenceDuration - (vario * 10));
        m_silenceDuration = std::max(10, m_silenceDuration);
    } else {
        m_currentPlaybackRate = 1.0 + (vario * 0.06);
        m_currentVolume = 0.5;

        // Fixed longer duration for sink
        m_duration = m_initial_duration;
        m_silenceDuration = m_initial_silenceDuration;
    }
}

void VarioSound::stopSound()
{
    if (m_mediaPlayer && m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_mediaPlayer->stop();

        if (!m_stop) {
            m_delayTimer->start(m_silenceDuration);
        }
    }
}

void VarioSound::playSound()
{
    if (!m_mediaPlayer || m_stop)
        return;

    m_stopTimer->stop();

    m_mediaPlayer->stop();
    QThread::msleep(5);

    m_mediaPlayer->setPosition(m_startPosition);
    m_mediaPlayer->play();

    m_stopTimer->start(m_duration);
}

void VarioSound::handlePlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    if (state == QMediaPlayer::StoppedState && !m_stop &&
        !m_delayTimer->isActive() && !m_stopTimer->isActive()) {
        m_delayTimer->start(m_silenceDuration);
    }
}

void VarioSound::onDelayFinished()
{
    if (!m_stop) {
        playSound();
    }
}

void VarioSound::updateVario(qreal vario)
{
    if (qAbs(vario - m_currentVario) < 0.05)
        return;

    m_currentVario = vario;
    calculateSoundCharacteristics(vario);

    if (m_mediaPlayer && m_audioOutput) {
        m_mediaPlayer->setPlaybackRate(m_currentPlaybackRate);
        m_audioOutput->setVolume(m_currentVolume);
    }

    if (!m_delayTimer->isActive() && !m_stopTimer->isActive()) {
        playSound();
    }
}

void VarioSound::setStop(bool newStop)
{
    m_stop = newStop;
    if (m_stop) {
        m_stopTimer->stop();
        m_delayTimer->stop();
        m_mediaPlayer->stop();
    } else {
        calculateSoundCharacteristics(m_currentVario);
        if (!m_delayTimer->isActive() && !m_stopTimer->isActive()) {
            playSound();
        }
    }
}
