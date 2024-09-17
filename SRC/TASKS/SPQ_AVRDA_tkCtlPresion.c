/*
 * File:   tkPiloto.c
 * Author: pablo
 *
 */


#include "SPQ_AVRDA.h"


static void pv_consigna_initService(void);
static void pv_consigna_service(void);
static bool pv_consigna_set( consigna_t consigna);
static bool pv_consigna_read_status( uint16_t *status);
static bool pv_consigna_write( consigna_t consigna);

#define SENSOR_PRESION_ADDR  0xFE
#define STATUS_REGADDR       0x00
#define ORDERS_REGADDR       0x01
#define STATUS_BIT           15

mbus_CONTROL_BLOCK_t mbus_cb;

//------------------------------------------------------------------------------
void tkCtlPresion(void * pvParameters)
{

	/*
     * DOBLE CONSIGNA:
     * Esta tarea se encarga de cotrolar la hora en que debe aplicarse
     * e invocar a las funciones que la fijan.
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
       
    // Espero que todo este arrancado (30s)
    //vTaskDelay( ( TickType_t)( 30000 / portTICK_PERIOD_MS ) );
    
//    if ( systemConf.ptr_consigna_conf->enabled ) {
//        pv_consigna_initService();          
//    }

	for( ;; )
	{
        /*
         * Corre cada 1 minuto porque el timeslot se mide como hhmm y no queremos
         * que se realmacene la orden de un mismo tslot
         * 
         */
        u_kick_wdt(TK_CTLPRES);
		vTaskDelay( ( TickType_t)( 30000 / portTICK_PERIOD_MS ) );
      
//        if ( systemConf.ptr_consigna_conf->enabled ) {
//            pv_consigna_service();
//        }
               
	}
}
//------------------------------------------------------------------------------
bool CONSIGNA_set_diurna(void)
{
    // Setea la consigna DIURNA
        
    if (consigna_debug_flag() ) {
        xprintf_P(PSTR("DEBUG CONSIGNA: Set_diurna\r\n"));
    }

    if ( pv_consigna_set(CONSIGNA_DIURNA) ) {
        consigna_aplicada = CONSIGNA_DIURNA;
        return(true);
}
    return(false);
}
//------------------------------------------------------------------------------
bool CONSIGNA_set_nocturna(void)
{
    // Setea la consigna NOCTURNA

 
     if (consigna_debug_flag() ) {
        xprintf_P(PSTR("DEBUG CONSIGNA: Set_nocturna\r\n"));
    }

    if ( pv_consigna_set(CONSIGNA_NOCTURNA) ) {
        consigna_aplicada = CONSIGNA_NOCTURNA; 
        return(true);
    }

    return(false);
}
//------------------------------------------------------------------------------
static void pv_consigna_initService(void)
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

    xprintf_P(PSTR("CONSIGNA init service.\r\n")); 
    
    RTC_read_dtime(&rtc);
    now = rtc.hour * 100 + rtc.min;
    c_dia = consigna_conf.consigna_diurna;
    c_noche = consigna_conf.consigna_nocturna;
    
    // Ajusto todo a minutos.
    now = u_hhmm_to_mins(now);
    c_dia = u_hhmm_to_mins( c_dia );
    c_noche = u_hhmm_to_mins( c_noche );
    
    if (c_dia > c_noche ) {
        xprintf_P(PSTR("ERROR: No puedo determinar consigna inicial (c_dia > c_noche)\r\n"));
        // Aplico la nocturna que es la que deberia tener menos presion
        xprintf_P(PSTR("CONSIGNA Init:nocturna %02d:%02d [%04d]\r\n"),rtc.hour,rtc.min, now);
		if ( ! CONSIGNA_set_nocturna() ) {
            xprintf_P(PSTR("CONSIGNA nocturna ERR\r\n"));    
        } else {
            xprintf_P(PSTR("CONSIGNA nocturna OK\r\n"));   
        }
        goto exit;
    }
    
    if ( ( now <= c_dia ) || ( c_noche <= now ) ) {
        // Aplico consigna nocturna
         xprintf_P(PSTR("CONSIGNA Init:nocturna %02d:%02d [%04d]\r\n"),rtc.hour,rtc.min, now);
		if ( ! CONSIGNA_set_nocturna() ) {
            xprintf_P(PSTR("CONSIGNA nocturna ERR\r\n"));    
        } else {
            xprintf_P(PSTR("CONSIGNA nocturna OK\r\n"));   
        }
        goto exit;
       
    } else {
        // Aplico consigna diurna
        xprintf_P(PSTR("CONSIGNA Init:diurna %02d:%02d [%04d]\r\n"),rtc.hour,rtc.min, now);
		if ( ! CONSIGNA_set_diurna() ) {
            xprintf_P(PSTR("CONSIGNA diurna ERR\r\n"));    
        } else {
            xprintf_P(PSTR("CONSIGNA diurna OK\r\n"));   
        }
        goto exit;
    }
    
exit:

   return; 
    
}
//------------------------------------------------------------------------------
static void pv_consigna_service(void)
{
    /*
     * Chequea que sea la hora de aplicar alguna consigna y la aplica.
     */
    
RtcTimeType_t rtc;
uint16_t now;

    if (consigna_debug_flag() ) {
        xprintf_P(PSTR("DEBUG CONSIGNA: Service\r\n"));
    }

	// Chequeo y aplico.
	// Las consignas se chequean y/o setean en cualquier modo de trabajo, continuo o discreto
	memset( &rtc, '\0', sizeof(RtcTimeType_t));
	if ( ! RTC_read_dtime(&rtc) ) {
		xprintf_P(PSTR("CONSIGNA ERROR: I2C:RTC chequear_consignas\r\n\0"));
		return;
	}

    now = rtc.hour * 100 + rtc.min;
    
	// Consigna diurna ?
	if ( now == consigna_conf.consigna_diurna  ) {
		xprintf_P(PSTR("Set CONSIGNA diurna %02d:%02d [%04d]\r\n"),rtc.hour,rtc.min, now);
        if ( ! CONSIGNA_set_diurna() ) {
            xprintf_P(PSTR("CONSIGNA diurna ERR\r\n"));    
        } else {
            xprintf_P(PSTR("CONSIGNA diurna OK\r\n"));   
        }
        goto exit;
	}

	// Consigna nocturna ?
	if ( now == consigna_conf.consigna_nocturna  ) {
		xprintf_P(PSTR("Set CONSIGNA nocturna %02d:%02d [%04d]\r\n"),rtc.hour,rtc.min, now);
        if ( ! CONSIGNA_set_nocturna() ) {
            xprintf_P(PSTR("CONSIGNA nocturna ERR\r\n"));    
        } else {
            xprintf_P(PSTR("CONSIGNA nocturna OK\r\n"));   
        }
        goto exit;
	}
    
    // No es hora de aplicar ninguna consigna
   
exit:

    return;

}
//------------------------------------------------------------------------------
static bool pv_consigna_set( consigna_t consigna)
{
    /*
     * Implementa el algoritmo de enviar comandos modbus al sensor de presion
     * para setear la consigna indicada, esperar, y leer el status.
     */
    
bool res;
uint16_t status;
uint8_t timeout;

    if (consigna_debug_flag() ) {
        xprintf_P(PSTR("DEBUG CONSIGNA: Set (%d)\r\n"), consigna);
    }

    consigna_prender_sensor(); 
    
    RS485COMMS_ENTER_CRITICAL();
    
    RS485_AWAKE();
            
    // Leo que el sensor de presion este en modo standby
    res = pv_consigna_read_status(&status);
    if ( ! res ) {      
        // MANDO ERROR AL SERVIDOR
        goto exit;
    }

    if (  word_isBitSet(status, STATUS_BIT )) {
        xprintf_P(PSTR("DEBUG: BIT SET. Exit r\n"));
        goto exit;
    }
     
    // Indico que aplique la consigna
    res = pv_consigna_write(consigna);
    if ( ! res ) {      
        // MANDO ERROR AL SERVIDOR
        goto exit;
    }    
    
    // Leo que el status indique que esta en standby(ya la aplicó) y la consigna
    // sea la correcta.
    timeout = 5;
    while(timeout-- > 0 ) {
        vTaskDelay( ( TickType_t)( 3000 / portTICK_PERIOD_MS ) ); 

        // Leo el status bit
        res = pv_consigna_read_status(&status);
        if ( ! res ) {      
            // MANDO ERROR AL SERVIDOR
            goto exit;
        }    
        
        if ( ! word_isBitSet(status, STATUS_BIT )) {
            goto exit;
        }
        
        if (consigna_debug_flag() ) {
            xprintf_P(PSTR("DEBUG CONSIGNA: Await\r\n"));
        }
    }
    
    xprintf_P(PSTR("DEBUG CONSIGNA: Tiemout ERROR\r\n"));
    
exit:
    
    RS485_SLEEP();

    RS485COMMS_EXIT_CRITICAL();
    
    consigna_apagar_sensor();
    
    return(res);
            
}
//------------------------------------------------------------------------------
static bool pv_consigna_read_status( uint16_t *status)
{
    
uint8_t size;
uint16_t crc;

    /*
     * Envio el comando modbus al sensor para leer el registro del status
     * y lo decodifico.
     * TX-> [0xFE][0x03][0x00][0x00][0x00][0x01][0x90][0x05]
     * 
     * TX (len=8)[0xFE][0x03][0x00][0x00][0x00][0x01][0x90][0x05]
     * 
     * Al recibir, el bit 15 indica en 0: standby, en 1 activo.
     * Para poder seguir mandando comandos, debe estar en 0 (standby)
     * 
     */
    
    if (consigna_debug_flag() ) {
        xprintf_P(PSTR("DEBUG CONSIGNA: pv_consigna_read_status\r\n"));
    }
    
    memset( &mbus_cb, '\0', sizeof(mbus_CONTROL_BLOCK_t));
    
    mbus_cb.tx_buffer[0] = SENSOR_PRESION_ADDR;    // [0xFE]
    mbus_cb.tx_buffer[1] = 0x3;                    // FCODE=0x03
    
    // Starting address
    mbus_cb.tx_buffer[2] = (uint8_t) (( STATUS_REGADDR & 0xFF00  ) >> 8);// DST_ADDR_H
    mbus_cb.tx_buffer[3] = (uint8_t) ( STATUS_REGADDR & 0x00FF );		 // DST_ADDR_L
    
    // Quantity of recs.
    mbus_cb.tx_buffer[4] = 0x00;
    mbus_cb.tx_buffer[5] = 0x01;
    
    size = 6;
    crc = modbus_CRC16( mbus_cb.tx_buffer, size );
    mbus_cb.tx_buffer[size++] = (uint8_t)( crc & 0x00FF );			// CRC Low
    mbus_cb.tx_buffer[size++] = (uint8_t)( (crc & 0xFF00) >> 8 );	// CRC High
    mbus_cb.tx_size = size;
    mbus_cb.io_status = false;
    
    // Esto lo requiero para la decodificación
    mbus_cb.channel.fcode = 0x03;
    mbus_cb.channel.type = u16;
    mbus_cb.channel.codec = CODEC3210;
    mbus_cb.channel.divisor_p10 = 0;
    
    // Realizo la transaccion MODBUS
    modbus_txmit_ADU( &mbus_cb );
	modbus_rcvd_ADU( &mbus_cb );
	modbus_decode_ADU ( &mbus_cb );
      
    // Análisis de resultados
    if (mbus_cb.io_status == false) {
        if (consigna_debug_flag() ) {
            xprintf_P(PSTR("DEBUG CONSIGNA: pv_consigna_read_status ERROR\r\n"));
        }
        *status = NULL;
        return(false);
    }
        
    *status = mbus_cb.udata.u16_value; 
    if (consigna_debug_flag() ) {
        if ( word_isBitSet(*status, STATUS_BIT )) {
            xprintf_P(PSTR("DEBUG CONSIGNA: pv_consigna_read_status (busy) OK(%u)[0x%04x]\r\n"), *status, *status);
        } else {
            xprintf_P(PSTR("DEBUG CONSIGNA: pv_consigna_read_status (standby) OK(%u)[0x%04x]\r\n"), *status, *status);
        }
    }
    
    return(true);
}
//------------------------------------------------------------------------------
static bool pv_consigna_write( consigna_t consigna)
{
    /*
     * Envio el comando modbus 0x6 al sensor para leer el registro del status
     * y lo decodifico.
     * Leo 1 registro ( 2 bytes ) !!!
     */
    
uint8_t size;
uint16_t crc;

    if (consigna_debug_flag() ) {
        xprintf_P(PSTR("DEBUG CONSIGNA: pv_consigna_write %d\r\n"), consigna);
    }

    memset( &mbus_cb, '\0', sizeof(mbus_CONTROL_BLOCK_t));
    
    mbus_cb.tx_buffer[0] = SENSOR_PRESION_ADDR;    // [0xFE]
    mbus_cb.tx_buffer[1] = 0x6;                    // FCODE=0x06
    
    // Starting address
    mbus_cb.tx_buffer[2] = (uint8_t) (( ORDERS_REGADDR & 0xFF00  ) >> 8);// DST_ADDR_H
    mbus_cb.tx_buffer[3] = (uint8_t) ( ORDERS_REGADDR & 0x00FF );		 // DST_ADDR_L
    
    // Register Value
    switch (consigna ) {
        case CONSIGNA_DIURNA:
            mbus_cb.tx_buffer[4] = 0x00;
            mbus_cb.tx_buffer[5] = 0x01;
            break;
        case CONSIGNA_NOCTURNA:
            mbus_cb.tx_buffer[4] = 0x00;
            mbus_cb.tx_buffer[5] = 0x02;
            break;
        default:
            xprintf_P(PSTR("ERROR: pv_consigna_write\r\n"));
            break;
    }
    
    size = 6;
    crc = modbus_CRC16( mbus_cb.tx_buffer, size );
    mbus_cb.tx_buffer[size++] = (uint8_t)( crc & 0x00FF );			// CRC Low
    mbus_cb.tx_buffer[size++] = (uint8_t)( (crc & 0xFF00) >> 8 );	// CRC High
    mbus_cb.tx_size = size;
    mbus_cb.io_status = false;

    mbus_cb.channel.fcode = 0x06;
    mbus_cb.channel.type = u16;
    mbus_cb.channel.codec = CODEC3210;
    mbus_cb.channel.divisor_p10 = 0;
    
    // Realizo la transaccion MODBUS
    modbus_txmit_ADU( &mbus_cb );
	modbus_rcvd_ADU( &mbus_cb );
	modbus_decode_ADU ( &mbus_cb );
    //
    if (mbus_cb.io_status == false) {
        if (consigna_debug_flag() ) {
            xprintf_P(PSTR("DEBUG CONSIGNA: pv_consigna_write ERROR\r\n"));
        }
        return(false);
    }
    
    if (consigna_debug_flag() ) {
        xprintf_P(PSTR("DEBUG CONSIGNA: pv_consigna_write OK\r\n"));
    }
    
    return(true);
}
//------------------------------------------------------------------------------
