
#include "consignas.h"

uint8_t consigna_status;

/*
 * consigna_status es un byte que almacena el valor a setear de c/consigna
 * El bit0 representa la valvula 0
 * El bit1 reprsenta la valvula 1
 * 
 * Los valores son: 0=cerrada, 1=abierta.
 */
#define VALVULA0_CERRADA_gc (0x00 << 0)
#define VALVULA0_ABIERTA_gc (0x01 << 0)
#define VALVULA1_CERRADA_gc (0x00 << 1)
#define VALVULA1_ABIERTA_gc (0x01 << 1)

//------------------------------------------------------------------------------
void consigna_set_nocturna(void)
{
	// ( VA open / VB close ) -> ( VA close / VB open )
	// Open VB con lo que el punto común de las válvulas queda a pAtm y la VA puede operar correctamente.
	// Close VA.
    // Mando un comando modbus a la placa CONTROL_PRESION

char consigna_txbuffer[10];
    
    consigna_status = VALVULA1_CERRADA_gc | VALVULA0_ABIERTA_gc;
    consigna_aplicada = CONSIGNA_NOCTURNA;
    
    // Preparo el mensaje
    memset(consigna_txbuffer,'\0', sizeof(consigna_txbuffer));
    sprintf_P( consigna_txbuffer, PSTR("CW=%d"), consigna_status );
    
    // Tomo el canal y transmito
    SET_RTS_RS485();
	vTaskDelay( ( TickType_t)( 5 ) );   
    xfprintf_P( fdRS485, PSTR("%s\r"), consigna_txbuffer);
    vTaskDelay( ( TickType_t)( 2 ) );
	// RTS OFF: Habilita la recepcion del chip
	CLEAR_RTS_RS485();
    
    xprintf_P( PSTR("CONSIGNA: Set nocturna [%s]\r\n"),consigna_txbuffer );
    
}
//------------------------------------------------------------------------------
void consigna_set_diurna(void)
{
    // Mando un comando modbus a la placa CONTROL_PRESION

char consigna_txbuffer[10];

    consigna_status = VALVULA1_ABIERTA_gc | VALVULA0_CERRADA_gc;
    consigna_aplicada = CONSIGNA_DIURNA;
    
    // Preparo el mensaje
    memset(consigna_txbuffer,'\0', sizeof(consigna_txbuffer));
    sprintf_P( consigna_txbuffer, PSTR("CW=%d"), consigna_status );
    
    // Tomo el canal y transmito
    SET_RTS_RS485();
	vTaskDelay( ( TickType_t)( 5 ) );   
    xfprintf_P( fdRS485, PSTR("%s\r"), consigna_txbuffer);
    vTaskDelay( ( TickType_t)( 2 ) );
	// RTS OFF: Habilita la recepcion del chip
	CLEAR_RTS_RS485();
    
    xprintf_P( PSTR("CONSIGNA: Set diurna [%s]\r\n"),consigna_txbuffer );
}
//------------------------------------------------------------------------------
void consigna_config_defaults(void)
{
    consigna_conf.enabled = false;
    consigna_conf.consigna_diurna = 700;
    consigna_conf.consigna_nocturna = 2300;
}
//------------------------------------------------------------------------------
bool consigna_config( char *s_enable, char *s_cdiurna, char *s_cnocturna )
{
    
    //xprintf_P(PSTR("CONSIGNA DEBUG: %s,%s,%s\r\n"),s_enable,s_cdiurna,s_cnocturna);
    
    if (!strcmp_P( strupr(s_enable), PSTR("TRUE"))  ) {
        consigna_conf.enabled = true;
    }
    
    if (!strcmp_P( strupr(s_enable), PSTR("FALSE"))  ) {
        consigna_conf.enabled = false;
    }
    
    consigna_conf.consigna_diurna = atoi(s_cdiurna);
    consigna_conf.consigna_nocturna = atoi(s_cnocturna);
    
    return (true);
    
}
//------------------------------------------------------------------------------
void consigna_print_configuration(void)
{
    xprintf_P( PSTR("Consigna:\r\n"));
    if (  ! consigna_conf.enabled ) {
        xprintf_P( PSTR(" status=disabled\r\n"));
        return;
    }
    
    xprintf_P( PSTR(" status=enabled"));
    if ( consigna_aplicada == CONSIGNA_DIURNA ) {
        xprintf_P(PSTR("(diurna)\r\n"));
    } else {
        xprintf_P(PSTR("(nocturna)\r\n"));
    }
    
	xprintf_P( PSTR(" cDiurna=%04d, cNocturna=%04d\r\n"), consigna_conf.consigna_diurna, consigna_conf.consigna_nocturna  );

}
//------------------------------------------------------------------------------
uint8_t consigna_hash( void )
{
    
uint8_t hash = 0;
uint8_t l_hash_buffer[64];
char *p;

   // Calculo el hash de la configuracion modbus

    memset( l_hash_buffer, '\0', sizeof(l_hash_buffer) );
    if ( consigna_conf.enabled ) {
        sprintf_P( (char *)&l_hash_buffer, PSTR("[TRUE,%04d,%04d]"),consigna_conf.consigna_diurna, consigna_conf.consigna_nocturna);
    } else {
        sprintf_P( (char *)&l_hash_buffer, PSTR("[FALSE,%04d,%04d]"),consigna_conf.consigna_diurna, consigna_conf.consigna_nocturna);
    }
    p = (char *)l_hash_buffer;
    while (*p != '\0') {
        hash = u_hash(hash, *p++);
    }
    //xprintf_P(PSTR("HASH_PILOTO:%s, hash=%d\r\n"), hash_buffer, hash ); 
    return(hash);   
}
//------------------------------------------------------------------------------
