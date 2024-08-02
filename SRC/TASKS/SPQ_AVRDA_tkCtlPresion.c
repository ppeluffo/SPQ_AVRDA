/*
 * File:   tkPiloto.c
 * Author: pablo
 *
 */


#include "SPQ_AVRDA.h"


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
    
    CPRES_IS_ON = false;
    
    if ( systemConf.ptr_consigna_conf->enabled ) {
        cpres_consigna_initService();          
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
        
        // Siempre controlo apagar el controlador de presion en modo discreto
        if ( CPRES_IS_ON && (u_get_sleep_time(false) > 0 )) {
            // Espero 10s que se apliquen las consignas y apago el modulo
            vTaskDelay( ( TickType_t)( 10000 / portTICK_PERIOD_MS ) );
            CLEAR_EN_PWR_CPRES(); 
            CPRES_IS_ON = false;
        }
        
        if ( systemConf.ptr_consigna_conf->enabled ) {
            cpres_consigna_service();
        }
               
	}
}
//------------------------------------------------------------------------------
