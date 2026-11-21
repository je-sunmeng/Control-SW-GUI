#pragma once

#include "Types.h"

// 오브젝트 ID 발급기 (1부터 증가, 0은 "없음")
class IdGenerator {
public:
    ObjectId next() { return ++m_last; }
    ObjectId last() const { return m_last; }
    void reset() { m_last = 0; }

private:
    ObjectId m_last = 0;
};
