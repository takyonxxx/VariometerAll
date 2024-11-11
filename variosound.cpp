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
    m_startPosition = 30;
    m_initial_duration = 250;
    m_initial_silenceDuration = 75;
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
        // No sound in the deadband
        m_currentPlaybackRate = 1.0;
        m_currentVolume = 0.0;
        m_duration = m_initial_duration;
        m_silenceDuration = m_initial_silenceDuration;
        return;
    }

    if (vario > 0) {
        // Climb sound characteristics
        m_currentPlaybackRate = qMin(1.0 + (vario * 0.08), 2.5);  // Smoother rate change
        m_currentVolume = qMin(0.8 + (vario * 0.05), 1.0);

        m_duration = static_cast<qint64>(m_initial_duration - (vario * 50));
        m_duration = qMax(static_cast<qint64>(50), m_duration);

        m_silenceDuration = static_cast<int>(m_initial_silenceDuration - (vario * 10));
        m_silenceDuration = qMax(5, m_silenceDuration);
    } else {
        // Sink sound characteristics
        m_currentPlaybackRate = qMax(1.0 + (vario * 0.04), 0.8);  // Lower rate change for sinking
        m_currentVolume = 0.6;  // Fixed lower volume for descent
        m_duration = m_initial_duration;
        m_silenceDuration = m_initial_silenceDuration;
    }
}

void VarioSound::updateVario(qreal vario)
{
    if (vario < 0.2)
        return;

    if (qAbs(vario - m_currentVario) < 0.02)  // Smaller sensitivity for smoother response
        return;

    m_currentVario = vario;
    calculateSoundCharacteristics(vario);

    // Apply new playback rate and volume with smoother transitions
    if (m_mediaPlayer && m_audioOutput) {
        m_mediaPlayer->setPlaybackRate(m_currentPlaybackRate);
        m_audioOutput->setVolume(m_currentVolume);
    }

    if (!m_delayTimer->isActive() && !m_stopTimer->isActive()) {
        playSound();
    }
}


void VarioSound::stopSound()
{
    if (m_mediaPlayer && m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_mediaPlayer->stop();  // Use pause instead of stop to prevent abrupt stops

        if (!m_stop) {
            // Wait for the silence duration before resuming playback
            m_delayTimer->start(m_silenceDuration);
        }
    }
}

void VarioSound::playSound()
{
    if (!m_mediaPlayer || m_stop)
        return;

    // Avoid repeatedly stopping the player; instead, adjust position if needed
    if (m_mediaPlayer->playbackState() != QMediaPlayer::PlayingState) {
        m_mediaPlayer->setPosition(m_startPosition);
        m_mediaPlayer->play();
    }

    // Start the stop timer based on the duration for smoother control over stop timing
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
