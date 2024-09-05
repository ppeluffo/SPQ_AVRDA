/*
 * File:   tkCtl.c
 * Author: pablo
 *
 * Created on 25 de octubre de 2021, 12:50 PM
 */


#include "SPQ_AVRDA.h"

#define TKCTL_DELAY_S	5

void sys_watchdog_check(void);
void sys_daily_reset(void);

const char string_0[] PROGMEM = "TK_CMD";
const char string_1[] PROGMEM = "TK_SYS";
const char string_2[] PROGMEM = "TK_WAN";
const char string_3[] PROGMEM = "TK_MODEMRX";
const char string_4[] PROGMEM = "TK_RS485RX";
const char string_5[] PROGMEM = "TK_CTLPRES";

const char * const wdg_names[] PROGMEM = { string_0, string_1, string_2, string_3, string_4, string_5 };

//------------------------------------------------------------------------------
void tkCtl(void * pvParameters)
{

	// Esta es la primer tarea que arranca.

( void ) pvParameters;
fat_s l_fat;

	vTaskDelay( ( TickType_t)( 500 / portTICK_PERIOD_MS ) );
    xprintf_P(PSTR("Starting tkCtl..\r\n"));
    
    ainputs_init();
    counter_init();
    
    // Inicializo los punteros
    systemConf.ptr_ainputs_conf = &ainputs_conf;
    systemConf.ptr_counter_conf = &counter_conf;
    systemConf.ptr_base_conf = &base_conf;
    systemConf.ptr_consigna_conf = &consigna_conf;
    systemConf.ptr_modbus_conf = &modbus_conf;
    systemConf.ptr_piloto_conf = &piloto_conf;
    
    // Leo la configuracion de EE en systemConf
    xprintf_P(PSTR("Loading config...\r\n"));
    if ( ! u_load_config_from_NVM())  {
       xprintf_P(PSTR("Loading config default..\r\n"));
       u_config_default();
    }
         
    // Inicializo la memoria EE ( fileSysyem)
	if ( FS_open() ) {
		xprintf_P( PSTR("FSInit OK\r\n"));
	} else {
        FAT_flush();
        xprintf_P( PSTR("FSInit FAIL !!.Reformatted...\r\n"));
		//FS_format(false );	// Reformateo soft.( solo la FAT )
		//xprintf_P( PSTR("FSInit FAIL !!.Reformatted...\r\n"));
	}

    wdt_reset();
    FAT_read(&l_fat);
	xprintf_P( PSTR("MEMsize=%d, wrPtr=%d, "),FF_MAX_RCDS, l_fat.head );
    xprintf_P( PSTR("rdPtr=%d, count=%d\r\n"),l_fat.tail, l_fat.count );

	// Imprimo el tamanio de registro de memoria
	xprintf_P( PSTR("RCD size %d bytes.\r\n"),sizeof(dataRcd_s));

	// Arranco el RTC. Si hay un problema lo inicializo.
    RTC_init();
    
    wdt_reset();
    vTaskDelay( ( TickType_t)( 2000 / portTICK_PERIOD_MS ) );
    
    
    // Por ultimo habilito a todas las otras tareas a arrancar
    starting_flag = true;
       
	for( ;; )
	{

        led_flash();
        sys_watchdog_check();
        sys_daily_reset();
        // xfprintf_P( fdXCOMMS, PSTR("The quick brown fox jumps over the lazy dog = %d\r\n"),a++);
        
        // Duerme 5 secs y corre.
        //vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
		vTaskDelay( ( TickType_t)( 1000 * TKCTL_DELAY_S / portTICK_PERIOD_MS ) );
        
	}
}
//------------------------------------------------------------------------------
void sys_watchdog_check(void)
{
    /*
     * El vector de watchdogs esta inicialmente en false.
     * Cada tarea con la funcion u_kick_wdt() pone su wdf en true.
     * Periodicamente lo ponemos en false.
     * Si al chequearlo, aparece alguno en false, es que no esta corriendo la
     * tarea que debería haberlo puesto en true por lo que nos reseteamos.
     * 
     */
    
static uint16_t wdg_count = 0;
uint8_t i;
char strBuffer[15] = { '\0' } ;

    /*
     * Cada 5s entro y reseteo el watchdog.
     * 
     */
    wdt_reset();
    return;
    
         
    /* EL wdg lo leo cada 240secs ( 5 x 60 )
     * Chequeo que cada tarea haya reseteado su wdg
     * ( debe haberlo puesto en true )
     */
    
    if ( wdg_count++ <  (120 / TKCTL_DELAY_S ) ) {
        return;
    }
    
    //xprintf_P(PSTR("DEBUG: wdg check\r\n"));
    wdg_count = 0;
    
    // Analizo los watchdows individuales
    //xTaskNotifyGive( xHandle_tkCmd);
    //vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
    //xprintf_P(PSTR("tkCtl: check wdg\r\n") );
    for (i = 0; i < RUNNING_TASKS; i++) {
        //xprintf_P(PSTR("tkCtl: check wdg[%d]->[%d][%d]\r\n"), i,tk_running[i], tk_watchdog[i]  );
        // Si la tarea esta corriendo...
        if ( tk_running[i] ) {
            // Si el wdg esta false es que no pudo borrarlo ( pasarlo a true) !!!
            if ( ! tk_watchdog[i] ) {
                          
                xTaskNotifyGive( xHandle_tkCmd);
                vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
                memset(strBuffer,'\0', sizeof(strBuffer));
				strcpy_P(strBuffer, (PGM_P)pgm_read_word(&(wdg_names[i])));
				xprintf_P( PSTR("ALARM !!!. TKCTL: RESET BY WDG (%s) !!\r\n"),strBuffer);     
                xprintf_P( PSTR("ALARM !!!. TKCTL: RESET BY WDG %d\r\n"), i );
                //reset();
                
                while(1)
                    ;
                
            }
        }
    }

    // Inicializo el vector de watchdogs
    for (i=0; i< RUNNING_TASKS; i++) {
        tk_watchdog[i] = false;
    }
    
}
//------------------------------------------------------------------------------
void sys_daily_reset(void)
{
	// Todos los dias debo resetearme para restaturar automaticamente posibles
	// problemas.
	// Se invoca 1 vez por minuto ( 60s ).

static uint32_t ticks_to_reset = 86400 / TKCTL_DELAY_S ; // ticks en 1 dia.

	//xprintf_P( PSTR("DEBUG dailyReset..\r\n\0"));
	while ( --ticks_to_reset > 0 ) {
		return;
	}

	xprintf_P( PSTR("Daily Reset !!\r\n\0") );
    vTaskDelay( ( TickType_t)( 2000 / portTICK_PERIOD_MS ) );
    reset();
    
}
//------------------------------------------------------------------------------
