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
    tk_running[TK_CTLPRES] = true;
    SYSTEM_EXIT_CRITICAL();
    
	vTaskDelay( ( TickType_t)( 500 / portTICK_PERIOD_MS ) );
    xprintf_P(PSTR("Starting tkCtlPresion..\r\n"));
    
    //VALVE_open();
    
    // Espero que todo este arrancado (30s)
    vTaskDelay( ( TickType_t)( 30000 / portTICK_PERIOD_MS ) );
     
    
	for( ;; )
	{
        /*
         * Corre cada 1 minuto porque el timeslot se mide como hhmm y no queremos
         * que se realmacene la orden de un mismo tslot
         * 
         */
        u_kick_wdt(TK_CTLPRES);
		vTaskDelay( ( TickType_t)( 30000 / portTICK_PERIOD_MS ) );
        
               
	}
}
//------------------------------------------------------------------------------
