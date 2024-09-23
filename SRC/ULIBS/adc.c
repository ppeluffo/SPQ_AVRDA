
#include "adc.h"

#define ADC_SAMPLES 32

uint16_t adc_buffer[32];

//------------------------------------------------------------------------------
/**
 * \brief Initialize ADC interface
 * If module is configured to disabled state, the clock to the ADC is disabled
 * if this is supported by the device's clock system.
 *
 * \return Initialization status.
 * \retval 0 the ADC init was successful
 * \retval 1 the ADC init was not successful
 */
int8_t ADC_init()
{

	//ADC0.CTRLB = ADC_SAMPNUM_NONE_gc; /* No accumulation */
    ADC0.CTRLB = ADC_SAMPNUM_ACC16_gc; // 16 cuentas
        
	ADC0.CTRLC = ADC_PRESC_DIV32_gc; /* CLK_PER divided by 2 */

	//ADC0.CTRLD = 0x0 << ADC_SAMPDLY_gp /* Sampling Delay Selection: 0x0 */
	//		 | ADC_INITDLY_DLY0_gc; /* Delay 0 CLK_ADC cycles */

    ADC0.CTRLD = ADC_INITDLY_DLY64_gc | ADC_SAMPDLY_DLY10_gc;
    
	ADC0.CTRLE = ADC_WINCM_NONE_gc; /* No Window Comparison */

	ADC0.DBGCTRL = 0 << ADC_DBGRUN_bp; /* Debug run: disabled */

	ADC0.EVCTRL = 0 << ADC_STARTEI_bp; /* Start Event Input Enable: disabled */

	ADC0.INTCTRL = 0 << ADC_RESRDY_bp /* Result Ready Interrupt Enable: disabled */
			 | 0 << ADC_WCMP_bp; /* Window Comparator Interrupt Enable: disabled */

	// ADC0.MUXPOS = ADC_MUXPOS_AIN1_gc; /* ADC input pin 0 */
        
	ADC0.SAMPCTRL = 0x0 << ADC_SAMPLEN_gp; /* Sample length: 0x0 */

	ADC0.WINHT = 0x0; /* Window Comparator High Threshold: 0x0 */

	ADC0.WINLT = 0x0; /* Window Comparator Low Threshold: 0x0 */

	ADC0.CTRLA = 1 << ADC_ENABLE_bp      /* ADC Enable: enabled */
	             | 0 << ADC_FREERUN_bp   /* ADC Freerun mode: disabled */
	             | ADC_RESSEL_12BIT_gc   /* 12-bit mode */
	             | 0 << ADC_RUNSTBY_bp   /* Run standby mode: disabled */
	             | 0 << ADC_LEFTADJ_bp   /* Left Adjust Result: disabled */
	             | 0 << ADC_CONVMODE_bp; /* Differential Mode Conversion: disabled */

    /*
     Para usar el ADC debemos configurar su voltaje de referencia.
     */
    VREF.ADC0REF = VREF_REFSEL_2V500_gc | VREF_ALWAYSON_bm;
    //VREF.ADC0REF = VREF_REFSEL_VREFA_gc | VREF_ALWAYSON_bm;
    
	return 0;
}

//------------------------------------------------------------------------------
/**
 * \brief Enable ADC_0
 * 1. If supported by the clock system, enables the clock to the ADC
 * 2. Enables the ADC module by setting the enable-bit in the ADC control register
 *
 * \return Nothing
 */
void ADC_enable()
{
	ADC0.CTRLA |= ADC_ENABLE_bm;
    vTaskDelay( ( TickType_t)( 100 ) );
}
//------------------------------------------------------------------------------
/**
 * \brief Disable ADC_0
 * 1. Disables the ADC module by clearing the enable-bit in the ADC control register
 * 2. If supported by the clock system, disables the clock to the ADC
 *
 * \return Nothing
 */
void ADC_disable()
{
	ADC0.CTRLA &= ~ADC_ENABLE_bm;
    vTaskDelay( ( TickType_t)( 100 ) );
}
//------------------------------------------------------------------------------
/**
 * \brief Start a conversion on ADC_0
 *
 * \param[in] channel The ADC channel to start conversion on
 *
 * \return Nothing
 */
void ADC_start_conversion(adc_0_channel_t channel)
{
	ADC0.CTRLA &= ~ADC_CONVMODE_bm;
	ADC0.MUXPOS  = channel;
	ADC0.COMMAND = ADC_STCONV_bm;
}
//------------------------------------------------------------------------------
/**
 * \brief Start a differential conversion on ADC_0
 *
 * \param[in] channel,channel1 The ADC channels to start conversion on
 *
 * \return Nothing
 */
void ADC_start_diff_conversion(adc_0_channel_t channel, adc_0_muxneg_channel_t channel1)
{
	ADC0.CTRLA |= ADC_CONVMODE_bm;
	ADC0.MUXPOS  = channel;
	ADC0.MUXNEG  = channel1;
	ADC0.COMMAND = ADC_STCONV_bm;
}
//------------------------------------------------------------------------------
/**
 * \brief Stop a conversion on ADC_0
 *
 * \return Nothing
 */
void ADC_stop_conversion()
{
	ADC0.COMMAND = ADC_SPCONV_bm;
} 
//------------------------------------------------------------------------------
/**
   * \brief Check if the ADC conversion is done
   *
   * \return The status of ADC converison done check
   * \retval true The ADC conversion is done
   * \retval false The ADC converison is not done
   */
bool ADC_is_conversion_done()
{
	return (ADC0.INTFLAGS & ADC_RESRDY_bm);
}
//------------------------------------------------------------------------------
/**
 * \brief Read a conversion result from ADC_0
 *
 * \return Conversion result read from the ADC_0 ADC module
 */
adc_result_t ADC_get_conversion_result(void)
{
    
 uint16_t adcVal;
 
    adcVal = ADC0.RES;
    adcVal = adcVal >> ADC_SHIFT_DIV16;
	return (adcVal);
}
//------------------------------------------------------------------------------
/**
 * \brief Start a conversion, wait until ready, and return the conversion result
 *
 * \return Conversion result read from the ADC_0 ADC module
 */
adc_result_t ADC_get_conversion(adc_0_channel_t channel)
{
	adc_result_t res;

	ADC_start_conversion(channel);
	while (!ADC_is_conversion_done())
		;
	res = ADC_get_conversion_result();
	ADC0.INTFLAGS |= ADC_RESRDY_bm;
	return res;
}
//------------------------------------------------------------------------------
/**
 * \brief Start a differential conversion, wait until ready, and return the conversion result
 *
 * \return Conversion result read from the ADC_0 ADC module
 */
diff_adc_result_t ADC_get_diff_conversion(adc_0_channel_t channel, adc_0_muxneg_channel_t channel1)
{
	diff_adc_result_t res;

	ADC_start_diff_conversion(channel, channel1);
	while (!ADC_is_conversion_done())
		;
	res = ADC_get_conversion_result();
	ADC0.INTFLAGS |= ADC_RESRDY_bm;
	return res;
} 
//------------------------------------------------------------------------------
/**
   * \brief Return the number of bits in the ADC conversion result
   *
   * \return The number of bits in the ADC conversion result
   */
uint8_t ADC_get_resolution()
{
	return (ADC0.CTRLA & ADC_RESSEL0_bm) ? 10 : 12;
}
//------------------------------------------------------------------------------
uint16_t ADC_read_single(adc_0_channel_t channel, bool debug)
{
    
uint16_t adc_acc;

    ADC_enable();
    adc_acc = ADC_get_conversion(channel); /* ADC input pin 12 */
    ADC_disable();
    if (debug) {
        xprintf_P(PSTR("ADC_RES=%u\r\n"), (uint16_t)(adc_acc) );
    }
    return( adc_acc);    
}
//------------------------------------------------------------------------------
uint16_t ADC_read_multiple(adc_0_channel_t channel, uint8_t samples, bool debug)
{
    
uint8_t i;
uint16_t adc = 0; 
float adc_acc = 0.0;

    if ( ( samples == 0) || (samples > 128)) {
        return(0);
    }

    // Muestreo

    ADC_enable();
    for (i=0; i<samples; i++) {
         adc = ADC_get_conversion(channel);
         adc_acc += (float)(adc);
         vTaskDelay( ( TickType_t)( 10 ) );
         if (debug) {
             xprintf_P(PSTR("s%02d=%u, %0.3f\r\n"), i, adc, adc_acc);
         }
    }
    ADC_disable();
        
    adc_acc /= samples;
    if (debug) {
        xprintf_P(PSTR("ADC_RES=%u\r\n"), (uint16_t)(adc_acc) );
    }
    return( (uint16_t) adc_acc);
    
}
//------------------------------------------------------------------------------
uint16_t ADC_read_sens3v3(void)
{
    
uint16_t adc_acc;

    adc_acc = ADC_read_multiple(ADC_MUXPOS_AIN12_gc, 64, false); /* ADC input pin 12 */
    return( adc_acc);    
}
//------------------------------------------------------------------------------
uint16_t ADC_read_sens12v(void)
{

uint16_t adc_acc;

    adc_acc = ADC_read_multiple(ADC_MUXPOS_AIN11_gc, 64, false);
    return( adc_acc);
    
}
//------------------------------------------------------------------------------
