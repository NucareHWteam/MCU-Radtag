#ifndef __FILTER_H
#define __FILTER_H

#include <stdint.h>
#include <stdbool.h>

#define FILTER_MAX_WINDOW    64
#define ALPHA_MIN            0.1f
#define ALPHA_MAX            0.4f
#define ALPHA_SCALE          0.05f

/** 윈도우 크기 설정 */
void    Filter_Init(uint16_t winSize);

/**
 * @brief  새 샘플 입력 → 블록평균, EMA 계산
 * @param  sample    : 최근 측정값
 * @param  pBlockAvg : (out) 현재 블록평균
 * @param  pEma      : (out) EMA 값
 * @return true이면 EMA가 유효(블록이 최소 한 번 채워진 이후), false면 아직 EMA 미준비
 */
bool    Filter_Update(float sample, float *pBlockAvg, float *pEma);

#endif // __FILTER_H
