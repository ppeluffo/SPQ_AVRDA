/*
 * l_eeprom.c
 *
 *  Created on: 11 dic. 2018
 *      Author: pablo
 */

#include "eeprom.h"

//------------------------------------------------------------------------------------
int16_t EE_test_write( char *addr, char *str, char *debug )
{
	// Funcion de testing de la EEPROM.
	// Escribe en una direccion de memoria un string
	// parametros: *addr > puntero char a la posicion de inicio de escritura
	//             *str >  puntero char al texto a escribir
	// retorna: -1 error
	//			nro.de bytes escritos

	// Calculamos el largo del texto a escribir en la eeprom.

int16_t xBytes = 0;
uint8_t length = 0;
char *p = NULL;
uint16_t start_address;

	p = str;
	while (*p != 0) {
		p++;
		length++;
	}

    start_address = (uint16_t)(atol(addr));

    if ((strcmp_P( strupr(debug), PSTR("DEBUG")) == 0) ) {
        xprintf_P(PSTR("EETEST_WRITE: start_address=0x%04x, length=%d\r\n"), start_address, length);
        xprintf_P(PSTR("EETEST_WRITE: debug on\r\n"));
        xBytes = EE_write( start_address, str, length, EE_DEBUG_ON );
    } else {
        xBytes = EE_write( start_address, str, length, EE_DEBUG_OFF );
    }
    
	if ( xBytes == -1 )
		xprintf_P(PSTR("EETEST_WRITE: ERROR=-1\r\n"));

	return(xBytes);
}
//------------------------------------------------------------------------------------
int16_t EE_test_read( char *addr, char *size, char *debug )
{
	// Funcion de testing de la EEPROM.
	// Lee de una direccion de la memoria una cantiad de bytes y los imprime
	// parametros: *addr > puntero char a la posicion de inicio de lectura
	//             *size >  puntero char al largo de bytes a leer
	// retorna: -1 error
	//			nro.de bytes escritos

int16_t xBytes = 0;
char buffer[32] = { 0 };
uint16_t start_address;

	// read ee {pos} {lenght}
    start_address = (uint16_t)(atol(addr));
    
    if ((strcmp_P( strupr(debug), PSTR("DEBUG")) == 0) ) {
        xprintf_P(PSTR("EETEST_READ: start_address=0x%04x\r\n"), start_address);
        xBytes = EE_read( start_address, buffer, (uint8_t)(atoi(size) ), EE_DEBUG_ON );
    } else {
        xBytes = EE_read( start_address, buffer, (uint8_t)(atoi(size) ), EE_DEBUG_OFF );
    }
    
	if ( xBytes == -1 )
		xprintf_P(PSTR("ERROR: I2C:EE_test_read\r\n"));

	if ( xBytes > 0 )
		xprintf_P( PSTR( "%s\r\n"),buffer);

	return (xBytes );

}
//------------------------------------------------------------------------------------
int16_t EE_read( uint16_t rdAddress, char *data, uint8_t length, bool debug )
{

int16_t rcode = 0;

    if (debug ) {
        rcode =  I2C_read( fdI2C1, DEVADDRESS_EEPROM_M2402, rdAddress, 0x02, data, length, EE_DEBUG_ON );
    } else {
        rcode =  I2C_read( fdI2C1, DEVADDRESS_EEPROM_M2402, rdAddress, 0x02, data, length, EE_DEBUG_OFF );
    }
    
	if ( rcode == -1 ) {
		// Hubo error: trato de reparar el bus y reintentar la operacion
		// Espero 1s que se termine la fuente de ruido.
		vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
		// Reconfiguro los dispositivos I2C del bus que pueden haberse afectado
		xprintf_P(PSTR("ERROR: EE_read recovering i2c bus\r\n") );
		goto quit;
	}

quit:

	return( rcode );
}
//------------------------------------------------------------------------------------
int16_t EE_write( uint16_t wrAddress, char *data, uint8_t length, bool debug )
{

int16_t rcode = 0;

    if (debug ) {
        xprintf_P(PSTR("EE_WRITE: debug on\r\n"));
        rcode =  I2C_write ( fdI2C1, DEVADDRESS_EEPROM_M2402, wrAddress, 0x02, data, length, EE_DEBUG_ON );
    } else {
        rcode =  I2C_write ( fdI2C1, DEVADDRESS_EEPROM_M2402, wrAddress, 0x02, data, length, EE_DEBUG_OFF );
    }
    
	if ( rcode == -1 ) {
		// Hubo error: trato de reparar el bus y reintentar la operacion
		// Espero 1s que se termine la fuente de ruido.
		vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
		// Reconfiguro los dispositivos I2C del bus que pueden haberse afectado
		xprintf_P(PSTR("EE_WRITE: ERROR=-1\r\n") );
	}

	return( rcode );

}
//------------------------------------------------------------------------------------
