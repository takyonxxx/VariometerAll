#ifndef VARIOSOUND_H
#define VARIOSOUND_H

#include <QObject>
#include <QAudioOutput>
#include <QMediaDevices>
#include <QAudioSink>
#include <QTimer>
#include <QBuffer>
#include <memory>

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
    void generateTone(float frequency, int durationMs);
    void calculateSoundCharacteristics();

    std::unique_ptr<QAudioSink> m_audioSink;
    QByteArray m_audioData;
    QBuffer m_audioBuffer;
    QTimer m_toneTimer;

    float m_climbToneOnThreshold{0.2f};
    float m_sinkToneOnThreshold{-2.0f};
    qreal m_currentVario{0.0};
    float m_currentVolume{1.0};
    int m_duration{300};
    float m_frequency{600.0f};
    bool m_isRunning{false};

    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int CHANNELS = 1;
    static constexpr int SAMPLE_SIZE = 16;
};

#endif // VARIOSOUND_H
