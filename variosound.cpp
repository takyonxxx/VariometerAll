#include "variosound.h"
#include <QVector>
#include <algorithm>

struct ToneParameters {
    ToneParameters(float v = 0, int f = 0, int c = 0, int d = 0)
        : vario(v), frequency(f), cycleLength(c), dutyPercent(d) {}
    float vario;
    int frequency;
    int cycleLength;
    int dutyPercent;
};

VarioSound::VarioSound(QObject *parent)
    : QObject(parent)
{
    // Initialize media player
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);
    m_mediaPlayer->setSource(QUrl("qrc:/sounds/vario_tone.wav"));
    m_audioOutput->setVolume(1.0);

    // Create timers
    m_delayTimer = new QTimer(this);
    m_delayTimer->setSingleShot(true);
    connect(m_delayTimer, &QTimer::timeout, this, &VarioSound::onDelayFinished);

    m_stopTimer = new QTimer(this);
    m_stopTimer->setSingleShot(true);
    connect(m_stopTimer, &QTimer::timeout, this, &VarioSound::stopSound);

    // Connect to handle playback state changes
    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged,
            this, &VarioSound::handlePlaybackStateChanged);

    m_toneParameters = QList<ToneParameters>{
        ToneParameters(0.00f, 300, 300, 20),   // Very weak lift
        ToneParameters(0.05f, 350, 300, 60),   // Added intermediate point
        ToneParameters(0.10f, 400, 300, 100),  // Weak lift
        ToneParameters(0.50f, 475, 288, 102),  // Added intermediate point
        ToneParameters(0.80f, 510, 282, 104),  // Added intermediate point
        ToneParameters(1.16f, 550, 276, 104),  // Moderate lift
        ToneParameters(1.50f, 600, 266, 106),  // Added intermediate point
        ToneParameters(2.00f, 680, 254, 108),  // Added intermediate point
        ToneParameters(2.67f, 763, 242, 110),  // Strong lift
        ToneParameters(3.50f, 870, 224, 112),  // Added intermediate point
        ToneParameters(4.24f, 985, 206, 116),  // Very strong lift
        ToneParameters(5.00f, 1100, 184, 120), // Added intermediate point
        ToneParameters(6.00f, 1234, 160, 124), // Exceptional lift
        ToneParameters(7.00f, 1375, 140, 128), // Added intermediate point
        ToneParameters(8.00f, 1517, 120, 132), // Strong thermal
        ToneParameters(9.00f, 1650, 98, 136),  // Added intermediate point
        ToneParameters(10.00f, 1800, 76, 140)  // Maximum lift
    };

    // Set thresholds
    m_climbToneOnThreshold = 0.2f;
    m_climbToneOffThreshold = 0.15f;
    m_sinkToneOnThreshold = -2.5f;
    m_sinkToneOffThreshold = -2.5f;

    // Initial values
    m_currentVario = 0.0;
    m_currentPlaybackRate = 1.0;
    m_currentVolume = 1.0;
    m_duration = 0;
    m_silenceDuration = 0;
    m_stop = false;
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

ToneParameters VarioSound::interpolateParameters(float vario)
{
    // Special case for sink
    if (vario <= m_sinkToneOnThreshold) {
        return ToneParameters(vario, 200, 500, 0); // Continuous tone for sink
    }

    // Find the surrounding points for interpolation
    auto it = std::lower_bound(m_toneParameters.begin(), m_toneParameters.end(), vario,
                               [](const ToneParameters& a, float v) { return a.vario < v; });

    if (it == m_toneParameters.begin()) {
        return *it;
    }
    if (it == m_toneParameters.end()) {
        return m_toneParameters.back();
    }

    auto high = *it;
    auto low = *(--it);

    // Linear interpolation
    float t = (vario - low.vario) / (high.vario - low.vario);
    ToneParameters result;
    result.vario = vario;
    result.frequency = static_cast<int>(low.frequency + t * (high.frequency - low.frequency));
    result.cycleLength = static_cast<int>(low.cycleLength + t * (high.cycleLength - low.cycleLength)) / 2;
    result.dutyPercent = static_cast<int>(low.dutyPercent + t * (high.dutyPercent - low.dutyPercent)) / 2;

    return result;
}

void VarioSound::calculateSoundCharacteristics(qreal vario)
{
    bool shouldPlay = false;

    // Check thresholds
    if (vario >= m_climbToneOnThreshold) {
        shouldPlay = true;
    } else if (vario <= m_sinkToneOnThreshold) {
        shouldPlay = true;
    } else if (vario > m_climbToneOffThreshold && m_currentVario >= m_climbToneOnThreshold) {
        shouldPlay = true;  // Hysteresis for climb
    } else if (vario < m_sinkToneOffThreshold && m_currentVario <= m_sinkToneOnThreshold) {
        shouldPlay = true;  // Hysteresis for sink
    }

    if (!shouldPlay) {
        m_currentVolume = 0.0;
        m_duration = 0;
        m_silenceDuration = 0;
        return;
    }

    // Get interpolated parameters
    auto params = interpolateParameters(vario);

    // Calculate playback rate to achieve desired frequency
    float baseFreq = 600.0f;  // Base frequency of our WAV file
    m_currentPlaybackRate = static_cast<float>(params.frequency) / baseFreq;

    m_duration = static_cast<qint64>(params.cycleLength);

    // Set durations based on cycle length and duty cycle
    if (vario <= m_sinkToneOnThreshold) {       
        m_silenceDuration = 0;
    } else {
        m_silenceDuration = m_duration  * params.dutyPercent / 100.0f;
    }

    m_currentVolume = 1.0;
}

void VarioSound::stopSound()
{
    if (m_mediaPlayer && m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_mediaPlayer->pause();

        if (!m_stop && m_silenceDuration > 0) {
            m_delayTimer->start(m_silenceDuration);
        }
    }
}

void VarioSound::playSound()
{
    if (!m_mediaPlayer || m_stop || m_currentVolume < 0.01)
        return;

    m_stopTimer->stop();
    m_mediaPlayer->setPosition(0);
    m_mediaPlayer->play();

    if (m_duration > 0 && m_silenceDuration > 0) {
        m_stopTimer->start(m_duration);
    }
}

void VarioSound::handlePlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    if (state == QMediaPlayer::StoppedState && !m_stop) {
        if (m_silenceDuration > 0 && !m_delayTimer->isActive() && !m_stopTimer->isActive()) {
            m_delayTimer->start(m_silenceDuration);
        } else if (m_silenceDuration == 0) {
            // For continuous tone, immediately replay
            m_mediaPlayer->setPosition(0);
            m_mediaPlayer->play();
        }
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
