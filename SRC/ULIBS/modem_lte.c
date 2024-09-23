
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
char MODEM_APN[APN_LENGTH] = {'\0'};
char MODEM_IP[SERVER_IP_LENGTH] = {'\0'};
char MODEM_PORT[SERVER_PORT_LENGTH] = {'\0'};

uint8_t csq = 0;

static void modem_config_default_ose(void);
static void modem_config_default_spy(void);
static void modem_config_default_besteffort(void);

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
char *modem_atcmd(char *s_cmd, bool verbose)
{
    /*
     * Envia un comando AT agregando el CR LF al final y devuelve la respuesta
     */

char *p;

    // p queda apuntando al buffer de recepcion del uart del modem
    p = MODEM_get_buffer_ptr();
    // borro el buffer
    MODEM_flush_rx_buffer();
    
    if (verbose)
        xprintf_P(PSTR("ModemTx-> %s\r\n"), s_cmd );
    
    // Envio el comando y espero respuesta
    xfprintf_P( fdWAN, PSTR("%s\r\n"), s_cmd);
    vTaskDelay(500);
    
    if (verbose)
        xprintf_P(PSTR("ModemRx-> %s\r\n"), p );
    
    return(p);
}
//------------------------------------------------------------------------------
void modem_atcmd_save(bool verbose)
{

    /*
     * Salva los comandos y sale reseteando al modem
     */

    modem_atcmd("AT+S", verbose);
    
}
//------------------------------------------------------------------------------
void modem_atcmd_queryall(void)
{
    /*
     * Pregunta el valor de todos los parametros del modem y los muestra
     * en pantalla.
     * Es para usar en modo comando en modo verbose
     */

bool verbose = true;

    // AT+CSQ
    modem_atcmd("AT+CSQ", verbose);
    vTaskDelay(500);
    
    // AT+IMEI?
    modem_atcmd("AT+IMEI?", verbose);
    vTaskDelay(500);
    
    //AT+ICCID?
    modem_atcmd("AT+ICCID?", verbose);
    vTaskDelay(500);
     
    // AT+APN?
    modem_atcmd("AT+APN?", verbose);
    vTaskDelay(500);
     
    //AT+HTPURL?
    modem_atcmd("AT+HTPURL?", verbose);
    vTaskDelay(500);
    
    //AT+HTPTP?
    modem_atcmd("AT+HTPTP?", verbose);
    vTaskDelay(500);
    
    //AT+HTPSV?
    modem_atcmd("AT+HTPSV?", verbose);
    vTaskDelay(500);
    
    // AT+UARTFT?
    modem_atcmd("AT+UARTFT?", verbose);
    vTaskDelay(500);
}
//------------------------------------------------------------------------------
void modem_atcmd_read_iccid(bool verbose)
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
char *ts = NULL;
char *delim = ",\r ";
char *iccid = NULL;

    //AT+ICCID?
    memset(MODEM_ICCID,'\0', sizeof(MODEM_ICCID));
    p = modem_atcmd("AT+ICCID?", verbose);
      
    if ( (ts = strchr(p, ':')) ) {
        ts++;
        iccid = strtok(ts,delim);
        if ( iccid != NULL ) {
            //memcpy(MODEM_APN, apn, sizeof(MODEM_APN) );  
            memcpy(MODEM_ICCID, iccid, sizeof(MODEM_ICCID));  
        }

        if (verbose) {
            xprintf_P(PSTR("ICCID=[%s]\r\n"), MODEM_ICCID);
        }
    }
  
}
//------------------------------------------------------------------------------
char *modem_atcmd_get_iccid(void)
{
    /*
     * El ICCID debe estar leido en el buffer MODEM_ICCID.
     * Retorna el puntero al inicio del buffer.
     */
    return (MODEM_ICCID);
}
//------------------------------------------------------------------------------
void modem_atcmd_read_imei(bool verbose)
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
char *ts = NULL;
char *delim = ",\r ";
char *imei = NULL;

    // AT+IMEI?
    memset(MODEM_IMEI,'\0', sizeof(MODEM_IMEI));    
    p = modem_atcmd("AT+IMEI?", verbose);

    if ( (ts = strchr(p, ':')) ) {
        ts++;
        imei = strtok(ts,delim);
        if ( imei != NULL ) {
            //memcpy(MODEM_APN, apn, sizeof(MODEM_APN) );  
            memcpy(MODEM_IMEI, imei, sizeof(MODEM_IMEI));  
        }

        if (verbose) {
            xprintf_P(PSTR("IMEI=[%s]\r\n"), MODEM_IMEI);
        }
    }    
}
//------------------------------------------------------------------------------
char *modem_atcmd_get_imei(void)
{
    return (MODEM_IMEI);
}
//------------------------------------------------------------------------------
void modem_atcmd_read_csq(bool verbose)
{
char *p;
char *ts = NULL;

    // AT+CSQ
    p = modem_atcmd("AT+CSQ", verbose);
    
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
uint8_t modem_atcmd_get_csq(void)
{
    return(csq);
}
//------------------------------------------------------------------------------
void modem_atcmd_read_apn(bool verbose)
{
    /*
     * Asume que el modem esta prendido en modo ATcmd
     * 
     * Lee el APN
     * 
     * AT+APN
     * 
     * +APN:SPYMOVIL.VPNANTEL,,,1
     * 
     * OK
     * 
     */
char *p;
char *ts = NULL;
char *delim = ",";
char *apn = NULL;

    // AT+APN?
    
    memset(MODEM_APN,'\0', sizeof(MODEM_APN));

    p = modem_atcmd("AT+APN?", verbose);
    if ( (ts = strchr(p, ':')) ) {
        ts++;
        // ts apunta al primer caracter del apn.
        apn = strtok(ts,delim);
        if ( apn != NULL ) {
            //memcpy(MODEM_APN, apn, sizeof(MODEM_APN) );  
            memcpy(MODEM_APN, apn, strlen(apn));  
        }

        if (verbose) {
            xprintf_P(PSTR("APN=[%s]\r\n"), MODEM_APN);
        }
    }
       
}
//------------------------------------------------------------------------------
char *modem_atcmd_get_apn(void)
{
    return (MODEM_APN);
}
//------------------------------------------------------------------------------
void modem_atcmd_set_apn( char *apn, bool verbose)
{
    
char param_str[30];

    // AT+APN=SPYMOVIL.VPNANTEL,,,0
    memset(param_str,'\0', strlen(param_str));
    snprintf( param_str, sizeof(param_str), "AT+APN=%s,,,0", apn);
    modem_atcmd(param_str, verbose);
    vTaskDelay(500);

}
//------------------------------------------------------------------------------
char *modem_atcmd_get_server_ip(void)
{
    return (MODEM_IP);
}
//------------------------------------------------------------------------------
char *modem_atcmd_get_server_port(void)
{
    return (MODEM_PORT);
}
//------------------------------------------------------------------------------
void modem_atcmd_read_server(bool verbose)
{
    /*
     * Asume que el modem esta prendido en modo ATcmd
     * 
     * AT+HTPSV?
     * 
     * +HTPSV:192.168.0.8,5000
     * 
     * OK
     * 
     */
char *p;
char *ts = NULL;
char *ip, *port;
char *delim = ", \r\n"; // Va un espacio en blanco !!!
    
    // AT+HTPSV?
    p = modem_atcmd("AT+HTPSV?", verbose);

    memset(MODEM_IP,'\0', sizeof(MODEM_IP));
    memset(MODEM_PORT,'\0', sizeof(MODEM_PORT));
    
    // Arranco en los :
    if ( (ts = strchr(p, ':')) ) {
        ts++;
        // ts apunta al primer caracter del apn.
        ip = strtok(ts,delim);
        if ( ip != NULL ) {
            memcpy(MODEM_IP, ip, strlen(ip));  
        }

        if (verbose) {
            xprintf_P(PSTR("IP=[%s]\r\n"), MODEM_IP);
        }

        port = strtok(NULL,delim);
        if ( port != NULL ) {
            memcpy(MODEM_PORT, port, strlen(port));  
        }

        if (verbose) {
            xprintf_P(PSTR("PORT=[%s]\r\n"), MODEM_PORT);
        }
        
        
    }
      
}
//------------------------------------------------------------------------------
void modem_atcmd_set_server( char *ip, char *port, bool verbose)
{
    
char param_str[30];

    memset(param_str,'\0', strlen(param_str));
    snprintf( param_str, sizeof(param_str), "AT+HTPSV=%s,%s", ip, port);
    modem_atcmd(param_str, verbose);
    vTaskDelay(500);
}
//------------------------------------------------------------------------------
void modem_atcmd_set_apiurl( char *apiurl, bool verbose)
{
 
char param_str[30];

    memset(param_str,'\0', strlen(param_str));
    snprintf( param_str, sizeof(param_str), "AT+HTPURL=%s", apiurl);
    modem_atcmd(param_str, verbose);
    vTaskDelay(500);

}
//------------------------------------------------------------------------------
void modem_atcmd_set_ftime( char *time_ms, bool verbose)
{
 
char param_str[30];

    memset(param_str,'\0', strlen(param_str));
    snprintf( param_str, sizeof(param_str), "AT+UARTFT=%s", time_ms);
    modem_atcmd(param_str, verbose);
    vTaskDelay(500);

}
//------------------------------------------------------------------------------
// COMANDOS DE CONFIGURACION
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
    
    xprintf_P(PSTR(" imei: %s\r\n"), modem_atcmd_get_imei());
    xprintf_P(PSTR(" iccid: %s\r\n"), modem_atcmd_get_iccid());
    xprintf_P(PSTR(" csq: %d\r\n"), modem_atcmd_get_csq());

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
        modem_config_default_besteffort();
        return;
    }
    
    if (!strcmp_P( strupr(s_arg), PSTR("OSE"))  ) {
        modem_config_default_ose();
        return;
    }
     
    if (!strcmp_P( strupr(s_arg), PSTR("SPY"))  ) {
        modem_config_default_spy();
        return;
    }
   
}
//------------------------------------------------------------------------------
static void modem_config_default_ose(void)
{
    memset(modem_conf.apn,'\0',APN_LENGTH );
    memset(modem_conf.server_ip,'\0', SERVER_IP_LENGTH);
    memset(modem_conf.server_port,'\0', SERVER_PORT_LENGTH );

    strlcpy( modem_conf.apn, "STG1.VPNANTEL", APN_LENGTH );
    strlcpy( modem_conf.server_ip, "172.27.0.26", SERVER_IP_LENGTH );
    strlcpy( modem_conf.server_port, "5000", SERVER_PORT_LENGTH );
}
//------------------------------------------------------------------------------
static void modem_config_default_spy(void)
{
    memset(modem_conf.apn,'\0',APN_LENGTH );
    memset(modem_conf.server_ip,'\0', SERVER_IP_LENGTH);
    memset(modem_conf.server_port,'\0', SERVER_PORT_LENGTH );

    strlcpy( modem_conf.apn, "SPYMOVIL.VPNANTEL", APN_LENGTH );
    strlcpy( modem_conf.server_ip, "192.168.0.8", SERVER_IP_LENGTH );
    strlcpy( modem_conf.server_port, "5000", SERVER_PORT_LENGTH );   
}
//------------------------------------------------------------------------------
static void modem_config_default_besteffort(void)
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
        
    // OSE
    xprintf_P(PSTR("Trying STG1...\r\n"));
    if ( strstr(modem_conf.apn, "STG1") != NULL ) {
        modem_config_default_ose();
        return;
    }
    
    if ( strstr(modem_conf.server_ip, "172") != NULL ) {
        modem_config_default_ose();
        return;
    }

    if ( strstr(modem_conf.server_ip, "27") != NULL ) {
        modem_config_default_ose();
        return;
    }
    
    // DEFAULT
    xprintf_P(PSTR("Default SPY...\r\n"));
    modem_config_default_spy();
    
}
//------------------------------------------------------------------------------
bool modem_check_and_reconfig(bool verbose)
{
    /*
     * Revisa los parámetros del modem y va viendo si esta reseteado
     * a default y debe reconfigurarlo o no
     */
    
char *p;
char param_str[30];
bool modem_in_default = false;
bool save_dlg_config = false;

    xprintf_P(PSTR("MODEM config check and set...\r\n"));
    
    // AT+WKMOD?
    p = modem_atcmd("AT+WKMOD?", verbose);
    if (strstr(p, "HTTPD") == NULL) {
        modem_in_default = true;
        // Reconfiguro
        xprintf_P(PSTR("MODEM reconfig WKMOD.\r\n"));
        p = modem_atcmd("AT+WKMOD=HTTPD",verbose);    
    }
   
    // AT+HTPTP?
    p = modem_atcmd("AT+HTPTP?", verbose);
    if (strstr(p, "GET") == NULL) {
        modem_in_default = true;
        // Reconfiguro
        xprintf_P(PSTR("MODEM reconfig HTPTP.\r\n"));
        p = modem_atcmd("AT+HTPTP=GET",verbose);    
    }
        
     // AT+UART?
    p = modem_atcmd("AT+UART?", verbose);
    if (strstr(p, "UART:115200,8,1,NONE,NONE") == NULL) {
        modem_in_default = true;
        // Reconfiguro
        xprintf_P(PSTR("MODEM reconfig UART.\r\n"));
        p = modem_atcmd("AT+UART=115200,8,1,NONE,NONE",verbose);    
    }
    
    // AT+HTPURL?
    p = modem_atcmd("AT+HTPURL?", verbose);
    if (strstr(p, "apidlg?") == NULL) {
        modem_in_default = true;
        // Reconfiguro
        xprintf_P(PSTR("MODEM reconfig HTPURL.\r\n"));
        p = modem_atcmd("AT+HTPURL=/apidlg?",verbose);    
    }
    
    // AT+UARTFT?
    p = modem_atcmd("AT+UARTFT?", verbose);
    if (strstr(p, "250") == NULL) {
        modem_in_default = true;
        // Reconfiguro
        xprintf_P(PSTR("MODEM reconfig UARTFT.\r\n"));
        p = modem_atcmd("AT+UARTFT=250",verbose);    
    }
    
    /*
     */
    if ( modem_in_default ) {
        /*
         * Si el modem estaba en default, los parámetros del APN y SERVER válidos
         * sin los de la configuracion del datalogger.
         * Actualizo el módem con estos.
         */
        
        xprintf_P(PSTR("MODEM in default mode: reconfigue APN & SERVER..\r\n"));
        
        // AT+HTPSV=192.168.0.8,5000
        memset(param_str,'\0', strlen(param_str));
        snprintf( param_str, sizeof(param_str), "AT+HTPSV=%s,%s",modem_conf.server_ip, modem_conf.server_port);
        p = modem_atcmd(param_str, NULL);
    
        // AT+APN=SPYMOVIL.VPNANTEL,,,0
        memset(param_str,'\0', strlen(param_str));
        snprintf( param_str, sizeof(param_str), "AT+APN=%s,,,0", modem_conf.apn);
        p = modem_atcmd(param_str, NULL);
        goto quit;
        
    } else {
        /*
         * Si el modem NO esta resetado a default, puedo asumir que el APN y SERVER IP:PORT
         * son correctos.
         * Los leo y actualizo la configuracion del datalogger si son diferentes
         */
            
        xprintf_P(PSTR("MODEM not in default mode: verify apn & server...\r\n"));
        
        // AT+APN?  
        modem_atcmd_read_apn(verbose);
        if (verbose) {
            xprintf_P(PSTR("APN configurado en dlg: %s\r\n"), modem_conf.apn);
        }
        if (strstr(MODEM_APN, modem_conf.apn) == NULL) {
            // Actualizo el APN del datalogger
            if (verbose) {
                xprintf_P(PSTR("Update Dlg Apn to:%s\r\n"), MODEM_APN);
            }
            memset(modem_conf.apn, '\0', sizeof(modem_conf.apn));
            memcpy( modem_conf.apn, MODEM_APN, APN_LENGTH );
            save_dlg_config = true;
        }
        
        // AT+HTPSV?
        modem_atcmd_read_server(verbose);
        if (verbose) {
            xprintf_P(PSTR("Server configurado en dlg: %s:%s\r\n"), modem_conf.server_ip, modem_conf.server_port );
        }
        if (strstr(MODEM_IP, modem_conf.server_ip) == NULL) {
            // Actualizo el server_ip del datalogger
            if (verbose) {
                xprintf_P(PSTR("Update Dlg ip to:%s\r\n"), MODEM_IP);
            }
            memset(modem_conf.server_ip, '\0', SERVER_IP_LENGTH );
            memcpy( modem_conf.server_ip, MODEM_IP, SERVER_IP_LENGTH );
            save_dlg_config = true;
        }
    
        if (strstr( MODEM_PORT, modem_conf.server_port) == NULL) {
            // Actualizo el port del datalogger
            if (verbose) {
                xprintf_P(PSTR("Update Dlg port to:%s\r\n"), MODEM_PORT);
            }          
            memset(modem_conf.server_port, '\0', SERVER_PORT_LENGTH );
            memcpy( modem_conf.server_port, MODEM_PORT, SERVER_PORT_LENGTH );
            save_dlg_config = true;
        }

    }
            
 quit:
    
    // Salvamos la configuracion y reinicia el modem
    if ( modem_in_default ) {
        p = modem_atcmd("AT+S", NULL);
    }
    
    return(save_dlg_config);
    
}

//------------------------------------------------------------------------------
/*
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
    
    // Envio un mensaje por el enlace del LTE para verificar que llegue a la api
    
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
*/
//------------------------------------------------------------------------------
