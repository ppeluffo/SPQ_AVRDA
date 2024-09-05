/* 
 * File:   consignas.h
 * Author: pablo
 *
 * Created on 12 de septiembre de 2023, 10:26 AM
 */

#ifndef CONSIGNAS_H
#define	CONSIGNAS_H

#ifdef	__cplusplus
extern "C" {
#endif

    
#include "FreeRTOS.h"
#include "task.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "stdbool.h"
    
#include "xprintf.h"
#include "toyi_valves.h"
#include "rtc79410.h"
#include "utils.h"
#include "pines.h"
#include "modbus.h"
    
typedef struct {
    bool enabled;
	uint16_t consigna_diurna;
	uint16_t consigna_nocturna;
} consigna_conf_t;

consigna_conf_t consigna_conf;

typedef enum { CONSIGNA_DIURNA = 0, CONSIGNA_NOCTURNA } consigna_t;

consigna_t consigna_aplicada;

//void consigna_init( int fd_consigna, int buffer_size, void (*f)(void), uint16_t (*g)(void), char *(*h)(void)  );
void consigna_config_defaults(void);
bool consigna_config( char *s_enable, char *s_cdiurna, char *s_cnocturna );
void consigna_print_configuration(void);
void consigna_config_debug(bool debug );
uint8_t consigna_hash(void);
void consigna_prender_sensor(void);
void consigna_apagar_sensor(void);
bool consigna_debug_flag(void);

#ifdef	__cplusplus
}
#endif

#endif	/* CONSIGNAS_H */

