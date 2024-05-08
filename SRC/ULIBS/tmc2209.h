/* 
 * File:   tmc2209.h
 * Author: pablo
 *
 * Created on 22 de enero de 2024, 12:10 PM
 */

#ifndef TMC2209_H
#define	TMC2209_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <avr/io.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "stdint.h"
    
typedef enum { DIR_FW=0, DIR_REV } t_tmc2209_dir;
typedef enum { RUNNING=0, STOPPED } t_tmc2209_status;

#define TMC2209_EN_PORT     PORTC
#define TMC2209_EN_PIN_bm	PIN5_bm
#define TMC2209_EN_PIN_bp   PIN5_bp
    
#define TMC2209_DISABLE()    ( TMC2209_EN_PORT.OUT |= TMC2209_EN_PIN_bm )
#define TMC2209_ENABLE()   ( TMC2209_EN_PORT.OUT &= ~TMC2209_EN_PIN_bm )
    
void TMC2209_EN_init(void);

#define TMC2209_DIR_PORT        PORTB
#define TMC2209_DIR_PIN_bm      PIN2_bm
#define TMC2209_DIR_PIN_bp      PIN2_bp
    
#define TMC2209_DIR_FORWARD()    ( TMC2209_DIR_PORT.OUT |= TMC2209_DIR_PIN_bm )
#define TMC2209_DIR_REVERSE()   ( TMC2209_DIR_PORT.OUT &= ~TMC2209_DIR_PIN_bm )
    
void TMC2209_DIR_init(void);

#define TMC2209_STEP_PORT       PORTB
#define TMC2209_STEP_PIN_bm     PIN3_bm
#define TMC2209_STEP_PIN_bp     PIN3_bp
    
#define TMC2209_STEP_ON()    ( TMC2209_STEP_PORT.OUT |= TMC2209_STEP_PIN_bm )
#define TMC2209_STEP_OFF()   ( TMC2209_STEP_PORT.OUT &= ~TMC2209_STEP_PIN_bm )
#define TMC2209_STEP_TOGGLE()    ( TMC2209_STEP_PORT.OUT ^= 1UL << TMC2209_STEP_PIN_bp);

void TMC2209_STEP_init(void);

void TMC2209_init(void);
void tmc2209_start(t_tmc2209_dir dir, uint8_t period, uint16_t pulse_counts);
void tmc2209_stop(void);

#ifdef	__cplusplus
}
#endif

#endif	/* TMC2209_H */

