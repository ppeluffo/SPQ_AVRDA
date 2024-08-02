
#include "modem_lte.h"

/*
 * Para configurarlo se debe usar el modulo de conversion USB.
 * Se conecta TX a JP23
 *            RX a JP22
 *            GND a JP21
 * 
 * y se configura con el programa de USRIOT
 *       
 */



t_lte_pwr_status lte_pwr_status;

char MODEM_ICCID[30] = {'\0'};
char MODEM_IMEI[30] = {'\0'};
uint8_t csq = 0;

//--------------------------------------------------------------------------
void MODEM_init(void)
{
    CONFIG_LTE_EN_DCIN();
    CONFIG_LTE_EN_VCAP();
    CONFIG_LTE_PWR();
    CONFIG_LTE_RESET();
    CONFIG_LTE_RELOAD();
    
    CLEAR_LTE_EN_DCIN();    // No usamos DCIN
    CLEAR_LTE_EN_VCAP();    // No alimento
    SET_LTE_RESET();        // Reset debe estar H
    SET_LTE_RELOAD();       // Reload debe estar H
    SET_LTE_PWR();
    
    lte_pwr_status = LTE_PWR_OFF;
}
//--------------------------------------------------------------------------
void MODEM_prender(void)
{
    SET_LTE_EN_VCAP();     // alimento
    SET_LTE_PWR();         // pwr on
    
    lte_pwr_status = LTE_PWR_ON;
}
//--------------------------------------------------------------------------
void MODEM_apagar(void)
{
    CLEAR_LTE_PWR();         
    CLEAR_LTE_EN_VCAP();     
    
    lte_pwr_status = LTE_PWR_OFF;
    
}
//--------------------------------------------------------------------------
t_lte_pwr_status LTE_get_pwr_status(void)
{
    return(lte_pwr_status);
}
//--------------------------------------------------------------------------
char *LTE_buffer_ptr(void)
{
    return ( lBchar_get_buffer(&lte_lbuffer));
}
//------------------------------------------------------------------------------
void LTE_flush_buffer(void)
{
    lBchar_Flush(&lte_lbuffer);
}
//------------------------------------------------------------------------------
void lte_test_link(void)
{
    /*
     Envio un mensaje por el enlace del LTE para verificar que llegue a la api
     */
    //xfprintf_P( fdXCOMMS, PSTR("The quick brown fox jumps over the lazy dog\r\n"));
  
uint16_t timeout = 10;   
char *p;

    LTE_flush_buffer();
    xfprintf_P( fdWAN, PSTR("CLASS=TEST&EQUIPO=SPQ_AVRDA"));
    vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
    
    p = LTE_buffer_ptr();
    while ( timeout-- > 0) {
        vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
        if  ( strstr( p, "</html>") != NULL ) {
            xprintf_P( PSTR("rxbuff-> %s\r\n"), p);
            LTE_flush_buffer();
            return;
        }
    }
     xprintf_P( PSTR("RX TIMEOUT\r\n"));
}
//------------------------------------------------------------------------------
int MODEM_txmit(char *tx_buffer[])
{
    int i;
    
    i = xfprintf_P( fdWAN, PSTR("%s"), tx_buffer);
    return(i);
}
//------------------------------------------------------------------------------
char *MODEM_get_buffer_ptr(void)
{
    return ( lBchar_get_buffer(&modem_rx_lbuffer));
}
//------------------------------------------------------------------------------
void MODEM_flush_rx_buffer(void)
{
    lBchar_Flush(&modem_rx_lbuffer);
}
//------------------------------------------------------------------------------
bool MODEM_enter_mode_at(bool verbose)
{
    
char *p;
bool retS = false;
    /*
     * Para al modem al modo comandos
     * Transmite +++, espera un 'a' y envia un 'a' seguido de +ok.
     * 
     */
    MODEM_flush_rx_buffer();
    xfprintf_P( fdWAN, PSTR("+++"));
    vTaskDelay(100);
    xfprintf_P( fdWAN, PSTR("a"));
    vTaskDelay(100);
    
    // Deberia haber recibido +ok
    p = MODEM_get_buffer_ptr();
    if  ( strstr( p, "+ok") != NULL ) {
        retS = true;
    } else {
        retS = false;
    }
    
    if (verbose) {
        xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
    }
    return(retS);
}
//------------------------------------------------------------------------------
void MODEM_exit_mode_at(bool verbose)
{

char *p;

    /*
     * Salva los comandos y sale reseteando al modem
     */
    MODEM_flush_rx_buffer();
    xfprintf_P( fdWAN, PSTR("AT+S\r\n"));
    vTaskDelay(1000);
    
    p = MODEM_get_buffer_ptr();
    if (verbose) {
        xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
    }
}
//------------------------------------------------------------------------------
void MODEM_query_parameters(void)
{
    /*
     * Pregunta el valor de todos los parametros del modem y los muestra
     * en pantalla.
     * Es para usar en modo comando.
     */

char *p;

    p = MODEM_get_buffer_ptr();
    
    // AT+CSQ
    MODEM_flush_rx_buffer();
    xfprintf_P( fdWAN, PSTR("AT+CSQ\r\n"));
    vTaskDelay(500);
    xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
    
    // AT+IMEI?
    MODEM_flush_rx_buffer();
    xfprintf_P( fdWAN, PSTR("AT+IMEI?\r\n"));
    vTaskDelay(500);
    xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
    
    //AT+ICCID?
    MODEM_flush_rx_buffer();
    xfprintf_P( fdWAN, PSTR("AT+ICCID?\r\n"));
    vTaskDelay(500);
    xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
    
    // AT+APN?
    MODEM_flush_rx_buffer();
    xfprintf_P( fdWAN, PSTR("AT+APN?\r\n"));
    vTaskDelay(500);
    xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
    
    //AT+HTPURL?
    MODEM_flush_rx_buffer();
    xfprintf_P( fdWAN, PSTR("AT+HTPURL?\r\n"));
    vTaskDelay(500);
    xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
    
    //AT+HTPTP?
    MODEM_flush_rx_buffer();
    xfprintf_P( fdWAN, PSTR("AT+HTPTP?\r\n"));
    vTaskDelay(500);
    xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
    
    //AT+HTPSV?
    MODEM_flush_rx_buffer();
    xfprintf_P( fdWAN, PSTR("AT+HTPSV?\r\n"));
    vTaskDelay(500);
    xprintf_P(PSTR("ModemRx-> %s\r\n"), p );    
}
//------------------------------------------------------------------------------
void MODEM_read_iccid(bool verbose)
{
    /*
     * Asume que el modem esta prendido en modo ATcmd
     * 
     * +ICCID:8959801023149326185F
     * 
     * OK
     * 
     */
char *p;
char *ts;
uint8_t i = 0;

    //AT+ICCID?
    MODEM_flush_rx_buffer();
    p = MODEM_get_buffer_ptr();
    ts = NULL;
    xfprintf_P( fdWAN, PSTR("AT+ICCID?\r\n"));
    vTaskDelay(500);
    if (verbose) {
        xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
    }
    memset(MODEM_ICCID,'\0', sizeof(MODEM_ICCID));
          
    if ( (ts = strchr(p, ':')) ) {
        ts++;
        memcpy(MODEM_ICCID, ts, sizeof(MODEM_ICCID) );
        // Elimino el CR
        for (i=0; i<sizeof(MODEM_ICCID); i++) {
            //xprintf_P(PSTR("DEBUG %d[%c][0x%02x]\r\n"),i,MODEM_ICCID[i],MODEM_ICCID[i]);
            
            if ( ! isalnum((uint8_t)MODEM_ICCID[i]) ) {
                //xprintf_P(PSTR("DEBUG %d not alphanum\r\n"));
                MODEM_ICCID[i] = '\0';
            } 
        }
        if (verbose) {
            xprintf_P(PSTR("ICCID=[%s]\r\n"), MODEM_ICCID);
        }
    }
       
}
//------------------------------------------------------------------------------
char *MODEM_get_iccid(void)
{
    return (&MODEM_ICCID);
}
//------------------------------------------------------------------------------
void MODEM_read_imei(bool verbose)
{
    /*
     * Asume que el modem esta prendido en modo ATcmd
     * 
     * Lee el IMEI 
     * 
     * +IMEI:868191051391785
     * 
     * OK
     * 
     */
char *p;
char *ts;
uint8_t i = 0;

    // AT+IMEI?
    MODEM_flush_rx_buffer();
    p = MODEM_get_buffer_ptr();
    ts = NULL;
    xfprintf_P( fdWAN, PSTR("AT+IMEI?\r\n"));
    vTaskDelay(500);
    if (verbose) {
        xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
    }
    memset(MODEM_IMEI,'\0', sizeof(MODEM_IMEI));
    if ( (ts = strchr(p, ':')) ) {
        ts++;
        memcpy(MODEM_IMEI, ts, sizeof(MODEM_IMEI) );
        // Elimino el CR
        for (i=0; i<sizeof(MODEM_IMEI); i++) {
            if ( ! isalnum((uint8_t)MODEM_IMEI[i]) ) {
                MODEM_IMEI[i] = '\0';
            } 
        }
        if (verbose) {
            xprintf_P(PSTR("IMEI=[%s]\r\n"), MODEM_IMEI);
        }
    }
    
}
//------------------------------------------------------------------------------
char *MODEM_get_imei(void)
{
    return (&MODEM_IMEI);
}
//------------------------------------------------------------------------------
void MODEM_read_csq(bool verbose)
{
char *p;
char *ts;

    // AT+CSQ
    MODEM_flush_rx_buffer();
    p = MODEM_get_buffer_ptr();
    ts = NULL;
    xfprintf_P( fdWAN, PSTR("AT+CSQ\r\n"));
    vTaskDelay(500);
    if (verbose) {
        xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
    }
    
    if ( (ts = strchr(p, ':')) ) {
        ts++;
        csq = atoi(ts);
        csq = 113 - 2 * csq;
    }
    
    if (verbose) {
        xprintf_P(PSTR("CSQ=[%d]\r\n"), csq);
    } 
}
//------------------------------------------------------------------------------
uint8_t MODEM_get_csq(void)
{
    return(csq);
}
//------------------------------------------------------------------------------
void MODEM_set_apn( char *apn)
{
    
char *p;

    p = MODEM_get_buffer_ptr();    
    MODEM_flush_rx_buffer();
    
    xfprintf_P( fdWAN, PSTR("AT+APN=%s,,,0\r\n"), apn);
    vTaskDelay(500);
    xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
}
//------------------------------------------------------------------------------
void MODEM_set_server( char *ip, char *port)
{
    
char *p;

    p = MODEM_get_buffer_ptr();    
    MODEM_flush_rx_buffer();
    
    xfprintf_P( fdWAN, PSTR("AT+HTPSV=%s,%s\r\n"), ip, port);
    vTaskDelay(500);
    xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
}
//------------------------------------------------------------------------------
void MODEM_set_apiurl( char *apiurl)
{
    
char *p;

    p = MODEM_get_buffer_ptr();    
    MODEM_flush_rx_buffer();
    
    xfprintf_P( fdWAN, PSTR("AT+HTPURL=%s\r\n"), apiurl);
    vTaskDelay(500);
    xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
}
//------------------------------------------------------------------------------
