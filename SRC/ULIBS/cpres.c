

#include "cpres.h"
#include "SPQ_AVRDA.h"

mbus_CONTROL_BLOCK_t cpres_mbus_cb;
modbus_channel_conf_t cpres_mbchannel;
modbus_hold_t cpres_udata;

//------------------------------------------------------------------------------
int8_t cpres_set_valves(uint8_t valve0_gc, uint8_t valve1_gc)
{
    /*
     * Debemos armar el MBUS_CONTROL_BLOCK
     * Primero preparamos el canal.
     * La operacion de set es 6.
     * 
     */
    
uint16_t uvalue;

    // Bit run=1
    uvalue = 0x80 | valve1_gc | valve0_gc;
    
    memset( &cpres_mbus_cb, '\0', sizeof(mbus_CONTROL_BLOCK_t));
    memset( &cpres_mbchannel, '\0', sizeof(modbus_channel_conf_t));
    memset( &cpres_udata, '\0', sizeof(modbus_hold_t));
    
    // Escribo el valor de las valvulas (fcode=0x06)
    cpres_mbchannel.enabled = true;
    strcpy( &cpres_mbchannel.name, "VALVES");
    cpres_mbchannel.slave_address = 0x64;    // Slave 100
    cpres_mbchannel.reg_address = 0x01;      // Address 1
    cpres_mbchannel.nro_regs = 1;
    cpres_mbchannel.fcode = 0x6;             // Write
    cpres_mbchannel.type = u16;
    cpres_mbchannel.divisor_p10 = 0;
    cpres_mbchannel.codec = CODEC1032;
    
    cpres_udata.u16_value = uvalue;
    
    cpres_mbus_cb.channel = cpres_mbchannel;
    cpres_mbus_cb.udata = cpres_udata;
    
    // Siempre prendo: Si esta prendido no pasa nada
    SET_EN_PWR_CPRES();
    CPRES_IS_ON = true;
    vTaskDelay( ( TickType_t)( 2000 / portTICK_PERIOD_MS ) );
        
    modbus_io( &cpres_mbus_cb );
       
    return(0);
}
//------------------------------------------------------------------------------
int8_t cpres_check_status(void)
{
    // Espero leyendo hasta que haya terminado (fcode = 0x03)
    // Asumo que el controlador esta prendido
    
uint16_t pvalue;

    memset( &cpres_mbus_cb, '\0', sizeof(mbus_CONTROL_BLOCK_t));
    memset( &cpres_mbchannel, '\0', sizeof(modbus_channel_conf_t));
    memset( &cpres_udata, '\0', sizeof(modbus_hold_t));
    
    // Escribo el valor de las valvulas (fcode=0x06)
    cpres_mbchannel.enabled = true;
    strcpy( &cpres_mbchannel.name, "VALVES");
    cpres_mbchannel.slave_address = 0x64;    // Slave 100
    cpres_mbchannel.reg_address = 0x01;      // Address 1
    cpres_mbchannel.nro_regs = 1;
    cpres_mbchannel.fcode = 0x3;             // Write
    cpres_mbchannel.type = u16;
    cpres_mbchannel.divisor_p10 = 0;
    cpres_mbchannel.codec = CODEC1032;
        
    cpres_mbus_cb.channel = cpres_mbchannel;
    cpres_mbus_cb.udata = cpres_udata;
    
    xprintf_P(PSTR("CPRES DEBUG\r\n"));
    
    modbus_io( &cpres_mbus_cb );
        
    // Si hubo un error, pongo un NaN y salgo
    if (cpres_mbus_cb.io_status == false) {
        xprintf_P(PSTR("CPRES RX error\r\n"));
        return(-1);
    }
        
    pvalue = cpres_mbus_cb.udata.u16_value;
    if ( pvalue & 0x80 == 0x00 ) {
        // Se setearon las valvulas. Salgo
        xprintf_P(PSTR("CPRES END OK\r\n"));
        return(0);
	}
    
    xprintf_P(PSTR("CPRES END FAIL\r\n"));
    return(1);
}
//------------------------------------------------------------------------------
void cpres_consigna_initService(void)
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
int8_t res;

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
        xprintf_P(PSTR("CONSIGNA Init:nocturna %02d:%02d [%04d]\r\n"),rtc.hour,rtc.min, now);
		res = consigna_set_nocturna();
        goto exit;
    }
    
    if ( ( now <= c_dia ) || ( c_noche <= now ) ) {
        // Aplico consigna nocturna
         xprintf_P(PSTR("CONSIGNA Init:nocturna %02d:%02d [%04d]\r\n"),rtc.hour,rtc.min, now);
		res = consigna_set_nocturna();
        goto exit;
       
    } else {
        // Aplico consigna diurna
        xprintf_P(PSTR("CONSIGNA Init:diurna %02d:%02d [%04d]\r\n"),rtc.hour,rtc.min, now);
		res = consigna_set_diurna();
        goto exit;
    }
    
exit:

    if ( res == -1 ) {
        xprintf_P(PSTR("CONSIGNA ERROR: TIMEOUT res = %d\r\n"), res);
    } else if ( res == 1 ) {
        xprintf_P(PSTR("CONSIGNA ERROR: ERR res = %d\r\n"), res);
    } 
    xprintf_P(PSTR("CONSIGNA OK: res = %d\r\n"), res);
    
}
//------------------------------------------------------------------------------
void cpres_consigna_service(void)
{
 
RtcTimeType_t rtc;
uint16_t now;
int8_t res;

	// Chequeo y aplico.
	// Las consignas se chequean y/o setean en cualquier modo de trabajo, continuo o discreto
	memset( &rtc, '\0', sizeof(RtcTimeType_t));
	if ( ! RTC_read_dtime(&rtc) ) {
		xprintf_P(PSTR("CONSIGNA ERROR: I2C:RTC chequear_consignas\r\n\0"));
		return;
	}

    now = rtc.hour * 100 + rtc.min;
    
	// Consigna diurna ?
	if ( now == systemConf.ptr_consigna_conf->consigna_diurna  ) {
		res = consigna_set_diurna();
		xprintf_P(PSTR("Set CONSIGNA diurna %02d:%02d [%04d]\r\n"),rtc.hour,rtc.min, now);
        if ( res == -1 ) {
            xprintf_P(PSTR("Set CONSIGNA diurna TO: res = %d\r\n"));
        }  
		return;
	}

	// Consigna nocturna ?
	if ( now == systemConf.ptr_consigna_conf->consigna_nocturna  ) {
		res = consigna_set_nocturna();
		xprintf_P(PSTR("Set CONSIGNA nocturna %02d:%02d [%04d]\r\n"),rtc.hour,rtc.min, now);
        if ( res == -1 ) {
            xprintf_P(PSTR("Set CONSIGNA nocturna TO: res = %d\r\n"));
        }
		return;
	}
    
}
//------------------------------------------------------------------------------


        // Siempre prendo: Si esta prendido no pasa nada
        //SET_EN_PWR_CPRES();
        //vTaskDelay( ( TickType_t)( 2000 / portTICK_PERIOD_MS ) );



    // Espero 10s que se apliquen las consignas y apago el modulo
    // Solo apago si estoy en modo discreto
/*   
if ( u_get_sleep_time(false) > 0 ){
        // Espero 10s que se apliquen las consignas y apago el modulo
        vTaskDelay( ( TickType_t)( 10000 / portTICK_PERIOD_MS ) );
        CLEAR_EN_PWR_CPRES(); 
    }
*/
