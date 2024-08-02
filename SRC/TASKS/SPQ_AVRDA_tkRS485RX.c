#include "SPQ_AVRDA.h"
#include "frtos_cmd.h"
#include "modbus.h"
#include "consignas.h"

#define RS485RX_BUFFER_SIZE 64

char rs485rx_buffer[RS485RX_BUFFER_SIZE];
lBuffer_s rs485rx_lbuffer;

void RS485_flush_RXbuffer(void);
uint16_t RS485_getRXCount(void);
char *RS485_get_RXBuffer(void);

//------------------------------------------------------------------------------
void tkRS485RX(void * pvParameters)
{

	// Esta tarea maneja la cola de datos de la UART RS485A que es donde
    // se coloca el bus MODBUS.
    // Cada vez que hay un dato lo pone el un buffer circular commsA_lbuffer
    // para su procesamiento externo.

( void ) pvParameters;
uint8_t c = 0;
uint32_t ulNotificationValue;

    while ( ! starting_flag )
        vTaskDelay( ( TickType_t)( 100 / portTICK_PERIOD_MS ) );

    SYSTEM_ENTER_CRITICAL();
    task_running |= RS485RX_WDG_gc;
    SYSTEM_EXIT_CRITICAL();
    
	vTaskDelay( ( TickType_t)( 500 / portTICK_PERIOD_MS ) );

    lBchar_CreateStatic ( &rs485rx_lbuffer, rs485rx_buffer, RS485RX_BUFFER_SIZE );
    
    /*
     * Este tarea recibe los datos del puerto A que es donde esta el bus modbus.
     * Debo inicializar el sistema modbus !!!
     */
    modbus_init( fdRS485_MODBUS, RS485RX_BUFFER_SIZE, RS485_flush_RXbuffer, RS485_getRXCount, RS485_get_RXBuffer  );
    //consigna_init( fdRS485_MODBUS, RS485RX_BUFFER_SIZE, RS485_flush_RXbuffer, RS485_getRXCount, RS485_get_RXBuffer  );
    
    // La variable tickless se fija externamente para controlar cuando entro y salgo del 
    // modo.
    RS485_SLEEP();
    
    xprintf_P(PSTR("Starting tkRS485..\r\n" ));
    
	// loop
	for( ;; )
	{
        
        u_kick_wdt(RS485RX_WDG_bp);
        
        while ( rs485_awake ) {
            c = '\0';	// Lo borro para que luego del un CR no resetee siempre el timer.
            // el read se bloquea 50ms. lo que genera la espera.
            while ( xfgetc( fdRS485, (char *)&c ) == 1 ) {
                lBchar_Put( &rs485rx_lbuffer, c);
            }
        
            //vTaskDelay( ( TickType_t)( 10 / portTICK_PERIOD_MS ) );
        }
        
        // Estoy en tickless
        ulNotificationValue = ulTaskNotifyTake(pdTRUE, ( TickType_t)(60000 / portTICK_PERIOD_MS ));
	}    
}
//------------------------------------------------------------------------------
void RS485_read_RXbuffer(void)
{
    xprintf_P(PSTR("RS485RX:%s\r\n"), &rs485rx_buffer[0] );
    xprintf_P(PSTR("RS485 count=%d\r\n"), RS485_getRXCount());
    RS485_flush_RXbuffer();
    
}
//------------------------------------------------------------------------------
void RS485_flush_RXbuffer(void)
{
    // Wrapper para usar en modbus_init
    
    lBchar_Flush( &rs485rx_lbuffer );
}
//------------------------------------------------------------------------------
uint16_t RS485_getRXCount(void)
{
    // Wrapper para usar en modbus_init
    
    return( lBchar_GetCount(&rs485rx_lbuffer) );
}
//------------------------------------------------------------------------------
char *RS485_get_RXBuffer(void)
{
    // Wrapper para usar en modbus_init
    // Devuelve el inicio del buffer.
    
    //return ( lBchar_get_buffer(&commsA_lbuffer) );
    return ( &rs485rx_buffer[0] );
}
//------------------------------------------------------------------------------
