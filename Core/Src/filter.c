#include "filter.h"
#include <string.h>
#include <math.h>

static float   buf[FILTER_MAX_WINDOW];
static uint16_t windowSize, bufIndex, sampleCount;
static float   emaPrev;
static bool    emaInit;


// 추가: EMA 업데이트 여부 저장
static bool blockReady = false;

/////////////////////Kalman
static float kalmanPrev;
static bool kalmanInit;

void Filter_Kalman_Init(void)
{
    kalmanPrev = 0.0f;
    kalmanInit = false;
}

float Filter_Update_Kalman(float sample)
{
    float y;
    if (!kalmanInit) {
        kalmanPrev = sample;
        kalmanInit = true;
        return kalmanPrev;
    }
    float delta = fabsf(sample - kalmanPrev);
    float alpha = 0.05f * delta;
    if (alpha < 0.1f) alpha = 0.1f;
    if (alpha > 0.4f) alpha = 0.4f;

    y = alpha * sample + (1.0f - alpha) * kalmanPrev;
    kalmanPrev = y;
    return y;
}


void Filter_Init(uint16_t winSize)
{
    if (winSize == 0 || winSize > FILTER_MAX_WINDOW)
        winSize = FILTER_MAX_WINDOW;
    windowSize  = winSize;
    bufIndex    = sampleCount = 0;
    emaInit     = false;
    emaPrev     = 0.0f;
    blockReady  = false;
    memset(buf, 0, sizeof(buf));
}

bool Filter_Update(float sample, float *pBlockAvg, float *pEma)
{
    buf[bufIndex++] = sample;
    if (bufIndex >= windowSize) bufIndex = 0;
    if (sampleCount < windowSize) sampleCount++;

    float sum = 0.f;
    for (uint16_t i = 0; i < sampleCount; i++)
        sum += buf[i];
    float avg = sum / (float)sampleCount;
    *pBlockAvg = avg;

    // blockReady: 새로운 윈도우가 완전히 찼을 때만 true
    bool blockComplete = (sampleCount >= windowSize) && (bufIndex == 0);

    if (blockComplete) {
        // 새로운 블록 평균이 준비되었으니 EMA를 업데이트
        float delta = emaInit ? fabsf(avg - emaPrev) : 0.f;
        float alpha = ALPHA_SCALE * delta;
        if      (alpha < ALPHA_MIN) alpha = ALPHA_MIN;
        else if (alpha > ALPHA_MAX) alpha = ALPHA_MAX;

        if (!emaInit) {
            emaPrev = avg;
            emaInit = true;
        } else {
            emaPrev = alpha * avg + (1.f - alpha) * emaPrev;
        }
        *pEma = emaPrev;
        blockReady = true; // BlockAvg/EMA 유효 신호
        sampleCount = 0;
        bufIndex    = 0;
    } else {
        *pEma = emaPrev; // 값은 전달하되, 아직 새로운 EMA는 아님
        blockReady = false;
    }

    return blockReady;
}


void Filter_ForceEMA(float* pBlockAvg, float* pEma)
{
    float sum = 0.f;
    for (uint16_t i = 0; i < sampleCount; i++)
        sum += buf[i];
    float avg = (sampleCount > 0) ? (sum / sampleCount) : 0.0f;
    *pBlockAvg = avg;

    float alpha = 2.0f / (sampleCount + 1);
    if (!emaInit) {
        emaPrev = avg;
        emaInit = true;
    } else {
        emaPrev = alpha * avg + (1 - alpha) * emaPrev;
    }
    *pEma = emaPrev;
}
