
#include "contadores.h"
#include <avr/interrupt.h>

static bool f_debug_counters = false;

StaticTimer_t counter_xTimerBuffer;
TimerHandle_t counter_xTimer;

static void pv_counter_TimerCallback( TimerHandle_t xTimer );
//------------------------------------------------------------------------------
void counter_init_outofrtos( void )
{
    CNT0_CONFIG();
          
    // Configuro los pines para interrumpir
    // Habilito a interrumpir, pullup, flanco de bajada.
    cli();
    PORTF.PIN4CTRL |= PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;
    sei();
}
// ----------------------------------------------------------------------------- 
void counter_init( void )
{
    counter_xTimer = xTimerCreateStatic ("CNT0",
		pdMS_TO_TICKS( 10 ),    // 10 ms
		pdTRUE,                 // Continuo.
		( void * ) 0,
		pv_counter_TimerCallback,
		&counter_xTimerBuffer
	);
        
    counter_clear();
    contador.fsm_ticks_count = 0;
}
// ----------------------------------------------------------------------------- 
void counter_config_defaults( void )
{
    /*
     * Realiza la configuracion por defecto de los canales digitales.
     */

    //strncpy( counter_conf.name, "X", CNT_PARAMNAME_LENGTH );
    strlcpy( counter_conf.name, "X", CNT_PARAMNAME_LENGTH );
    counter_conf.enabled = false;
    counter_conf.magpp = 1;
    counter_conf.modo_medida = CAUDAL;

}
//------------------------------------------------------------------------------
void counter_print_configuration( void )
{
    /*
     * Muestra la configuracion de todos los canales de contadores en la terminal
     * La usa el comando tkCmd::status.
     */
    

    xprintf_P(PSTR("Counter:\r\n"));
    xprintf_P(PSTR(" debug: "));
    f_debug_counters ? xprintf_P(PSTR("on\r\n")) : xprintf_P(PSTR("off\r\n"));
    
        
    if ( counter_conf.enabled ) {
        xprintf_P( PSTR(" c0: +"));
    } else {
        xprintf_P( PSTR(" c0: -"));
    }
                
    xprintf_P( PSTR("[%s,magpp=%.03f,"), counter_conf.name, counter_conf.magpp );
    if ( counter_conf.modo_medida == CAUDAL ) {
        xprintf_P(PSTR("CAUDAL]\r\n"));
    } else {
        xprintf_P(PSTR("PULSO]\r\n"));
    }
      
}
//------------------------------------------------------------------------------
bool counter_config_channel( char *s_enable, char *s_name, char *s_magpp, char *s_modo )
{
	// Configuro un canal contador.
	// channel: id del canal
	// s_param0: string del nombre del canal
	// s_param1: string con el valor del factor magpp.
	//
	// {0..1} dname magPP

bool retS = false;

    //xprintf_P(PSTR("DEBUG COUNTERS: en=%s,name=%s,magpp=%s,modo=%s,rbsize=%s\r\n"), s_enable,s_name,s_magpp,s_modo,s_rb_size  );

	if ( s_name == NULL ) {
		return(retS);
	}

    // Enable ?
    if (!strcmp_P( strupr(s_enable), PSTR("TRUE"))  ) {
        counter_conf.enabled = true;
    } else if (!strcmp_P( strupr(s_enable), PSTR("FALSE"))  ) {
        counter_conf.enabled = false;
    }
        
    // NOMBRE
	//snprintf_P( cnt->channel[ch].name, CNT_PARAMNAME_LENGTH, PSTR("%s"), s_name );
    //strncpy( counter_conf.name, s_name, CNT_PARAMNAME_LENGTH );
    strlcpy( counter_conf.name, s_name, CNT_PARAMNAME_LENGTH );

	// MAGPP
	if ( s_magpp != NULL ) { counter_conf.magpp = atof(s_magpp); }

    // MODO ( PULSO/CAUDAL )
    if ( s_modo != NULL ) {
		if ( strcmp_P( strupr(s_modo), PSTR("PULSO")) == 0 ) {
			counter_conf.modo_medida = PULSOS;

		} else if ( strcmp_P( strupr(s_modo) , PSTR("CAUDAL")) == 0 ) {
			counter_conf.modo_medida = CAUDAL;

		} else {
			//xprintf_P(PSTR("ERROR: counters modo: PULSO/CAUDAL only!!\r\n"));
            return (false);
		}
    }
        
    retS = true;
	return(retS);

}
//------------------------------------------------------------------------------
void counter_config_debug(bool debug )
{
    if ( debug ) {
        f_debug_counters = true;
    } else {
        f_debug_counters = false;
    }
}
//------------------------------------------------------------------------------
bool counter_read_debug(void)
{
    return (f_debug_counters);
}
//------------------------------------------------------------------------------
uint8_t counter_read_pin(void)
{
    return ( ( CNT0_PORT.IN & CNT0_PIN_bm ) >> CNT0_PIN) ;
}
// -----------------------------------------------------------------------------
counter_value_t counter_read(void)
{
    return(contador);
}
// -----------------------------------------------------------------------------
void counter_clear(void)
{
   /*
    * Una vez por periodo ( timerpoll ) borro los contadores.
    * Si en el periodo NO llegaron pulsos, aqui debo entonces en los
    * caudales agregar un 0.0 al ring buffer para que luego de un tiempo
    * converja a 0.
    * 
    */
    contador.caudal = 0.0;
    contador.pulsos = 0;
}
//------------------------------------------------------------------------------
uint8_t counter_hash(void)
{
    
uint8_t j;
uint8_t hash = 0;
uint8_t l_hash_buffer[64];
char *p;

    // Calculo el hash de la configuracion de los contadores


    memset( l_hash_buffer, '\0', sizeof(l_hash_buffer));
    j = sprintf_P( (char *)&l_hash_buffer, PSTR("[C0:") );
    
    if ( counter_conf.enabled ) {
        j += sprintf_P( (char *)&l_hash_buffer[j], PSTR("TRUE,") );
    } else {
       j += sprintf_P( (char *)&l_hash_buffer[j], PSTR("FALSE,") );
    }
        
    j += sprintf_P( (char *)&l_hash_buffer[j], PSTR("%s,"), counter_conf.name );
    j += sprintf_P( (char *)&l_hash_buffer[j], PSTR("%.03f,"), counter_conf.magpp );
        
    if ( counter_conf.modo_medida == 0 ) {
        j += sprintf_P( (char *)&l_hash_buffer[j], PSTR("CAUDAL]"));
    } else {
        j += sprintf_P( (char *)&l_hash_buffer[j], PSTR("PULSOS]"));
    }

    p = (char *)l_hash_buffer;
    while (*p != '\0') {
        hash = u_hash(hash, *p++);
    }
        
    //xprintf_P(PSTR("HASH_CNT:<%s>, hash=%d\r\n"), hash_buffer, hash );
 
    return(hash);
    
}
//------------------------------------------------------------------------------
ISR(PORTF_PORT_vect)
{

    // Borro las flags.
    if (PF4_INTERRUPT ) {
        
        if ( contador.fsm_ticks_count == 0) {
            // Arranca el timer que va a hacer el debounced
            xTimerStart(counter_xTimer, 10);   
        }
            
        // La interrupcion la vuelve a habilitar el timer.
        PF4_CLEAR_INTERRUPT_FLAG;
    }

}
//------------------------------------------------------------------------------
static void pv_counter_TimerCallback( TimerHandle_t xTimer )
{
	/*
     *  Funcion de callback de la entrada de contador A.
     *  Controla el pulse_width y el debounce.
     *  Lo arranca la llegada de la interrupcion.
     *  El primer tick es a los 10 ms.
     *  Controla si el nivel es alto aún. 
     *  Si lo es, contabiliza un pulso.
     *  Espera 50ms ( cuenta hasta 5) que es el pulso minimo y rearma el sistema
     *  de interrupciones.
     *  
     */
    
float duracion_pulso;
uint32_t ticks_now;

    contador.fsm_ticks_count++;
    
    // Pulso valido ( pulse width minimo )
    if (contador.fsm_ticks_count == 1) {
        if ( counter_read_pin() == 0 ) {
            // El pulso tiene el ancho minimo de 10 ms.
            contador.pulsos++;
            
            ticks_now = xTaskGetTickCountFromISR();
            duracion_pulso =  (float)(ticks_now - contador.start_pulse);    // Duracion en ticks
            duracion_pulso /= configTICK_RATE_HZ;                           // Duracion en secs.
            duracion_pulso /= 3600;                                         // Duracion en horas
            // Guardo el inicio del pulso para medir el caudal
            contador.start_pulse = ticks_now;
        
            if ( duracion_pulso > 0 ) {
                contador.caudal = counter_conf.magpp / duracion_pulso;      // En mt3/h 
            } else {
                contador.caudal = 0.0;
            }
            //
            contador.caudal = duracion_pulso; 
            
            if (f_debug_counters) {
                xprintf_P(PSTR("COUNTER: PULSOS=%d, CAUDAL=%0.3f\r\n"), contador.pulsos, contador.caudal );
            }
        }
        return;
    }
    
    // Debounced: Pulso valido
    if (contador.fsm_ticks_count == 10) {
        // Apago el timer.
         xTimerStop(counter_xTimer, 10);
        // Habilito la interrupcion
        contador.fsm_ticks_count = 0;
        return;
    }
 
}

       