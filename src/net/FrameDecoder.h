#pragma once

#include "MessageHeader.h"

#include <QByteArray>
#include <QVector>

// TCP 스트림 → 프레임 분리 (부분 수신 / 여러 프레임 병합 수신 처리)
//  수신 버퍼 누적 → 헤더 8B 확보 → DataSize 만큼 확보 시 프레임 방출
//  DataSize 가 최대 크기를 넘으면 오류 + 버퍼 리셋 (스트림 동기 상실)
class FrameDecoder {
public:
    struct Frame {
        MessageHeader header;
        QByteArray payload;
        QByteArray raw;   // 헤더 + payload (로그 hex dump 용)
    };

    explicit FrameDecoder(quint32 maxPayload);

    QVector<Frame> feed(const QByteArray& chunk);
    void reset();

    bool hadError() const { return m_errors > 0; }
    int errorCount() const { return m_errors; }
    QString lastError() const { return m_lastError; }
    int buffered() const { return int(m_buf.size()); }

private:
    quint32 m_maxPayload;
    QByteArray m_buf;
    int m_errors = 0;
    QString m_lastError;
};
