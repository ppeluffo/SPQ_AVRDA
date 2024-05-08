/* 
 * File:   adc.h
 * Author: pablo
 *
 * Created on 27 de julio de 2022, 12:14 PM
 */

#ifndef ADC_H
#define	ADC_H

#ifdef	__cplusplus
extern "C" {
#endif

    
#include <avr/io.h>
#include "stdint.h"
#include "stdbool.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "pines.h"
#include "task.h"
    
typedef void (*adc_irq_cb_t)(void);

/** Datatype for the result of the ADC conversion */
typedef uint16_t adc_result_t;

typedef int16_t diff_adc_result_t;

//* Analog channel selection */
typedef ADC_MUXPOS_t adc_0_channel_t;

typedef ADC_MUXNEG_t adc_0_muxneg_channel_t;

int8_t ADC_init();

void ADC_enable();

void ADC_disable();

void ADC_start_conversion(adc_0_channel_t channel);

void ADC_start_diff_conversion(adc_0_channel_t channel, adc_0_muxneg_channel_t channel1);
void ADC_stop_conversion();

bool ADC_is_conversion_done();

adc_result_t ADC_get_conversion_result(void);

adc_result_t ADC_get_conversion(adc_0_channel_t channel);

diff_adc_result_t ADC_get_diff_conversion(adc_0_channel_t channel, adc_0_muxneg_channel_t channel1);

uint8_t ADC_get_resolution();

uint16_t ADC_read(uint8_t samples);
uint16_t ADC_read_sens3v3(void);
uint16_t ADC_read_sens12v(void);


#ifdef	__cplusplus
}
#endif

#endif	/* ADC_H */

