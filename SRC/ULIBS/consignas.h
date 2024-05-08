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
    
#include "xprintf.h"
#include "toyi_valves.h"
#include "rtc79410.h"
#include "utils.h"
#include "pines.h"
    
typedef struct {
    bool enabled;
	uint16_t consigna_diurna;
	uint16_t consigna_nocturna;
} consigna_conf_t;

consigna_conf_t consigna_conf;

typedef enum { CONSIGNA_DIURNA = 0, CONSIGNA_NOCTURNA } consigna_t;

consigna_t consigna_aplicada;

void consigna_config_defaults(void);
bool consigna_config( char *s_enable, char *s_cdiurna, char *s_cnocturna );
void consigna_print_configuration(void);
uint8_t consigna_hash(void);
void consigna_set_diurna(void);
void consigna_set_nocturna(void);

#ifdef	__cplusplus
}
#endif

#endif	/* CONSIGNAS_H */

