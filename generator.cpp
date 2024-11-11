#include "Generator.h"

Generator::Generator(const QAudioFormat &format, qint64 durationUs, int sampleRate, QObject *parent)
    : QIODevice(parent)
    , m_pos(0)
    , m_sampleRate(sampleRate)
{
    if (format.isValid())
        generateData(format, durationUs, sampleRate);
}

Generator::~Generator()
{
    stop();
}

void Generator::start()
{
    m_pos = 0;
    open(QIODevice::ReadOnly);
}

void Generator::stop()
{
    m_pos = 0;
    close();
}

void Generator::generateData(const QAudioFormat &format, qint64 durationUs, int sampleRate)
{
    const int channelBytes = format.bytesPerSample();
    const int sampleBytes = format.channelCount() * channelBytes;

    qint64 length = (format.sampleRate() * format.channelCount() * format.bytesPerSample())
                    * durationUs / 1000000; // Changed from 100000 to 1000000 for correct timing

    m_buffer.resize(length);
    unsigned char *ptr = reinterpret_cast<unsigned char *>(m_buffer.data());
    int sampleIndex = 0;

    while (length > 0) {
        // Generate sine wave
        const qreal x = qSin(2 * M_PI * sampleRate * qreal(sampleIndex % format.sampleRate()) / format.sampleRate());

        for (int i = 0; i < format.channelCount(); ++i) {
            switch (format.sampleFormat()) {
            case QAudioFormat::Int16: {
                qint16 value = static_cast<qint16>(x * 32767);
                memcpy(ptr, &value, sizeof(qint16));
                break;
            }
            case QAudioFormat::Int32: {
                qint32 value = static_cast<qint32>(x * 2147483647);
                memcpy(ptr, &value, sizeof(qint32));
                break;
            }
            case QAudioFormat::Float: {
                float value = static_cast<float>(x);
                memcpy(ptr, &value, sizeof(float));
                break;
            }
            default:
                return;
            }
            ptr += channelBytes;
            length -= channelBytes;
        }
        ++sampleIndex;
    }
}

qint64 Generator::readData(char *data, qint64 len)
{
    qint64 total = 0;
    if (!m_buffer.isEmpty()) {
        while (len - total > 0) {
            const qint64 chunk = qMin((m_buffer.size() - m_pos), len - total);
            memcpy(data + total, m_buffer.constData() + m_pos, chunk);
            m_pos = (m_pos + chunk) % m_buffer.size();
            total += chunk;
        }
    }
    return total;
}

qint64 Generator::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return 0;
}

qint64 Generator::bytesAvailable() const
{
    return m_buffer.size() + QIODevice::bytesAvailable();
}
