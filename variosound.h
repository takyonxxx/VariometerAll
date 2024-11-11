#ifndef VARIOSOUND_H
#define VARIOSOUND_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>
#include <QtMath>
#include <QThread>

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

    QMediaPlayer *m_mediaPlayer{nullptr};
    QAudioOutput *m_audioOutput{nullptr};
    QTimer *m_delayTimer{nullptr};
    QTimer *m_stopTimer{nullptr};

    qreal m_currentVario{0.0};
    qreal m_currentPlaybackRate{1.0};
    qreal m_currentVolume{1.0};
    int m_silenceDuration{350};
    qint64 m_startPosition{30};
    qint64 m_duration{30};
    int m_initial_silenceDuration{0};
    qint64 m_initial_duration{0};
    const qreal m_deadband{0.2};
    bool m_stop{false};
};

#endif // VARIOSOUND_H
