
/*
 * File:   spxR2_main.c
 * Author: pablo
 *
 * Created on 25 de octubre de 2021, 11:55 AM
 * 
 * Debido a que es un micro nuevo, conviene ir a https://start.atmel.com/ y
 * crear un projecto con todos los perifericos que usemos y bajar el codigo
 * para ver como se inicializan y se manejan.
 *
 * -----------------------------------------------------------------------------
 * Version 1.3.2 @ 20240814
 * - Al arrancar leemos el valor del vector de reset para saber la causa del reset.
 *   Este valor lo enviamos en el frame de CONF_BASE y luego lo borramos.
 * - Modificamos el frame de CONF_BASE paa que mande el valor del watchdog.
 *   Esto implica que el modem debe reconfigurarse para que el packaging time sea
 *   de 250 y permita frames más largos.
 * - Agregamos en el comando 'test modem queryall' que lea el packaging time, y 
 *   agregamos un comando para modificarlo: 'test modem set ftime'
 * - Agrego en el menu de test de valve operar sobre el pin enable y ctl.
 * - La incializacion de la valve a open la hago dentro del tkCtlPresion ya que 
 *   demora 10s y si la hago en tkCtl el watchdog reseteaba el micro. 
 * -----------------------------------------------------------------------------
 * Version 1.3.1 @ 20240813
 * - Cuando imprimo en pantalla en los menues uso strings cortos para no generar
 *   errores de caracteres.
 * - Controlo la causa del reset en el arranque y la mando en el primer frame
 * - El manejo de las tareas corriendo y los watchdows lo hago por medio de 
 *   vectores de bools.
 * -----------------------------------------------------------------------------
 * Version 1.2.5 @ 2024-06-04:
 * - Elimino el semaforo de modbus y creo uno global sem_RS485 que lo piden todos
 *   los que deban acceder al puerto.
 * ----------------------------------------------------------------------------- 
 * Version 1.2.1 @ 2024-04-10:
 * - cambio las funciones strncpy por strlcpy
 * - PENDIENTE: Controlar los SNPRINTF
 * -----------------------------------------------------------------------------
 * Version 1.2.0 @ 2024-04-08:
 * Aumento el tamaño de los buffers en las funciones que hacen hash
 * -----------------------------------------------------------------------------
 * Version 1.1.0 @ 2023-11-15:
 * Modbus: El caudalimentro Shinco tiene s/n 77 entonces el id es 119 !!!
 * -----------------------------------------------------------------------------
 * Version 1.1.0 @ 2023-07-21
 * Veo que hay veces que se cuelga o que tareas ocupan mas tiempo del esperable.
 * Esto se refleja en el flasheo del led e indica problemas con la asignacion de
 * tareas.
 * https://www.freertos.org/FreeRTOS_Support_Forum_Archive/December_2013/freertos_configUSE_TIME_SLICING_278999daj.html
 * https://www.freertos.org/FreeRTOS_Support_Forum_Archive/February_2018/freertos_Preemption_by_an_equal_priority_task_c8cb93d0j.html
 * 
 * #define configUSE_TIME_SLICING 1
 * 
 * El problema esta en la funcion FLASH_0_read_eeprom_block.
 * La solución fue poner vTaskDelay( ( TickType_t)( 10 ) ) ente c/escritura de bytes.
 * 
 * Control del tamaño de las colas:
 * https://www.freertos.org/FAQMem.html#StackSize
 * https://www.freertos.org/FreeRTOS_Support_Forum_Archive/November_2015/freertos_Measuring_Stack_Usage_b5706bb4j.html
 * 
 * -----------------------------------------------------------------------------
 * Version 1.0.0 @ 2022-09-12
  * 
 * Para que imprima floats hay que poner en el linker options la opcion
 * -Wl,-u,vfprintf -lprintf_flt -lm
 * https://github.com/microchip-pic-avr-examples/avr128da48-cnano-printf-float-mplab-mcc
 * 
 * Log en el servidor:
 * hset 'SPCOMMS' 'DEBUG_DLGID' 'PPOTKIYU'
 * 
 * 
 * PENDIENTE:
 * 1- Transmitir en modo bloque al hacer un dump.
 * 2- Consumo: entrar en modo tickless
 *
 * -----------------------------------------------------------------------------
 * V1.1.0 @ 20230626
 * Hay muchos equipos que no pasan de la configuracion.
 * El problema es que se resetea x wdt. No queda claro porque pero con un
 * wdg_reset en  wan_state_online_data se arregla.
 * HAY QUE REVISAR TODO EL TEMA DE LOS WDGs. !!!!
 * 1,848,861-8
 * Patricia Cruz y José Montejo
 * LITERATURA CRUZ BARISIONE, Patricia
 * Iris e Inés Montejo
 * http://www.CABRERA/MONS/ENRIQUE/Montevideo/Padron/2354U
 * 18488618
 * 22923645
 * 
 * -----------------------------------------------------------------------------
 * V1.1.0 @ 20230620
 * Si el canal analogico 2 esta configurado, entonces la bateria es 0.
 * El protocolo esta modificado para hablar con APICOMMSV3
 * -----------------------------------------------------------------------------
 * V1.0.8 @ 20230505
 * En los contadores incorporo el concepto de rb_size que sirve para determinar
 * cuanto quiero promediar los contadores.
 * El problema surge en perforaciones Colonia que no es util un promediado largo.
 * 
 * -----------------------------------------------------------------------------
 * V1.0.7 @ 20230306
 * Incorporamos el modulo de modbus.
 * Manejamos 5 canales (float) por lo que aumentamos el FS_BLOCKSIZE a 64.
 * Falta:
 *  - A veces da el pc timeout ? Es como si el frame se fragmentara
 *  - WAN
 * 
 * - BUG: No podia escribir el RTC
 * - BUG: Los canales analogicos, contadores, modbus por default deben ser 'X'
 * - BUG: Los contadores al medir caudal nunca llegaban a 0. Agrego en counters_clear()
 *        que converjan a 0.
 * - BUG: No calculaba bien el hash de conf_base y por eso se reconfigura siempre
 * 
 *  
 * -----------------------------------------------------------------------------
 * V1.0.6 @ 20230202
 * Solo manejo 2 canales y el 3o es la bateria.
 * 
 * V1.0.5 @ 20230202
 * Configuro 'samples_count' y 'alarm_level' por la wan.
 *  
 * V1.0.4 @ 20230118
 * Incorporo los conceptos de 'samples_count' y 'alarm_level'
 *  
 * 
 * V1.0.3 @ 20230109
 * Agrego logica de manejo de reles K1,K2. El modem esta en K1.
 * Revisar el reset memory hard que bloquea al dlg.
 * 
 * 
 * V1.0.2 @ 2023-01-02
 * Agrego en la funcion ainputs_test_read_channel() que prenda y apague los sensores.
 * 
 * V1.2.4 @ 2024-05-03:
 * BUG: La velocidad del bus I2C estaba mal seteada, suponiendo un clock de 4Mhz y no de 24Mhz
 * Lo soluciono cambiando el #define al ppio del archivo drv_i2c_avrDX.c
 * 
 * V1.2.6 @ 2024-06-24:
 * Cambio la velocidad del puerto TERM a 9600 para que quede compatible
 * con los bluetooh que usan los técnicos.
 *  
 */

#include "SPQ_AVRDA.h"

/*

FUSES = {
	.WDTCFG = 0x00, // WDTCFG {PERIOD=OFF, WINDOW=OFF}
	.BODCFG = 0x00, // BODCFG {SLEEP=DISABLE, ACTIVE=DISABLE, SAMPFREQ=128Hz, LVL=BODLEVEL0}
	.OSCCFG = 0xF8, // OSCCFG {CLKSEL=OSCHF}
	.SYSCFG0 = 0xD2, // SYSCFG0 {EESAVE=CLEAR, RSTPINCFG=GPIO, CRCSEL=CRC16, CRCSRC=NOCRC}
	.SYSCFG1 = 0xF8, // SYSCFG1 {SUT=0MS}
	.CODESIZE = 0x00, // CODESIZE {CODESIZE=User range:  0x0 - 0xFF}
	.BOOTSIZE = 0x00, // BOOTSIZE {BOOTSIZE=User range:  0x0 - 0xFF}
};

LOCKBITS = 0x5CC5C55C; // {KEY=NOLOCK}
*/


FUSES = {
	.WDTCFG = 0x0B, // WDTCFG {PERIOD=8KCLK, WINDOW=OFF}
	.BODCFG = 0x00, // BODCFG {SLEEP=DISABLE, ACTIVE=DISABLE, SAMPFREQ=128Hz, LVL=BODLEVEL0}
	.OSCCFG = 0xF8, // OSCCFG {CLKSEL=OSCHF}
	.SYSCFG0 = 0xD2, // SYSCFG0 {EESAVE=CLEAR, RSTPINCFG=GPIO, CRCSEL=CRC16, CRCSRC=NOCRC}
	.SYSCFG1 = 0xF8, // SYSCFG1 {SUT=0MS}
	.CODESIZE = 0x00, // CODESIZE {CODESIZE=User range:  0x0 - 0xFF}
	.BOOTSIZE = 0x00, // BOOTSIZE {BOOTSIZE=User range:  0x0 - 0xFF}
};

LOCKBITS = 0x5CC5C55C; // {KEY=NOLOCK}

//------------------------------------------------------------------------------
int main(void) {

uint8_t i;
    
// Leo la causa del reset para trasmitirla en el init.
	wdg_resetCause = RSTCTRL.RSTFR;
	RSTCTRL.RSTFR = 0xFF;
    
    // Init out of rtos !!!
    system_init();
    
    frtos_open(fdTERM, 9600 );
    frtos_open(fdWAN, 9600 );
    frtos_open(fdRS485, 9600 );
    frtos_open(fdI2C1, 100 );
    frtos_open(fdNVM, 0 );
    
    sem_SYSVars = xSemaphoreCreateMutexStatic( &SYSVARS_xMutexBuffer );
//    sem_XCOMMS = xSemaphoreCreateMutexStatic( &XCOMMS_xMutexBuffer );
    sem_RS485 = xSemaphoreCreateMutexStatic( &RS485_xMutexBuffer );
    
    FS_init();
    ainputs_init_outofrtos();
    counter_init_outofrtos();
    modbus_init_outofrtos();
    piloto_init_outofrtos();
    
    // Inicializo el vector de tareas activas
    for (i=0; i< RUNNING_TASKS; i++) {
        tk_running[i] = false;
    }

    // Inicializo el vector de watchdogs
    for (i=0; i< RUNNING_TASKS; i++) {
        tk_watchdog[i] = false;
    }
    
    starting_flag = false;
    
    xHandle_tkCtl = xTaskCreateStatic( tkCtl, "CTL", tkCtl_STACK_SIZE, (void *)1, tkCtl_TASK_PRIORITY, tkCtl_Buffer, &tkCtl_Buffer_Ptr );
    xHandle_tkCmd = xTaskCreateStatic( tkCmd, "CMD", tkCmd_STACK_SIZE, (void *)1, tkCmd_TASK_PRIORITY, tkCmd_Buffer, &tkCmd_Buffer_Ptr );
    xHandle_tkSys = xTaskCreateStatic( tkSys, "SYS", tkSys_STACK_SIZE, (void *)1, tkSys_TASK_PRIORITY, tkSys_Buffer, &tkSys_Buffer_Ptr );
    xHandle_tkModemRX = xTaskCreateStatic( tkModemRX, "MODEMRX", tkModemRX_STACK_SIZE, (void *)1, tkModemRX_TASK_PRIORITY, tkModemRX_Buffer, &tkModemRX_Buffer_Ptr );
    xHandle_tkWan = xTaskCreateStatic( tkWan, "WAN", tkWan_STACK_SIZE, (void *)1, tkWan_TASK_PRIORITY, tkWan_Buffer, &tkWan_Buffer_Ptr );
    xHandle_tkRS485RX = xTaskCreateStatic( tkRS485RX, "RS485", tkRS485RX_STACK_SIZE, (void *)1, tkRS485RX_TASK_PRIORITY, tkRS485RX_Buffer, &tkRS485RX_Buffer_Ptr );
    xHandle_tkCtlPresion = xTaskCreateStatic( tkCtlPresion, "CPRES", tkCtlPresion_STACK_SIZE, (void *)1, tkCtlPresion_TASK_PRIORITY, tkCtlPresion_Buffer, &tkCtlPresion_Buffer_Ptr );

    /* Arranco el RTOS. */
	vTaskStartScheduler();
  
	// En caso de panico, aqui terminamos.
	exit (1);
    
}
//------------------------------------------------------------------------------
/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
//------------------------------------------------------------------------------------
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

	/* Pass out a pointer to the StaticTask_t structure in which the Timer
	task's state will be stored. */
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

	/* Pass out the array that will be used as the Timer task's stack. */
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;

	/* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
	Note that, as the array is necessarily of type StackType_t,
	configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
//-----------------------------------------------------------------------------


