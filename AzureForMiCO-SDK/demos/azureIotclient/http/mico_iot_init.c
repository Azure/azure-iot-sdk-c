#include "mico.h"
#include "mico_iot_init.h"

static mico_semaphore_t wait_sem = NULL;

//const char* ap_ssid = "Ruff_R0101965";
//const char* ap_wep_pwd = "Password01!";
//const char* ap_ssid = "Arthur-950";
//const char* ap_wep_pwd = "#Bugsfor$1";
const char* ap_ssid = "MSFTLAB";
const char* ap_wep_pwd = "";

static void micoNotify_WifiFailedHandler(OSStatus err, void* inContext)
{
    mico_iot_azure_log("Failed to join '%s': %d", ap_ssid, err);
}

static void micoNotify_WifiStatusHandler( WiFiEvent status, void* const inContext )
{
    UNUSED_PARAMETER( inContext );
    switch ( status )
    {
        case NOTIFY_STATION_UP:
            mico_iot_azure_log("%s was connected!", ap_ssid);
            mico_rtos_set_semaphore( &wait_sem );
            break;
        case NOTIFY_STATION_DOWN:
            mico_iot_azure_log("%s was disconnected!", ap_ssid);
            break;
        default:
            break;
    }
}

extern bool mico_iot_init(void)
{
    OSStatus err = kNoErr;
    network_InitTypeDef_adv_st  wNetConfigAdv;

    // Initialization
    MicoInit( );
    mico_rtos_init_semaphore( &wait_sem, 1 );

    /* Register user function when wlan connection status is changed */
    err = mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED, (void *)micoNotify_WifiStatusHandler, NULL );
    require_noerr( err, exit );

    /* Register user function when wlan connection is faild in one attempt */
    err = mico_system_notify_register( mico_notify_WIFI_CONNECT_FAILED, (void *)micoNotify_WifiFailedHandler, NULL );
    require_noerr( err, exit );

    // Get mac address of the board
    IPStatusTypedef para;
    micoWlanGetIPStatus(&para, 1);
    mico_iot_azure_log("mac address: %s.", (char*)para.mac);

    /* Initialize wlan parameters */
    memset( &wNetConfigAdv, 0x0, sizeof(wNetConfigAdv) );
    strcpy((char*)wNetConfigAdv.ap_info.ssid, ap_ssid);           /* wlan ssid string */
    strcpy((char*)wNetConfigAdv.key, ap_wep_pwd);                 /* wlan key string or hex data in WEP mode */
    wNetConfigAdv.key_len = strlen(ap_wep_pwd);                   /* wlan key length */
    wNetConfigAdv.ap_info.security = SECURITY_TYPE_AUTO;          /* wlan security mode */
    wNetConfigAdv.ap_info.channel = 0;                            /* Select channel automatically */
    wNetConfigAdv.dhcpMode = DHCP_Client;                         /* Fetch Ip address from DHCP server */
    wNetConfigAdv.wifi_retry_interval = 100;                      /* Retry interval after a failure connection */

    /* Connect Now! */
    mico_iot_azure_log("connecting to %s...", wNetConfigAdv.ap_info.ssid);
    micoWlanStartAdv(&wNetConfigAdv);

    /* Wait for wlan connection*/
    mico_rtos_get_semaphore( &wait_sem, MICO_WAIT_FOREVER );

    // Print ip address
    micoWlanGetIPStatus(&para, 1);
    mico_iot_azure_log("ip address: %s.", (char*)para.ip);

    return true;

exit:
    return false;
}
