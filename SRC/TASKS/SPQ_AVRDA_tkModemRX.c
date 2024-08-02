
#include "SPQ_AVRDA.h"
#include "modem_lte.h"

//------------------------------------------------------------------------------
void tkModemRX(void * pvParameters)
{

	/*
     * Esta tarea se encarga de recibir datos del modem LTE y
     * guardarlos en un buffer lineal.
     * Si se llena, BORRA EL BUFFER Y SE PIERDE TODO
     * No debería pasar porque antes lo debería haber procesado y flusheado
     * pero por las dudas. 
     */

( void ) pvParameters;
uint8_t c = 0;
uint32_t ulNotificationValue;

    while ( ! starting_flag )
        vTaskDelay( ( TickType_t)( 100 / portTICK_PERIOD_MS ) );

    SYSTEM_ENTER_CRITICAL();
    task_running |= MODEMRX_WDG_gc;
    SYSTEM_EXIT_CRITICAL();
        
    lBchar_CreateStatic ( &modem_rx_lbuffer, modem_rx_buffer, MODEM_RX_BUFFER_SIZE );
     
    xprintf_P(PSTR("Starting tkModemRX..\r\n" ));
  
	// loop
	for( ;; )
	{
        u_kick_wdt(MODEMRX_WDG_gc);
       
        while ( modem_awake ) {
            c = '\0';	// Lo borro para que luego del un CR no resetee siempre el timer.
            // el read se bloquea 50ms. lo que genera la espera.
            while ( xfgetc( fdWAN, (char *)&c ) == 1 ) {
                // Si hay datos recividos, los encolo
                if ( ! lBchar_Put( &modem_rx_lbuffer, c) ) {
                    // Si el buffer esta lleno, la borro !!!!
                    lBchar_Flush(&modem_rx_lbuffer);
                }
            }
        }
        
        // Estoy en tickless
        ulNotificationValue = ulTaskNotifyTake(pdTRUE, ( TickType_t)(60000 / portTICK_PERIOD_MS ));
	}    
}
//------------------------------------------------------------------------------

