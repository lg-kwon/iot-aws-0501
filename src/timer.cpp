

#include <Arduino.h>

// 시간 구조체를 한국 로컬 시간으로 변환하는 함수
void convertToKoreanTime(struct tm* timeinfo) {
    // 한국 표준시(UTC+9)에 대한 오프셋 추가
    timeinfo->tm_hour += 9;
    // 24시간 형식으로 변환
    if (timeinfo->tm_hour >= 24) {
        timeinfo->tm_hour -= 24;
    }
}