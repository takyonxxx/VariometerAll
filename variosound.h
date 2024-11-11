#ifndef VARIOSOUND_H
#define VARIOSOUND_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>
#include <QtMath>
#include <QVector>
#include <QThread>

struct ToneParameters;

class VarioSound : public QObject
{
    Q_OBJECT
public:
    explicit VarioSound(QObject *parent = nullptr);
    ~VarioSound();

    void updateVario(qreal vario);
    void setStop(bool newStop);

private slots:
    void playSound();
    void stopSound();
    void handlePlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onDelayFinished();

private:
    void calculateSoundCharacteristics(qreal vario);
    ToneParameters interpolateParameters(float vario);

    QMediaPlayer *m_mediaPlayer{nullptr};
    QAudioOutput *m_audioOutput{nullptr};
    QTimer *m_delayTimer{nullptr};
    QTimer *m_stopTimer{nullptr};

    QVector<ToneParameters> m_toneParameters;

    qreal m_currentVario{0.0};
    qreal m_currentPlaybackRate{1.0};
    qreal m_currentVolume{1.0};
    qint64 m_duration{0};
    int m_silenceDuration{0};
    bool m_stop{false};

    // Thresholds
    float m_climbToneOnThreshold;
    float m_climbToneOffThreshold;
    float m_sinkToneOnThreshold;
    float m_sinkToneOffThreshold;
};

#endif // VARIOSOUND_H
