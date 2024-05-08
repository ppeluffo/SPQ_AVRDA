
#include "tmc2209.h"

TimerHandle_t tmc2209_xTimer;
StaticTimer_t tmc2209_xTimerBuffer;
void tmc2209_TimerCallback( TimerHandle_t xTimer );

uint16_t tmc_2209_counter;
t_tmc2209_status tmc2209_status;

// ---------------------------------------------------------------
void TMC2209_EN_init(void)
{
	// Configura el pin del led como output
	TMC2209_EN_PORT.DIR |= TMC2209_EN_PIN_bm;	
	TMC2209_DISABLE();
}
// ---------------------------------------------------------------
void TMC2209_DIR_init(void)
{
	// Configura el pin del led como output
	TMC2209_DIR_PORT.DIR |= TMC2209_DIR_PIN_bm;	
	TMC2209_DIR_FORWARD();
}
// ---------------------------------------------------------------
void TMC2209_STEP_init(void)
{
	// Configura el pin del led como output
	TMC2209_STEP_PORT.DIR |= TMC2209_STEP_PIN_bm;	
	TMC2209_STEP_OFF();
}
// ---------------------------------------------------------------
void tmc2209_timer_init(void)
{
	// Configuro el timer que va a generar los pulsos del stepper
	// Se debe correr antes que empieze el RTOS

	tmc2209_xTimer = xTimerCreateStatic (
            "TMC2209",
			pdMS_TO_TICKS( 1 ),
			pdTRUE,
			( void * ) 0,
			tmc2209_TimerCallback,
			&tmc2209_xTimerBuffer
			);
   
}
//----------------------------------------------------------------
void TMC2209_init(void)
{
    TMC2209_EN_init();
    TMC2209_DIR_init();
    TMC2209_STEP_init();
    
    tmc2209_timer_init();
    tmc2209_status = STOPPED;
    
}
// ---------------------------------------------------------------
void tmc2209_TimerCallback( TimerHandle_t xTimer )
{
	// Genera un pulso.
    if ( tmc_2209_counter > 0 ) {
        
        tmc_2209_counter--;
        TMC2209_STEP_TOGGLE();
        //TMC2209_STEP_ON();
        //TMC2209_STEP_OFF();
        
    } else {
        tmc2209_stop();
    }
    
}
//------------------------------------------------------------------
void tmc2209_start(t_tmc2209_dir dir, uint8_t period, uint16_t pulse_counts)
{
    
    if (  tmc2209_status == RUNNING ) {
        tmc2209_stop();
    }
    
    
    tmc_2209_counter = pulse_counts;
    
    if (dir == DIR_FW ) {
       TMC2209_DIR_FORWARD(); 
    } else {
        TMC2209_DIR_REVERSE();
    }
    
    TMC2209_ENABLE();
    
    //xTimerChangePeriod( tmc2209_xTimer, period / 2 / portTICK_PERIOD_MS, 10 );
    xTimerStart( tmc2209_xTimer, 10 );
    tmc2209_status = RUNNING;
}
//------------------------------------------------------------------
void tmc2209_stop(void)
{
    xTimerStop( tmc2209_xTimer, 10 );
    TMC2209_DISABLE();
    tmc2209_status = STOPPED;

}
//------------------------------------------------------------------
