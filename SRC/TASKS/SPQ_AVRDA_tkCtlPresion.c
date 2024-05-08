/*
 * File:   tkPiloto.c
 * Author: pablo
 *
 */


#include "SPQ_AVRDA.h"

void CONSIGNA_initService(void);
void CONSIGNA_service(void);

//------------------------------------------------------------------------------
void tkCtlPresion(void * pvParameters)
{

	/*
     * Tarea que implementa el sistema de piloto para controlar una
     * valvula reguladora.
     * Impleentamos un modelo productor - consumidor.
     * 
     */
    
( void ) pvParameters;


	while (! starting_flag )
		vTaskDelay( ( TickType_t)( 200 / portTICK_PERIOD_MS ) );

    SYSTEM_ENTER_CRITICAL();
    task_running |= CTLPRES_WDG_gc;
    SYSTEM_EXIT_CRITICAL();
    
	vTaskDelay( ( TickType_t)( 500 / portTICK_PERIOD_MS ) );
    xprintf_P(PSTR("Starting tkCtlPresion..\r\n"));
    
    // Espero que todo este arrancado (30s)
    vTaskDelay( ( TickType_t)( 30000 / portTICK_PERIOD_MS ) );
    
    if ( systemConf.ptr_consigna_conf->enabled ) {
        CONSIGNA_initService();
    }
    
	for( ;; )
	{
        /*
         * Corre cada 1 minuto porque el timeslot se mide como hhmm y no queremos
         * que se realmacene la orden de un mismo tslot
         * 
         */
        u_kick_wdt(CTLPRES_WDG_gc);
		vTaskDelay( ( TickType_t)( 30000 / portTICK_PERIOD_MS ) );
        
        if ( systemConf.ptr_consigna_conf->enabled ) {
            CONSIGNA_service();
        }
        
	}
}
//------------------------------------------------------------------------------
void CONSIGNA_initService(void)
{
    /*
     * Determino en base a la fecha y hora actual, que consigna pongo
     * en el arranque
     *
     */
   
RtcTimeType_t rtc;
uint16_t now;
uint16_t c_dia;
uint16_t c_noche;

    // Al arrancar prendemos el modulo de control de presion.
    SET_EN_PWR_CPRES();
    vTaskDelay( ( TickType_t)( 2000 / portTICK_PERIOD_MS ) );
    
    RTC_read_dtime(&rtc);
    now = rtc.hour * 100 + rtc.min;
    c_dia = systemConf.ptr_consigna_conf->consigna_diurna;
    c_noche = systemConf.ptr_consigna_conf->consigna_nocturna;
    
    // Ajusto todo a minutos.
    now = u_hhmm_to_mins(now);
    c_dia = u_hhmm_to_mins( c_dia );
    c_noche = u_hhmm_to_mins( c_noche );
    
    if (c_dia > c_noche ) {
        xprintf_P(PSTR("ERROR: No puedo determinar consigna inicial (c_dia > c_noche)\r\n"));
        // Aplico la nocturna que es la que deberia tener menos presion
        xprintf_P(PSTR("CONSIGNA Init:nocturna %04d\r\n"),now);
        XCOMMS_ENTER_CRITICAL();
		consigna_set_nocturna();
        XCOMMS_EXIT_CRITICAL();
        goto exit;
		return;
    }
    
    if ( ( now <= c_dia ) || ( c_noche <= now ) ) {
        // Aplico consigna nocturna
         xprintf_P(PSTR("CONSIGNA Init:nocturna %04d\r\n"),now);
        XCOMMS_ENTER_CRITICAL();
		consigna_set_nocturna();
        XCOMMS_EXIT_CRITICAL();
        goto exit;
		return;  
        
    } else {
        // Aplico consigna diurna
         xprintf_P(PSTR("CONSIGNA Init:diurna %04d\r\n"),now);
        XCOMMS_ENTER_CRITICAL();
		consigna_set_diurna();
        XCOMMS_EXIT_CRITICAL();
        goto exit;
		return;
    }
    
exit:

    // Espero 10s que se apliquen las consignas y apago el modulo
    // Solo apago si estoy en modo discreto
    if ( u_get_sleep_time(false) > 0 ){
        // Espero 10s que se apliquen las consignas y apago el modulo
        vTaskDelay( ( TickType_t)( 10000 / portTICK_PERIOD_MS ) );
        CLEAR_EN_PWR_CPRES(); 
    }
  
}
//------------------------------------------------------------------------------
void CONSIGNA_service(void)
{
 
RtcTimeType_t rtcDateTime;
uint16_t now;
         
	// Chequeo y aplico.
	// Las consignas se chequean y/o setean en cualquier modo de trabajo, continuo o discreto
	memset( &rtcDateTime, '\0', sizeof(RtcTimeType_t));
	if ( ! RTC_read_dtime(&rtcDateTime) ) {
		xprintf_P(PSTR("CONSIGNA ERROR: I2C:RTC chequear_consignas\r\n\0"));
		return;
	}

    now = rtcDateTime.hour * 100 + rtcDateTime.min;
    
	// Consigna diurna ?
	if ( now == consigna_conf.consigna_diurna  ) {
        
        // Siempre prendo: Si esta prendido no pasa nada
        SET_EN_PWR_CPRES();
        vTaskDelay( ( TickType_t)( 2000 / portTICK_PERIOD_MS ) );
            
        XCOMMS_ENTER_CRITICAL();
		consigna_set_diurna();
        XCOMMS_EXIT_CRITICAL();
		xprintf_P(PSTR("Set CONSIGNA diurna %04d\r\n"), now );
        
        // Solo apago si estoy en modo discreto
        if ( u_get_sleep_time(true) > 0 ){
            // Espero 10s que se apliquen las consignas y apago el modulo
            vTaskDelay( ( TickType_t)( 10000 / portTICK_PERIOD_MS ) );
            CLEAR_EN_PWR_CPRES(); 
        }  
		return;
	}

	// Consigna nocturna ?
	if ( now == consigna_conf.consigna_nocturna  ) {
        
        SET_EN_PWR_CPRES();
        vTaskDelay( ( TickType_t)( 2000 / portTICK_PERIOD_MS ) );
        
        XCOMMS_ENTER_CRITICAL();
		consigna_set_nocturna();
        XCOMMS_EXIT_CRITICAL();
		xprintf_P(PSTR("Set CONSIGNA nocturna %04d\r\n"),now);
        
        // Solo apago si estoy en modo discreto
        if ( u_get_sleep_time(true) > 0 ){
            // Espero 10s que se apliquen las consignas y apago el modulo
            vTaskDelay( ( TickType_t)( 10000 / portTICK_PERIOD_MS ) );
            CLEAR_EN_PWR_CPRES(); 
        }
		return;
	}
    
}
//------------------------------------------------------------------------------

