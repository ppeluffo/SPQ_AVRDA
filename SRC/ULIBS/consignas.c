
#include "consignas.h"

/*
 * consigna_status es un byte que almacena el valor a setear de c/consigna
 * El bit0 representa la valvula 0
 * El bit1 reprsenta la valvula 1
 * 
 * Los valores son: 0=cerrada, 1=abierta.
 * 
 * El modulo externo de control de presión lo vamos a manejar como un 
 * dispositivo MODBUS.
 * Van a tener fija la dirección 100 (0x64)
 * Las consignas van a tener un registro de 2 bytes en la posición 1 y 2.
 * El byte HIGH va a estar siempre en 0.
 * El byte LOW va a tener un valor de 0,1,2,3 dependiendo del estado que queremos
 * poner las válvulas, y el bit 7 en 1 indica que hay una operacion en curso y en 
 * 0 que ya termino y esta en reposo.
 * 
 * Para mandar una consigna debemos armar un MODBUS_BLOCK con los datos y realizar
 * una operacion de modbus.
 * La escritura implica que pongamos el valor de las valvulas y el bit RUN en 1.
 * Luego, leyendo el registro vemos cuando termino porque el bit RUN esta en 0.
 * 
 */
#define VALVULA0_CERRADA_gc (0x00 << 0)
#define VALVULA0_ABIERTA_gc (0x01 << 0)
#define VALVULA1_CERRADA_gc (0x00 << 1)
#define VALVULA1_ABIERTA_gc (0x01 << 1)


#ifdef OLD_CONSIGNAS
int FD_CONSIGNA;
// Puntero al inicio del buffer de modbus ( para copiarlo )
int CONSIGNA_BUFFER_SIZE;
// Puntero a la funcion que borra el buffer de recepcion de modbus.
void (*ptrFuncFlush) (void); 
// Puntero a la funcion que nos da cuantos bytes hay en el buffer de lectura
uint16_t (*ptrFuncGetCount) (void);
// Puntero que nos da la direccion de comienzo del buffer de recepcion 
char *(*ptrFunc_getRXBuffer) (void);

mbus_CONTROL_BLOCK_t consigna_mbus_cb;
modbus_channel_conf_t consigna_mbchannel;
modbus_hold_t consigna_udata;

void valvulas_set( uint8_t valve0_gc, uint8_t valve1_gc);

//------------------------------------------------------------------------------
void consigna_init( int fd_consigna, int buffer_size, void (*f)(void), uint16_t (*g)(void), char *(*h)(void)  )
{
    /*
     * Fijo cual es el fd por donde vamos a hacer las transmisiones
     * y cual funcion voy a usar para leer los datos recibidos.
     */
    
    FD_CONSIGNA = fd_consigna;
    CONSIGNA_BUFFER_SIZE = buffer_size;
    
    ptrFuncFlush = f;
    ptrFuncGetCount = g;
    ptrFunc_getRXBuffer = h;
    
}
//------------------------------------------------------------------------------
int8_t consigna_set_nocturna_old(void)
{
	// ( VA open / VB close ) -> ( VA close / VB open )
	// Open VB con lo que el punto común de las válvulas queda a pAtm y la VA puede operar correctamente.
	// Close VA.
    // Mando un comando modbus a la placa CONTROL_PRESION

char consigna_txbuffer[20];
uint8_t valves;
char *p;
uint16_t timeout_counts;

    valves = VALVULA1_CERRADA_gc | VALVULA0_ABIERTA_gc;
    consigna_aplicada = CONSIGNA_NOCTURNA;
    
    // Preparo el mensaje
    memset(consigna_txbuffer,'\0', sizeof(consigna_txbuffer));
    sprintf_P( consigna_txbuffer, PSTR("set valves %d"), valves );
    
    // Tomo el canal y transmito
    //SET_RTS_RS485();
	vTaskDelay( ( TickType_t)( 5 ) );       
    // Borro el rxBuffer antes de transmitir
    ptrFuncFlush();   
    
    xfprintf_P( FD_CONSIGNA, PSTR("%s\r"), consigna_txbuffer);
    vTaskDelay( ( TickType_t)( 2 ) );
	// RTS OFF: Habilita la recepcion del chip
	//CLEAR_RTS_RS485();
   
    xprintf_P( PSTR("CONSIGNA: Set nocturna [%s]\r\n"),consigna_txbuffer );
    
    p = ptrFunc_getRXBuffer();
    timeout_counts = 500;	// 500 x 10ms: espero 1 sec
    while ( timeout_counts-- > 0 ) {

        vTaskDelay( ( TickType_t)( 10 / portTICK_PERIOD_MS ) );
        
        if (!strcmp_P( p, PSTR("OK"))  ) {
            return(1);
        }

        if (!strcmp_P( p, PSTR("ERR"))  ) {
            return(0);
        }
    }
        
    return(-1);
    
}
//------------------------------------------------------------------------------
int8_t consigna_set_diurna_old(void)
{
    // Mando un comando modbus a la placa CONTROL_PRESION

char consigna_txbuffer[20];
uint8_t valves;
char *p;
uint16_t timeout_counts;

    valves = VALVULA1_ABIERTA_gc | VALVULA0_CERRADA_gc;
    consigna_aplicada = CONSIGNA_DIURNA;
    
    // Preparo el mensaje
    memset(consigna_txbuffer,'\0', sizeof(consigna_txbuffer));
    sprintf_P( consigna_txbuffer, PSTR("set valves %d"), valves );
    
    // Tomo el canal y transmito
    //SET_RTS_RS485();
	vTaskDelay( ( TickType_t)( 5 ) );
    // Borro el rxBuffer antes de transmitir
    ptrFuncFlush();   
    xfprintf_P( FD_CONSIGNA, PSTR("%s\r"), consigna_txbuffer);
    vTaskDelay( ( TickType_t)( 2 ) );
	// RTS OFF: Habilita la recepcion del chip
	//CLEAR_RTS_RS485();
    
    xprintf_P( PSTR("CONSIGNA: Set diurna [%s]\r\n"),consigna_txbuffer );
    
    p = ptrFunc_getRXBuffer();
    timeout_counts = 500;	// 500 x 10ms: espero 5 sec
    while ( timeout_counts-- > 0 ) {

        vTaskDelay( ( TickType_t)( 10 / portTICK_PERIOD_MS ) );
        
        if (!strcmp_P( p, PSTR("OK"))  ) {
            //xprintf_P(PSTR("RES OK\r\n"));
            return(1);
        }

        if (!strcmp_P( p, PSTR("ERR"))  ) {
            //xprintf_P(PSTR("RES ERR\r\n"));
            return(0);
        }
        
        //xprintf_P(PSTR("counts=%d, buff=%s\r\n"), timeout_counts, p);
    }
        
    //xprintf_P(PSTR("RES TO\r\n"));
    return(-1);
}
//------------------------------------------------------------------------------
#endif

int8_t consigna_set_diurna(void)
{
    // Mando un comando modbus a la placa CONTROL_PRESION

    consigna_aplicada = CONSIGNA_DIURNA;    
    cpres_set_valves(VALVULA1_ABIERTA_gc, VALVULA0_CERRADA_gc);
    return(1);
}
//------------------------------------------------------------------------------
int8_t consigna_set_nocturna(void)
{
    // Mando un comando modbus a la placa CONTROL_PRESION

    consigna_aplicada = CONSIGNA_NOCTURNA; 
    cpres_set_valves(VALVULA1_CERRADA_gc, VALVULA0_ABIERTA_gc);
    return(1);
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
