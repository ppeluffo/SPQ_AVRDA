
#include "toyi_valves.h"

// -----------------------------------------------------------------------------
void VALVE_EN_init(void)
{
    // Configura el pin como output
	VALVE_EN_PORT.DIR |= VALVE_EN_PIN_bm;	
	DISABLE_VALVE();
}
// -----------------------------------------------------------------------------
void VALVE_CTRL_init(void)
{
    // Configura el pin como output
	VALVE_CTRL_PORT.DIR |= VALVE_CTRL_PIN_bm;	

}
// -----------------------------------------------------------------------------
void VALVE_init(void)
{
    VALVE_EN_init();
    VALVE_CTRL_init();
}
// -----------------------------------------------------------------------------
t_valve_status get_valve_status(void)
{
    return valve_status;
}
// -----------------------------------------------------------------------------
void VALVE_open(void)
{
    OPEN_VALVE();
    ENABLE_VALVE();
     
    vTaskDelay( ( TickType_t)( 10000 / portTICK_PERIOD_MS ) );
    DISABLE_VALVE();
    valve_status = VALVE_OPEN;
}
// -----------------------------------------------------------------------------
void VALVE_close(void)
{
    CLOSE_VALVE();
    ENABLE_VALVE();
    
    vTaskDelay( ( TickType_t)( 10000 / portTICK_PERIOD_MS ) );
    DISABLE_VALVE();
    valve_status = VALVE_CLOSE;
}
//-----------------------------------------------------------------------------
void valve_print_configuration( void )
{        
    xprintf_P(PSTR("VALVES\r\n"));
    return;
    
    if ( valve_status == VALVE_OPEN) {
        xprintf_P( PSTR("Valve: OPEN\r\n"));
    } else {
        xprintf_P( PSTR("Valve: CLOSE\r\n"));
    }
  
}
//------------------------------------------------------------------------------
