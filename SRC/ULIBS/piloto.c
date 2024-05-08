
#include "piloto.h"
#include <stdio.h>

// Configurar ch_pA, ch_pB x cmd y por WAN

float presion_consigna;

#define INTERVALO_ENTRE_MUESTRAS_SECS 5
#define NRO_MUESTRAS            3
#define MAX_INTENTOS_AJUSTE     5
#define DPRES_X_REV				0.500		// 500 gr c/rev del piloto
#define DELTA_PRES_MIN          0.075      //  50 grs de diferencia
#define TIEMPO_ESPERA_AJUSTE    15000

static bool f_debug_piloto = false;

struct {
//	int16_t pulsos_calculados;
//	int16_t pulsos_a_aplicar;
//	int16_t total_pulsos_rollback;

//	uint16_t pwidth;

//	float pError;
//	bool motor_running;
    
	uint8_t loops;
    uint8_t ch_pA;
    uint8_t ch_pB;
	float pRef;
	float pA;
	float pB;
	t_dir_pilotos dir;
    uint32_t steps;
    
} PLTCB;	// Piloto Control Block

typedef enum { PLT_READ_INPUTS = 0, PLT_CHECK_CONDITIONS4ADJUST, PLT_AJUSTE, PLT_PROCESS_OUTPUT, PLT_EXIT } t_plt_states;

void p_productor(void);
void p_productor_handler_slots(void);
int8_t p_productor_leer_slot_actual( void );
void p_consumidor(void);

bool FSM_ajuste_presion( float presion);
void p_fsm_read_inputs( int8_t samples, uint16_t intervalo_secs );
bool p_fsm_condiciones_para_ajustar(void);
bool p_fsm_ajustar_presion(void);

SemaphoreHandle_t sem_Piloto;
StaticSemaphore_t PILOTO_xMutexBuffer;

//------------------------------------------------------------------------------
void piloto_init_outofrtos(void)
{
    sem_Piloto = xSemaphoreCreateMutexStatic( &PILOTO_xMutexBuffer );
 
}
// -----------------------------------------------------------------------------
void piloto_init(void)
{
    /*
     * Usamos un ringbuffer como estructura para almacenar los pedidos de
     * ajustes de presión.
     */
    presion_consigna = -1;
    
}
//------------------------------------------------------------------------------
void p_productor(void)
{
    /*
     * Tenemos 3 fuentes de generar pedidos de ajustes de presion:
     * - TimeSlot
     * - Ordenes Online (vienen en las respuestas a dataframes)
     * - Ordenes de linea de comando.
     * 
     * Las ordenes online y de cmdline entran solas al ringbuffer.
     * Solo debemos ejecutar el handle de timeslots.
     * 
     */
    
    p_productor_handler_slots();
    
}
//------------------------------------------------------------------------------
void p_consumidor(void)
{
    /*
	 * CONSUMIDOR:
	 * Leo la presion de la FIFO y veo si puedo ajustar.
	 * Si ajusto y todo sale bien, la saco de la FIFO.
	 * Si no pude ajustar, la dejo en la FIFO para que el proximo ciclo vuelva a reintentar
	 * con esta accion pendiente.
	 */

float l_presion_consigna;
    
    if (f_debug_piloto) {
    //    xprintf_P( PSTR("PILOTO CONS: start.\r\n"));
    }
 
    while ( xSemaphoreTake( sem_Piloto, ( TickType_t ) 5 ) != pdTRUE ) {
        vTaskDelay( ( TickType_t)( 1 ) );
    }
    
    l_presion_consigna = presion_consigna;
  
    xSemaphoreGive( sem_Piloto );
    
	if ( l_presion_consigna > 0 ) { 
        if (f_debug_piloto) {
            xprintf_P( PSTR("PILOTO CONS: new_press=%0.2f.\r\n"), l_presion_consigna);
        }
    
        // Ajuste de presion
        FSM_ajuste_presion(l_presion_consigna);
    
        // Borro el request.
        while ( xSemaphoreTake( sem_Piloto, ( TickType_t ) 5 ) != pdTRUE )
            vTaskDelay( ( TickType_t)( 1 ) );
    
        presion_consigna = -1.0;
  
        xSemaphoreGive( sem_Piloto );

    }
    
    if (f_debug_piloto) {
    //    xprintf_P( PSTR("PILOTO CONS: end.\r\n"));
    }
    return;
}
//------------------------------------------------------------------------------
bool FSM_ajuste_presion( float presion )
{
    /*
     * Maquira de estados que ajusta la presion
     */
    
uint8_t state;

    PLTCB.pRef = presion;
    PLTCB.loops = 0;
    PLTCB.ch_pA = piloto_conf.ch_pA;
    PLTCB.ch_pB = piloto_conf.ch_pB;
    
    state = PLT_READ_INPUTS;
    
    while( true ) {
        
        vTaskDelay( ( TickType_t) (100 / portTICK_PERIOD_MS ) );
        
        switch(state) {

            case PLT_READ_INPUTS:
                // Leo las entradas
                if (f_debug_piloto) {
                    xprintf_P( PSTR("PILOTO FSMajuste: state READ_INPUTS\r\n"));
                    // Espero siempre 30s antes para que se estabilizen. Sobre todo en valvulas grandes
                    xprintf_P( PSTR("PILOTO FSMajuste: await %ds\r\n"), TIEMPO_ESPERA_AJUSTE / 1000);
                }
                vTaskDelay( ( TickType_t) (TIEMPO_ESPERA_AJUSTE / portTICK_PERIOD_MS ) );
                p_fsm_read_inputs( NRO_MUESTRAS, INTERVALO_ENTRE_MUESTRAS_SECS );
                state = PLT_CHECK_CONDITIONS4ADJUST;
                break;
        
            case PLT_CHECK_CONDITIONS4ADJUST: 
                // Vemos si la consigna y las entradas son compatibles para empezar
                // a ajustar.
                if ( p_fsm_condiciones_para_ajustar()) {
                    state = PLT_AJUSTE;
                } else {
                    // No hay condiciones de ajuste: salgo
                    return(false);
                }
            break;
        
            case PLT_AJUSTE:
                if ( p_fsm_ajustar_presion() ) {
                    state = PLT_READ_INPUTS;
                } else {
                    return(false);
                }
            break;
            
            default:
                xprintf_P(PSTR("PILOTO FSM STATE ERROR: Reset FSM\r\n"));
                return (false);
        }
    }
   
    return(false);
}
//------------------------------------------------------------------------------
void p_fsm_read_inputs( int8_t samples, uint16_t intervalo_secs )
{
	/*
	 * Lee las entradas: pA,pB.
	 * Medimos N veces y promediamos
	 * Dejamos el valor en gramos
	 *
	 */

uint8_t i;
float mag = 0.0;
uint16_t raw;

	// Mido pA/pB
	PLTCB.pA = 0.0;
	PLTCB.pB = 0.0;
      
    ainputs_prender_sensores();
    
	for ( i = 0; i < samples; i++) {
        
        ainputs_read_channel ( PLTCB.ch_pA, &mag, &raw );
        PLTCB.pA += mag;
        if (f_debug_piloto) {
            xprintf_P(PSTR("PILOTO READINPUTS: pA:[%d]->%0.3f\r\n"), i, mag );
        }
        
        ainputs_read_channel ( PLTCB.ch_pB, &mag, &raw );
        PLTCB.pB += mag;
        if (f_debug_piloto) {
            xprintf_P(PSTR("PILOTO READINPUTS: pB:[%d]->%0.3f\r\n"), i, mag );
        }        
		
		vTaskDelay( ( TickType_t)( intervalo_secs * 1000 / portTICK_PERIOD_MS ) );
	}
    
    ainputs_apagar_sensores();
    
	PLTCB.pA /= samples;
	PLTCB.pB /= samples;

    if (f_debug_piloto) {
        xprintf_P(PSTR("PILOTO READINPUTS: pA=%.02f, pB=%.02f\r\n"), PLTCB.pA, PLTCB.pB );
    }
    
}
//------------------------------------------------------------------------------
bool p_fsm_condiciones_para_ajustar(void)
{
    /*
     * Revisa si las condiciones permiten ajsutar al piloto
     */
    if (f_debug_piloto) {
        xprintf_P(PSTR("PILOTO condXajuste: pA=%0.3f\r\n"),PLTCB.pA);
        xprintf_P(PSTR("                    pB=%0.3f\r\n"),PLTCB.pB);
        xprintf_P(PSTR("                    pRef=%0.3f\r\n"),PLTCB.pRef);
        xprintf_P(PSTR("                    delta=%0.3f\r\n"), fabs(PLTCB.pRef - PLTCB.pB) );
        xprintf_P(PSTR("                    loops=%d\r\n"),PLTCB.loops);   
    }
    
    // La presion llego a la banda
    if ( fabs( PLTCB.pB - PLTCB.pRef ) < DELTA_PRES_MIN ) {
        if (f_debug_piloto) {
            xprintf_P( PSTR("PILOTO condXajuste EXIT: pRef in Band !!\r\n"));
        }
        return(false);        
    }
    
    // Nro intentos
    if ( PLTCB.loops >= MAX_INTENTOS_AJUSTE ) {
        if (f_debug_piloto) {
            xprintf_P( PSTR("PILOTO condXajuste EXIT: Max.intentos !!\r\n"));
        }
        return (false);
    }
            
    // Las presiones debe ser todas positivas.
    if ( PLTCB.pA <= 0) {
        if (f_debug_piloto) {
            xprintf_P( PSTR("PILOTO condXajuste EXIT: pA < 0 !!\r\n"));
        }
        return (false);
    }
    
    if ( PLTCB.pB <= 0) {
        if (f_debug_piloto) {
            xprintf_P( PSTR("PILOTO condXajuste EXIT: pB < 0 !!\r\n"));
        }
        return (false);
    }
    
    if ( PLTCB.pRef <= 0 ) {
        if (f_debug_piloto) {
            xprintf_P( PSTR("PILOTO condXajuste EXIT: pRef < 0 !!\r\n"));
        }
        return (false);
    }
          
    return(true);
}
//------------------------------------------------------------------------------
bool p_fsm_ajustar_presion(void)
{
    /*
     * Calculamos los parametros de ajuste: direccion, pulsos
     */
    
float delta_pres = 0.0;

    if (f_debug_piloto) {
        xprintf_P( PSTR("PILOTO AJUSTE.\r\n"));
    }

	if ( PLTCB.pB < PLTCB.pRef ) {
		// Debo aumentar pB o sxprintf_P(PSTR("PILOTO: npulses=%d\r\n"), spiloto.npulses);ea apretar el tornillo (FWD)
		PLTCB.dir = DIR_FORWARD; // Giro forward, aprieto el tornillo, aumento la presion de salida
	} else {
		PLTCB.dir = DIR_REVERSE;
	}

	/*
	 * Calculo los pulsos a aplicar.
	 * El motor es de 200 pasos /rev
	 * El servo reduce 15:1
	 * Esto hace que para girar el vastago 1 rev necesite 3000 pulsos
	 * El piloto es de 4->1500gr.
	 */

	delta_pres = fabs(PLTCB.pB - PLTCB.pRef);
	PLTCB.steps = (uint32_t)( delta_pres * piloto_conf.pulsesXrev / DPRES_X_REV  );
    if (f_debug_piloto) {
        xprintf_P(PSTR("PILOTO AJUSTE: steps_calculados=%d\r\n"), PLTCB.steps);
    }
    
    // No giro mas de 1rev por intento
    if ( PLTCB.steps > piloto_conf.pulsesXrev) {
        PLTCB.steps = piloto_conf.pulsesXrev;
    }   
    if (f_debug_piloto) {
        xprintf_P(PSTR("PILOTO AJUSTE: steps_a_aplicar=%d\r\n"), PLTCB.steps);
    }
    
    PLTCB.loops++;
    /*
    stepper_move( PLTCB.dir, PLTCB.steps, piloto_conf.pWidth, 15 );
    
    while( stepper_is_running()) {
        
        // Controlo los fin de carrera
        // Ajusto al alta
        if ( ( FC_alta_read() == 0 ) && ( PLTCB.pRef > PLTCB.pB) ) {
            stepper_stop();
            xprintf_P(PSTR("PILOTO AJUSTE: STOP X FIN DE CARRERA ALTA.\r\n"));
            return(false);
        }
        
        // Ajuste a la baja
        if ( ( FC_baja_read() == 0 ) && ( PLTCB.pRef < PLTCB.pB ) ) {
            stepper_stop();
            xprintf_P(PSTR("PILOTO AJUSTE: STOP X FIN DE CARRERA BAJA.\r\n"));
            return(false);
        }
        
        vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
    }
    */
    return(true);
    
}
//------------------------------------------------------------------------------
int8_t p_productor_leer_slot_actual( void )
{
	// Determina el id del slot en que estoy.
	// Los slots deben tener presion > 0.1

RtcTimeType_t rtcDateTime;
uint16_t now;
int8_t slot_actual = -1;
int8_t ultimo_slot;
int8_t i;

	// Vemos si hay slots configuradas.
	// Solo chequeo el primero. DEBEN ESTAR ORDENADOS !!
	if ( piloto_conf.pltSlots[0].presion < 0.1 ) {
		// No tengo slots configurados. !!
		return(-1);
	}

	// Determino la hhmm actuales
	memset( &rtcDateTime, '\0', sizeof(RtcTimeType_t));
	if ( ! RTC_read_dtime(&rtcDateTime) ) {
		xprintf_P(PSTR("PILOTO ERROR: I2C:RTC:pv_get_hhhmm_now\r\n\0"));
		return(-1);
	}
	now = rtcDateTime.hour * 100 + rtcDateTime.min;

    while ( xSemaphoreTake( sem_Piloto, ( TickType_t ) 5 ) != pdTRUE )
  		vTaskDelay( ( TickType_t)( 1 ) );
    
    // Busco el ultimo slot configurado
    ultimo_slot = 0;
    for ( i=0; i < MAX_PILOTO_PSLOTS; i++ ) {
        if ( piloto_conf.pltSlots[i].presion > 0.1 ) {
            ultimo_slot = i;
        }
    }
    
    // Busco el slot actual
    // Caso 1: Estoy antes que empieza el slot 0
    if ( now <= piloto_conf.pltSlots[0].pTime ) {
        // El slot es el ultimo configurado
        slot_actual = ultimo_slot;
        goto quit;
    }
    
    // Caso 2: 
    for ( i=0; i < ultimo_slot; i++ ) {
        if ( piloto_conf.pltSlots[i].presion > 0.1 ) {
            if ( ( piloto_conf.pltSlots[i].pTime <= now ) && ( now < piloto_conf.pltSlots[i+1].pTime ) ) {
                slot_actual = i;
                goto quit;
            } 
        }
    }
    
    // Estoy en el ultimo slot
    slot_actual = ultimo_slot;
    
quit:
        
    xSemaphoreGive( sem_Piloto );
    //if (f_debug_piloto) {
    //    xprintf_P(PSTR("PILOTO: Slot actual=%d\r\n"), slot_actual);
    //}
    return(slot_actual);    

}     
//------------------------------------------------------------------------------
/*
 Handlers de Productor
 */
void p_productor_handler_slots(void)
{
    /*
     * Lo invoca el productor cada 1 minuto.
     * Se fija en que timeslot esta y si cambio o no.
     * Si cambio, manda la orden de ajustar a la presion corespondiente 
     * al nuevo tslot.
     * 
     */
    
static int8_t slot0 = -1;
int8_t slot_now = -1;

    if (f_debug_piloto) {
    //    xprintf_P(PSTR("PILOTO PROD: start.\r\n"));
    }

    slot_now = p_productor_leer_slot_actual();
    if ( slot_now < 0 ) {
        xprintf_P(PSTR("PILOTO PROD: Error slot now !!\r\n"));
        goto quit;
    }
    
    if (f_debug_piloto) {
        xprintf_P( PSTR("PILOTO PROD: SLOTS slot0=%d, slot_now=%d\r\n"), slot0, slot_now );
    }
        
    // Cambio el slot. ?
	if ( slot_now != slot0 ) {
        
        while ( xSemaphoreTake( sem_Piloto, ( TickType_t ) 5 ) != pdTRUE )
            vTaskDelay( ( TickType_t)( 1 ) );
    
        presion_consigna = piloto_conf.pltSlots[slot_now].presion;
    
        xSemaphoreGive( sem_Piloto );
        
        if (f_debug_piloto) {
            xprintf_P(PSTR("PILOTO PROD: Cambio de slot: %d=>%d, pRef=%.03f\r\n"), slot0, slot_now, presion_consigna);
		}
		slot0 = slot_now;
	}
    
quit:
        
    if (f_debug_piloto) {
    //    xprintf_P(PSTR("PILOTO PROD: end.\r\n"));    
    }
}
//------------------------------------------------------------------------------
/*
 CONFIGURACION
 */
void piloto_config_defaults(void)
{
    
uint8_t i;
    
    piloto_conf.enabled = false;
    piloto_conf.pWidth = 10;
    piloto_conf.pulsesXrev = 2000;
    for(i=0; i<MAX_PILOTO_PSLOTS; i++) {
        piloto_conf.pltSlots[i].presion = 0.0;
        piloto_conf.pltSlots[i].pTime = 0;
    }
    piloto_conf.ch_pA = 0;
    piloto_conf.ch_pB = 1;
    
}
//------------------------------------------------------------------------------
void piloto_print_configuration( void )
{
    
int8_t slot;

	xprintf_P( PSTR("Piloto:\r\n"));
    xprintf_P(PSTR(" debug: "));
    f_debug_piloto ? xprintf_P(PSTR("on\r\n")) : xprintf_P(PSTR("off\r\n"));

    if (  ! piloto_conf.enabled ) {
        xprintf_P( PSTR(" status=disabled\r\n"));
        return;
    }
    
    xprintf_P( PSTR(" status=enbled\r\n"));
	xprintf_P( PSTR(" pPulsosXrev=%d, pWidth=%d(ms)\r\n"), piloto_conf.pulsesXrev, piloto_conf.pWidth  );
    xprintf_P( PSTR(" ch_pA=%d, ch_pB=%d\r\n"), piloto_conf.ch_pA, piloto_conf.ch_pB);
	xprintf_P( PSTR(" Slots:\r\n"));
    slot = p_productor_leer_slot_actual();
    xprintf_P( PSTR(" pos=%02d, pRef=%0.2f\r\n"), slot, piloto_conf.pltSlots[slot].presion );
	xprintf_P( PSTR(" "));
	for (slot=0; slot < (MAX_PILOTO_PSLOTS / 2);slot++) {
		xprintf_P( PSTR("[%02d]%02d->%0.2f "), slot, piloto_conf.pltSlots[slot].pTime,piloto_conf.pltSlots[slot].presion  );
	}
	xprintf_P( PSTR("\r\n"));

	xprintf_P( PSTR(" "));
	for (slot=(MAX_PILOTO_PSLOTS / 2); slot < MAX_PILOTO_PSLOTS;slot++) {
		xprintf_P( PSTR("[%02d]%02d->%0.2f "), slot, piloto_conf.pltSlots[slot].pTime, piloto_conf.pltSlots[slot].presion  );
	}
	xprintf_P( PSTR("\r\n"));
}
//------------------------------------------------------------------------------
bool piloto_config_slot( uint8_t slot, char *s_ptime, char *s_presion )
{
	// Configura un slot

uint16_t ptime;
float presion;

	// Intervalos tiempo:presion:

    ptime = atoi(s_ptime);
    presion = atof(s_presion);
    
	if ( ( slot < MAX_PILOTO_PSLOTS ) && ( ptime < 2359 ) && ( presion >= 0.0 ) ){
        
        piloto_conf.pltSlots[slot].pTime = ptime;
        piloto_conf.pltSlots[slot].presion = presion;
		return(true);
	}

	return(false);
}
//------------------------------------------------------------------------------
bool piloto_config_pulseXrev( char *s_pulseXrev )
{
	piloto_conf.pulsesXrev = atoi(s_pulseXrev);
    return (true);
}
//------------------------------------------------------------------------------
bool piloto_config_pwidth( char *s_pwidth )
{
	piloto_conf.pWidth = atoi(s_pwidth);
    return(true);
}
//------------------------------------------------------------------------------
bool piloto_config_enable( char *s_enable )
{
    
    if (!strcmp_P( strupr(s_enable), PSTR("TRUE"))  ) {
        piloto_conf.enabled = true;
        return (true);
    }
    
    if (!strcmp_P( strupr(s_enable), PSTR("FALSE"))  ) {
        piloto_conf.enabled = false;
        return (true);
    }
    
    return(false);
}
//------------------------------------------------------------------------------
uint8_t piloto_hash( void )
    {
     /*
      * Calculo el hash de la configuracion de modbus.
      */
    
uint8_t i,j;
uint8_t hash = 0;
char *p;
uint8_t l_hash_buffer[64];

   // Calculo el hash de la configuracion modbus

    memset(l_hash_buffer, '\0', sizeof(l_hash_buffer) );
    j = 0;
    if ( piloto_conf.enabled ) {
        j += sprintf_P( (char *)&l_hash_buffer[j], PSTR("[TRUE,%04d,%02d]"),piloto_conf.pulsesXrev, piloto_conf.pWidth);
    } else {
        j += sprintf_P( (char *)&l_hash_buffer[j], PSTR("[FALSE,%04d,%02d]"),piloto_conf.pulsesXrev, piloto_conf.pWidth);
    }
    p = (char *)l_hash_buffer;
    while (*p != '\0') {
        hash = u_hash(hash, *p++);
    }
    //xprintf_P(PSTR("HASH_PILOTO:%s, hash=%d\r\n"), l_hash_buffer, hash ); 

    for(i=0; i < MAX_PILOTO_PSLOTS; i++) {
        memset(l_hash_buffer, '\0', sizeof(l_hash_buffer) );
        sprintf_P( (char *)&l_hash_buffer, PSTR("[S%02d:%04d,%0.2f]"), i, piloto_conf.pltSlots[i].pTime, piloto_conf.pltSlots[i].presion );
        p = (char *)l_hash_buffer;
        while (*p != '\0') {
            hash = u_hash(hash, *p++);
        }
        //xprintf_P(PSTR("HASH_PILOTO:%s, hash=%d\r\n"), l_hash_buffer, hash ); 
    }

    return(hash);
}
//------------------------------------------------------------------------------
void piloto_config_debug(bool debug )
{
    if ( debug ) {
        f_debug_piloto = true;
    } else {
        f_debug_piloto = false;
    }
    
}
//------------------------------------------------------------------------------  
void PILOTO_productor_handler_online( float presion)
{
    /*
     * Se invoca desde tkWAN cuando llega en una respuesta una orden
     * de actuar sobre la presion.
     */
    
	xprintf_P(PSTR("PILOTO PROD: onlineOrder (p=%0.2f).\r\n"), presion);
    while ( xSemaphoreTake( sem_Piloto, ( TickType_t ) 5 ) != pdTRUE )
  		vTaskDelay( ( TickType_t)( 1 ) );
    
	presion_consigna = presion;
    
    xSemaphoreGive( sem_Piloto );
      
}
//------------------------------------------------------------------------------
bool PILOTO_productor_handler_cmdline(float presion)
{
    /*
     * Funcion invocada desde cmdline para setear una presion
     * Genera una 'orden' de trabajo para el consumidor con la presion
     * dada
     */

	xprintf_P(PSTR("PILOTO PROD: cmdlineOrder (p=%0.2f).\r\n"), presion);
    
    while ( xSemaphoreTake( sem_Piloto, ( TickType_t ) 5 ) != pdTRUE )
  		vTaskDelay( ( TickType_t)( 1 ) );
    
	presion_consigna = presion;
    
    xSemaphoreGive( sem_Piloto );
    
    return (true);
}
//------------------------------------------------------------------------------
