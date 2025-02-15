#ifndef VARIOSOUND_H
#define VARIOSOUND_H

#include <QObject>
#include <QAudioOutput>
#include <QMediaDevices>
#include <QAudioSink>
#include <QTimer>
#include <QBuffer>
#include <memory>

class ContinuousAudioBuffer;

class VarioSound : public QObject {
    Q_OBJECT
public:
    explicit VarioSound(QObject *parent = nullptr);
    ~VarioSound();

    void start();
    void stop();
    void updateVario(qreal vario);

private slots:
    void handleAudioStateChanged(QAudio::State state);
    void generateNextBuffer();

private:
    void initializeAudio();
    //void generateTone(float frequency, int durationMs);
    void generateTone(float frequency, int durationMs);
    void calculateSoundCharacteristics();

    std::unique_ptr<QAudioSink> m_audioSink;
    QByteArray m_audioData;

    ContinuousAudioBuffer* m_audioBuffer;
    QTimer m_toneTimer;

    float m_climbToneOnThreshold{};
    float m_sinkToneOnThreshold{};
    qreal m_currentVario{};
    float m_currentVolume{};
    int m_duration{};
    float m_frequency{};
    bool m_isRunning{false};

    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int CHANNELS = 2;
};

#endif // VARIOSOUND_H
