/* 
 * File:   cpres.h
 * Author: pablo
 *
 * Created on 7 de junio de 2024, 10:05 AM
 */

#ifndef CPRES_H
#define	CPRES_H

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
    
#include "modbus.h"
#include "xprintf.h"
#include "rtc79410.h"
    
bool CPRES_IS_ON;
    
int8_t cpres_set_valves(uint8_t valve0_gc, uint8_t valve1_gc);
int8_t cpres_check_status(void);

void cpres_consigna_initService(void);
void cpres_consigna_service(void);

#ifdef	__cplusplus
}
#endif

#endif	/* CPRES_H */

