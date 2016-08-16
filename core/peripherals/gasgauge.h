#ifndef GASGAUGE_H_
#define GASGAUGE_H_

#include "isl6295.h"
#include "types.h"

#define GASGAUGE_STATE_NO_CHARGE    0
#define GASGAUGE_STATE_CHARGING     1
#define GASGAUGE_STATE_CHARGED      2

typedef struct {
    uint8_t state;
    int32_t voltage;
    int32_t current;
} gasgauge_stats;

#endif // GASGAUGE_H_

