#ifndef VARIOSOUND_H
#define VARIOSOUND_H

#include <QThread>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QDebug>
#include <QFile>
#include <QTimer>
#include <QPointer>

class VarioSound : public QThread
{
    Q_OBJECT

public:
    explicit VarioSound(QObject *parent = nullptr);
    ~VarioSound();

    void setStop();
    void updateVario(qreal vario);

protected:
    void run() override;

private slots:
    void handlePlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void handleMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void handleErrorOccurred(QMediaPlayer::Error error, const QString &errorString);

private:
    void calculateSoundCharacteristics();
    void playSound();
    void stopSound();
    bool initializeAudio();

    QPointer<QMediaPlayer> m_mediaPlayer;
    QPointer<QAudioOutput> m_audioOutput;

    float m_climbToneOnThreshold;
    float m_sinkToneOnThreshold;

    qreal m_currentVario;
    float m_currentPlaybackRate;
    float m_currentVolume;
    long m_duration;
    long m_frequency;
    bool m_stop;
    bool m_mediaLoaded;
};

#endif // VARIOSOUND_H
