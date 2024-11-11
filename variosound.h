#ifndef VARIOSOUND_H
#define VARIOSOUND_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>
#include <QtMath>

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
    void handlePlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onDelayFinished();

private:
    void calculateSoundCharacteristics(qreal vario);

    QMediaPlayer *m_mediaPlayer{nullptr};
    QAudioOutput *m_audioOutput{nullptr};
    QTimer *m_delayTimer{nullptr};

    qreal m_currentVario{0.0};
    qreal m_currentPlaybackRate{1.0};
    qreal m_currentVolume{1.0};
    int m_silenceDuration{400};
    qint64 m_startPosition{500};
    qint64 m_duration{200};

    const qreal m_deadband{0.2};
    bool m_stop{false};
};

#endif // VARIOSOUND_H
