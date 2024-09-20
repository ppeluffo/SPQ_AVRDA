
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

static void p_modem_config_default_ose(void);
static void p_modem_config_default_spy(void);
static void p_config_default_besteffort(void);

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
    /*
     * La forma de entrar en modo AT es:
     * Se envia +++
     * El modem responde con 'a'
     * En los 3s siguientes enviar una 'a'
     * El módem responde con '+ok' indicando que estamos en modo comando.
     * 
     * Para salvar la configuracion damos el comando 'AT+S' con lo que salva y
     * reinicia el módulo.
     * 
     * Para salir del modo comando (sin salvar) es 'AT+ENTM'
     * 
     */
char *p;
bool retS = false;
uint16_t timer;

    /*
     * Para al modem al modo comandos
     * Transmite +++, espera un 'a' y envia un 'a' seguido de +ok.
     * 
     */
    if (verbose) {
        xprintf_P(PSTR("ModemTx-> +++\r\n"));
    }
    MODEM_flush_rx_buffer();
    xfprintf_P( fdWAN, PSTR("+++"));
    // Espero 3s la respuesta
    p = MODEM_get_buffer_ptr(); 
    for(timer=0; timer < 30; timer++) {
        vTaskDelay(100);
        // El modem responde con 'a'
        if  ( strstr( p, "a") != NULL ) {
            if (verbose) {
                xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
            }
            goto await_ok;
        }
    }

    if (verbose) {
        xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
    }
    // No respondio.
    return(false);

await_ok:

    // Envio 'a' y espero el '+ok'
    if (verbose) {
        xprintf_P(PSTR("ModemTx-> a\r\n"));
    }
    MODEM_flush_rx_buffer();
    xfprintf_P( fdWAN, PSTR("a"));
    for(timer=0; timer < 30; timer++) {
        vTaskDelay(100);
        // El modem responde con 'a'
        if  ( strstr( p, "+ok") != NULL ) {
            if (verbose) {
                xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
            }
            return(true);
        }
    }

    if (verbose) {
        xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
    }
    return(false);
}
//------------------------------------------------------------------------------
void MODEM_exit_mode_at(bool verbose)
{

char *p;

    /*
     * Salva los comandos y sale reseteando al modem
     */
    MODEM_flush_rx_buffer();
    xfprintf_P( fdWAN, PSTR("AT+ENTM\r\n"));
    vTaskDelay(1000);
    
    p = MODEM_get_buffer_ptr();
    if (verbose) {
        xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
    }
}
//------------------------------------------------------------------------------
void MODEM_save_atcommands(bool verbose)
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
    
    // AT+UARTFT?
    MODEM_flush_rx_buffer();
    xfprintf_P( fdWAN, PSTR("AT+UARTFT?\r\n"));
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
void MODEM_set_baudrate( char *baudrate)
{
    
char *p;

    p = MODEM_get_buffer_ptr();    
    MODEM_flush_rx_buffer();
    
    xfprintf_P( fdWAN, PSTR("AT+UART=%s,8,1,NONE,NONE\r\n"), baudrate);
    vTaskDelay(500);
    xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
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
void MODEM_set_ftime( char *time_ms)
{
    
char *p;

    p = MODEM_get_buffer_ptr();    
    MODEM_flush_rx_buffer();
    
    xfprintf_P( fdWAN, PSTR("AT+UARTFT=%s\r\n"), time_ms);
    vTaskDelay(500);
    xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
}
//------------------------------------------------------------------------------
void modem_print_configuration( void )
{
    /*
     * La usa el comando tkCmd::status.
     */
    

    xprintf_P(PSTR("Modem:\r\n"));
    xprintf_P(PSTR(" apn: %s\r\n"), modem_conf.apn);
    xprintf_P(PSTR(" ip: %s\r\n"), modem_conf.server_ip);
    xprintf_P(PSTR(" port: %s\r\n"), modem_conf.server_port);
    
    xprintf_P(PSTR(" imei: %s\r\n"), MODEM_get_imei());
    xprintf_P(PSTR(" iccid: %s\r\n"), MODEM_get_iccid());
    xprintf_P(PSTR(" csq: %d\r\n"), MODEM_get_csq());

}
//------------------------------------------------------------------------------
bool modem_config( char *s_arg, char *s_value )
{
    
    if (!strcmp_P( strupr(s_arg), PSTR("APN"))  ) {
        memset(modem_conf.apn,'\0',APN_LENGTH );
        strlcpy( modem_conf.apn, s_value, APN_LENGTH );
        return(true);
    }

    if (!strcmp_P( strupr(s_arg), PSTR("IP"))  ) {
        memset(modem_conf.server_ip,'\0', SERVER_IP_LENGTH);
        strlcpy( modem_conf.server_ip, s_value, SERVER_IP_LENGTH );
        return(true);
    }
    
    if (!strcmp_P( strupr(s_arg), PSTR("PORT"))  ) {
        memset(modem_conf.server_port,'\0', SERVER_PORT_LENGTH );
        strlcpy( modem_conf.server_port, s_value, SERVER_PORT_LENGTH );
        return(true);
    }
    
    return(false);
    
}
//------------------------------------------------------------------------------
void modem_config_defaults( char *s_arg )
{
       
    if ( s_arg == NULL ) {
        p_config_default_besteffort();
        return;
    }
    
    if (!strcmp_P( strupr(s_arg), PSTR("OSE"))  ) {
        p_modem_config_default_ose();
        return;
    }
     
    if (!strcmp_P( strupr(s_arg), PSTR("SPY"))  ) {
        p_modem_config_default_spy();
        return;
    }
   
}
//------------------------------------------------------------------------------
static void p_modem_config_default_ose(void)
{
    memset(modem_conf.apn,'\0',APN_LENGTH );
    memset(modem_conf.server_ip,'\0', SERVER_IP_LENGTH);
    memset(modem_conf.server_port,'\0', SERVER_PORT_LENGTH );

    strlcpy( modem_conf.apn, "STG1.VPNANTEL", APN_LENGTH );
    strlcpy( modem_conf.server_ip, "172.27.0.26", SERVER_IP_LENGTH );
    strlcpy( modem_conf.server_port, "5000", SERVER_PORT_LENGTH );
}
//------------------------------------------------------------------------------
static void p_modem_config_default_spy(void)
{
    memset(modem_conf.apn,'\0',APN_LENGTH );
    memset(modem_conf.server_ip,'\0', SERVER_IP_LENGTH);
    memset(modem_conf.server_port,'\0', SERVER_PORT_LENGTH );

    strlcpy( modem_conf.apn, "SPYMOVIL.VPNANTEL", APN_LENGTH );
    strlcpy( modem_conf.server_ip, "192.168.0.8", SERVER_IP_LENGTH );
    strlcpy( modem_conf.server_port, "5000", SERVER_PORT_LENGTH );   
}
//------------------------------------------------------------------------------
static void p_config_default_besteffort(void)
{
    /*
     * Vamos a tratar de adivinar que configuración tenia.
     * Por defecto usamos SPY.
     */
    xprintf_P(PSTR("Modem find best default...\r\n"));
    
    // SPY
    xprintf_P(PSTR("Trying SPY...\r\n"));
    //xprintf_P(PSTR("DEBUG APN: [%s]\r\n"), modem_conf.apn );
    //xprintf_P(PSTR("DEBUG IP: [%s]\r\n"), modem_conf.server_ip );
    
    if ( strstr(modem_conf.apn, "SPY") != NULL ) {
        p_modem_config_default_spy();
        return;
    }
    
    if ( strstr(modem_conf.server_ip, "192") != NULL ) {
        p_modem_config_default_spy();
        return;
    }
    
    if ( strstr(modem_conf.server_ip, "168") != NULL ) {
        p_modem_config_default_spy();
        return;
    }
    
    // OSE
    xprintf_P(PSTR("Trying STG1...\r\n"));
    if ( strstr(modem_conf.apn, "STG1") != NULL ) {
        p_modem_config_default_ose();
        return;
    }
    
    if ( strstr(modem_conf.server_ip, "172") != NULL ) {
        p_modem_config_default_ose();
        return;
    }

    if ( strstr(modem_conf.server_ip, "27") != NULL ) {
        p_modem_config_default_ose();
        return;
    }
    
    // DEFAULT
    xprintf_P(PSTR("Default SPY...\r\n"));
    p_modem_config_default_spy();
    
}
//------------------------------------------------------------------------------
char *modem_at_command(char *s_cmd)
{
    /*
     * Envia un comando AT agregando el CR LF al final y devuelve la respuesta
     */

char *p;

    p = MODEM_get_buffer_ptr();
    MODEM_flush_rx_buffer();
    xprintf_P(PSTR("ModemTx-> %s\r\n"), s_cmd );
    
    xfprintf_P( fdWAN, PSTR("%s\r\n"), s_cmd);
    vTaskDelay(500);
    
    xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
    return(p);
}
//------------------------------------------------------------------------------
bool modem_verify_configuration(void)
{
 
bool res;
char *p;

    res = true;

    xprintf_P(PSTR("MODEM: Verify config...\r\n"));

    // AT+HTPTP?
    p = modem_at_command("AT+HTPTP?");
    if (strstr(p, "GET") != NULL) {
        xprintf_P(PSTR("HTPTP OK\r\n"));
    } else {
        xprintf_P(PSTR("HTPTP FAIL\r\n"));
        res = false;
    }
    
    // AT+UART?
    p = modem_at_command("AT+UART?");
    if (strstr(p, "UART:115200,8,1,NONE,NONE") != NULL) {
        xprintf_P(PSTR("UART OK\r\n"));
    } else {
        xprintf_P(PSTR("UART FAIL\r\n"));
        res = false;
    }
    
    // AT+WKMOD?
    p = modem_at_command("AT+WKMOD?");
    if (strstr(p, "HTTPD") != NULL) {
        xprintf_P(PSTR("WKMOD OK\r\n"));
    } else {
        xprintf_P(PSTR("WKMOD FAIL\r\n"));
        res = false;
    }
    
    // AT+HTPURL?
    p = modem_at_command("AT+HTPURL?");
    if (strstr(p, "apidlg?") != NULL) {
        xprintf_P(PSTR("HTPURL OK\r\n"));
    } else {
        xprintf_P(PSTR("HTPURL FAIL\r\n"));
        res = false;
    }
    
    // AT+HTPSV?
    p = modem_at_command("AT+HTPSV?");
    if (strstr(p, modem_conf.server_ip) != NULL) {
        xprintf_P(PSTR("HTPSV ip OK\r\n"));
    } else {
        xprintf_P(PSTR("HTPSV ip FAIL\r\n"));
        res = false;
    }
    
    if (strstr(p, modem_conf.server_port) != NULL) {
        xprintf_P(PSTR("HTPSV port OK\r\n"));
    } else {
        xprintf_P(PSTR("HTPSV port FAIL\r\n"));
        res = false;
    }
    
    // AT+UARTFT?
    p = modem_at_command("AT+UARTFT?");
    if (strstr(p, "250") != NULL) {
        xprintf_P(PSTR("AT+UARTFT OK\r\n"));
    } else {
        xprintf_P(PSTR("AT+UARTFT FAIL\r\n"));
        res = false;
    }
    
    // AT+APN?
    p = modem_at_command("AT+APN?");
    if (strstr(p, modem_conf.apn) != NULL) {
        xprintf_P(PSTR("AT+APN OK\r\n"));
    } else {
        xprintf_P(PSTR("AT+APN FAIL\r\n"));
        res = false;
    }
    
    return(res);
}
//------------------------------------------------------------------------------
void modem_setup_default_params(void)
{
    /*
     * Configuramos un modem que en principio esta en default con los
     * parametros adecuados
     */
   
char *p;
char param_str[30];

    xprintf_P(PSTR("MODEM setup default params.\r\n"));

    // AT+HTPTP=GET
    p = modem_at_command("AT+HTPTP=GET");
    
    // AT+UART=115200,8,1,NONE,NONE
    p = modem_at_command("AT+UART=115200,8,1,NONE,NONE");
    
    // AT+WKMOD=HTTPD
    p = modem_at_command("AT+WKMOD=HTTPD");

    // AT+HTPURL/apidlg?
    p = modem_at_command("AT+HTPURL=/apidlg?");
    
    // AT+HTPSV=192.168.0.8,5000
    memset(param_str,'/0', strlen(param_str));
    snprintf( param_str, sizeof(param_str), "AT+HTPSV=%s,%s",modem_conf.server_ip, modem_conf.server_port);
    p = modem_at_command(param_str);
    
    // AT+UARTFT=250
    p = modem_at_command("AT+UARTFT=250");
    
    // AT+APN=SPYMOVIL.VPNANTEL,,,0
    memset(param_str,'/0', strlen(param_str));
    snprintf( param_str, sizeof(param_str), "AT+APN=%s,,,0", modem_conf.apn);
    p = modem_at_command(param_str);
    
    // Salvamos la configuracion y reinicia el modem
    p = modem_at_command("AT+S");
    
}
//------------------------------------------------------------------------------
bool modem_is_in_default(void)
{
    /*
     * Cuando queda en dafault, hay algunos parámetros que toman
     * valores que nos permite determinar el estado
     * 
     */
    
bool modem_in_default;
char *p;

    xprintf_P(PSTR("MODEM: Verify config default..\r\n"));
        
    modem_in_default = false;
    
    // AT+WKMOD?
    p = modem_at_command("AT+WKMOD?");
    if (strstr(p, "NET") != NULL) {
        modem_in_default = true;
        goto exit;
    }
    
    // AT+HTPURL?
    p = modem_at_command("AT+HTPURL?");
    if (strstr(p, "1.php?") != NULL) {
        modem_in_default = true;
        goto exit;
    }
            
    // AT+APN?
    p = modem_at_command("AT+APN?");
    if (strstr(p, "CMNET") != NULL) {
        modem_in_default = true;
        goto exit;
    }
    
exit:
    if (modem_in_default) {
        xprintf_P(PSTR("ALERT !!: Modem in default.\r\n"));
    }

    return(modem_in_default);
}
//------------------------------------------------------------------------------
void modem_read_and_config(void)
{
    /*
     * Leo los parametros del modem y configuro con estos el modem_conf
     */
    
char *p;

    xprintf_P(PSTR("MODEM: Read and config system..\r\n"));
    
    // AT+APN?
    p = modem_at_command("AT+APN?");
    
    if (strstr(p, "STG1") != NULL) {
        p_modem_config_default_ose();
        return;
    }
    
    if (strstr(p, "SPY") != NULL) {
        p_modem_config_default_spy();
        return;
    }
}
//------------------------------------------------------------------------------
