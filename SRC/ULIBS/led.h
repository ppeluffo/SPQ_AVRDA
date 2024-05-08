/* 
 * File:   led.h
 * Author: pablo
 *
 * Created on 8 de febrero de 2022, 11:33 AM
 */

#ifndef LED_H
#define	LED_H

#ifdef	__cplusplus
extern "C" {
#endif


//#include <xc.h> 
#include <avr/io.h>
#include "FreeRTOS.h"
#include "task.h"
    
#define LED_PORT	PORTC
#define LED_PIN_bm	PIN4_bm
#define LED_PIN_bp  PIN4_bp
    
//#define LED_PORT	PORTG
//#define LED_PIN_bm	PIN7_bm
//#define LED_PIN_bp  PIN7_bp
    
#define PRENDER_LED() ( LED_PORT.OUT |= LED_PIN_bm )
#define APAGAR_LED() ( LED_PORT.OUT &= ~LED_PIN_bm )
#define TOGGLE_LED()    ( LED_PORT.OUT ^= 1UL << LED_PIN_bp);
    
void LED_init(void);
void led_flash(void);


#ifdef	__cplusplus
}
#endif

#endif	/* LED_H */

