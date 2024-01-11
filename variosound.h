#ifndef VARIOSOUND_H
#define VARIOSOUND_H

#include <QAudioOutput>
#include <QMediaDevices>
#include <QAudioFormat>
#include <QAudioSink>
#include <QAudioSource>
#include <QBuffer>
#include <QThread>
#include <QtMath>
#include <QDebug>


class VarioSound : public QThread
{
    Q_OBJECT

public:
    VarioSound(QObject *parent = nullptr);
    ~VarioSound();

#include <cmath>
#include <QByteArray>

    QByteArray generateSineWave(int durationInSeconds, int sampleRate, int channelCount, double baseFrequency, qreal vario) {
        const int numFrames = durationInSeconds * sampleRate;
        const int numChannels = channelCount;

        QByteArray buffer;
        buffer.resize(numFrames * numChannels * sizeof(short));

        short* data = reinterpret_cast<short*>(buffer.data());
        const double amplitude = 0.5 * SHRT_MAX;

        double frequency = baseFrequency;  // Initial frequency

        for (int i = 0; i < numFrames; ++i) {
            double t = static_cast<double>(i) / sampleRate;

            // Smooth modulation of frequency based on vario using tanh function
            double modulationFactor = std::tanh(vario);  // Adjust the tanh scaling factor as needed
            frequency = baseFrequency + modulationFactor * vario;

            for (int channel = 0; channel < numChannels; ++channel) {
                double value = amplitude * std::sin(2.0 * M_PI * frequency * t);
                data[i * numChannels + channel] = static_cast<short>(value);
            }
        }

        return buffer;
    }

    void updateVario(qreal vario);

    void setStop(bool newStop);

private slots:
    void handleStateChanged(QAudio::State);
    void playSound();

private:   
    QAudioSink *m_audioOutput;
    QBuffer m_audioOutputIODevice;
    qreal frequency = 440.0;
    int phase = 0;
    qreal currentVario = 0.0;
    int sampleRate = 44100;
    bool m_stop;

protected:
    void run() override;
};

#endif // VARIOSOUND_H
