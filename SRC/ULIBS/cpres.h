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
    
void cpres_set_valves(uint8_t valve0_gc, uint8_t valve1_gc);
int8_t cpres_check_status(void);

#ifdef	__cplusplus
}
#endif

#endif	/* CPRES_H */

