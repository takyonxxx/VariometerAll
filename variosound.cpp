#include "variosound.h"

VarioSound::VarioSound(QObject *parent)
    : QObject(parent)
{
    // Initialize media player
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);

    // Load WAV file from resources
    m_mediaPlayer->setSource(QUrl("qrc:/sounds/554Hz.wav"));
    m_audioOutput->setVolume(1.0);

    // Create timer for managing silence between beeps
    m_delayTimer = new QTimer(this);
    m_delayTimer->setSingleShot(true);
    connect(m_delayTimer, &QTimer::timeout, this, &VarioSound::onDelayFinished);

    // Connect to handle playback state changes
    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged,
            this, &VarioSound::handlePlaybackStateChanged);

    // Set initial position to play only a short segment
    m_startPosition = 500;  // Start at 500ms
    m_duration = 200;      // Play for 200ms
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
    if (m_audioOutput) {
        delete m_audioOutput;
    }
}

void VarioSound::calculateSoundCharacteristics(qreal vario)
{
    if (qAbs(vario) < m_deadband) {
        m_currentPlaybackRate = 1.0;
        m_currentVolume = 0.0;
        m_silenceDuration = 500;
        return;
    }

    if (vario > 0) {
        // Increase playback rate with climb rate
        m_currentPlaybackRate = 1.0 + (vario * 0.2); // 20% faster per m/s
        m_currentVolume = 1.0;
        // Decrease silence as vario increases (faster beeps)
        m_silenceDuration = static_cast<int>(400 - (vario * 50));  // 400ms to 150ms
        m_silenceDuration = std::max(150, m_silenceDuration);
    } else {
        // Decrease playback rate with sink rate
        m_currentPlaybackRate = 1.0 + (vario * 0.1); // 10% slower per m/s
        m_currentVolume = 0.8;
        m_silenceDuration = 400;  // Constant silence for sink
    }
}

void VarioSound::playSound()
{
    if (!m_mediaPlayer || m_stop)
        return;

    m_mediaPlayer->setPosition(m_startPosition);
    m_mediaPlayer->play();

    // Stop after duration
    QTimer::singleShot(m_duration, this, [this]() {
        if (m_mediaPlayer) {
            m_mediaPlayer->stop();
        }
    });
}

void VarioSound::handlePlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    if (state == QMediaPlayer::StoppedState && !m_stop) {
        // Start the silence period
        m_delayTimer->start(m_silenceDuration);
    }
}

void VarioSound::onDelayFinished()
{
    // When the silence period is over, play the next beep if we're not stopped
    if (!m_stop) {
        playSound();
    }
}

void VarioSound::updateVario(qreal vario)
{
    m_currentVario = vario;
    calculateSoundCharacteristics(vario);

    // Update player settings
    if (m_mediaPlayer && m_audioOutput) {
        m_mediaPlayer->setPlaybackRate(m_currentPlaybackRate);
        m_audioOutput->setVolume(m_currentVolume);
    }

    if (!m_delayTimer->isActive()) {
        playSound();
    }
}

void VarioSound::setStop(bool newStop)
{
    m_stop = newStop;
    if (m_stop) {
        if (m_delayTimer) {
            m_delayTimer->stop();
        }
        if (m_mediaPlayer) {
            m_mediaPlayer->stop();
        }
    } else {
        calculateSoundCharacteristics(m_currentVario);
        if (!m_delayTimer->isActive()) {
            playSound();
        }
    }
}
