/**
 * NetHander.c �����ʼ������
 * Describtion:
 * Author: qinfei 2015.04.09
 * Version: GatewayV1.0
 * Support:qf.200806@163.com
 */
#include "NetHandler.h"
#include "delay.h"//��ʱ����
#include "usart.h"//����
#include "stm32timer.h"
#include "spi.h"
#include "socket.h"//Just include one header for WIZCHIP
#include "dhcp.h"
#include "dns.h"
#include <string.h>

uint8_t memsize[2][8] = {{2,2,2,2,2,2,2,2},{2,2,2,2,2,2,2,2}}; /* WIZCHIP SOCKET Buffer initialize */

uint8_t domain_ip[4]={0};/*����IP*/
uint8_t domain_name[]="yeelink.net";/*����*/

/* Private macro -------------------------------------------------------------*/
uint8_t gDATABUF[DATA_BUF_SIZE];//��ȡ���ݵĻ�������2048

/*Ĭ������IP��ַ����*/
wiz_NetInfo gWIZNETINFO = { .mac = {0x00, 0x08, 0xdc,0x00, 0xab, 0xcd},//MAC��ַ
                            .ip = {192, 168, 0, 127},                  //IP��ַ
                            .sn = {255,255,255,0},                     //��������
                            .gw = {192, 168, 0, 1},                    //Ĭ������
                            .dns = {211,161,192,13},                   //DNS������
                            .dhcp = NETINFO_STATIC };

/* Private functions ---------------------------------------------------------*/
static void RegisterSPItoW5500(void);/*��SPI�ӿں���ע�ᵽW5500��socket����*/
static void InitW5500SocketBuf(void);/*��ʼ��W5500����оƬ*/
static void PhyLinkStatusCheck(void);/* PHY��·״̬���*/
static void DhcpInitHandler(void);   /*DHCP��ʼ��*/
static void my_ip_assign(void);      /*��̬����IP*/          
static void my_ip_conflict(void);    /*IP��ַ��ͻ�ļ򵥻ص�����*/
void DNS_Analyse(void);/*DNS����*/

/* IP��ַ��ͻ�ļ򵥻ص����� */
static void my_ip_conflict(void)
{
    printf("CONFLICT IP from DHCP\r\n");
    
    //halt or reset or any...
    while(1); // this example is halt.
}

/*******************************************************
 * @ brief Call back for ip assing & ip update from DHCP
 * ��̬����IP����Ϣ
 *******************************************************/
static void my_ip_assign(void)
{
   getIPfromDHCP(gWIZNETINFO.ip);  //IP��ַ
   getGWfromDHCP(gWIZNETINFO.gw);  //Ĭ������
   getSNfromDHCP(gWIZNETINFO.sn);  //��������
   getDNSfromDHCP(gWIZNETINFO.dns);//DNS������
   gWIZNETINFO.dhcp = NETINFO_DHCP;
   
   /* Network initialization */
   network_init();//Ӧ��DHCP����������ַ���������ʼ��
   printf("DHCP LEASED TIME : %d Sec.\r\n", getDHCPLeasetime());//��ӡ��õ�DHCP�����ַʱ��

   DNS_Analyse();//��������
}

/******************************************************************************
 * @brief  Network Init
 * Intialize the network information to be used in WIZCHIP
 *****************************************************************************/
void network_init(void)
{
    uint8_t tmpstr[6] = {0};
    wiz_NetInfo netinfo;

    /*����gWIZNETINFO�ṹ������������Ϣ*/
    //DNS������gWIZNETINFO�����б���������ȷ����������޷���ȷ����������
    ctlnetwork(CN_SET_NETINFO, (void*)&gWIZNETINFO);

    /*��ȡ���ú��������Ϣ����netinfo�ṹ��*/
    ctlnetwork(CN_GET_NETINFO, (void*)&netinfo);

    /* ���ڴ�ӡ������Ϣ */
    ctlwizchip(CW_GET_ID,(void*)tmpstr);
    if(netinfo.dhcp == NETINFO_DHCP) 
      printf("\r\n=== %s NET CONF : DHCP ===\r\n",(char*)tmpstr);
    else 
      printf("\r\n=== %s NET CONF : Static ===\r\n",(char*)tmpstr);

    printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",netinfo.mac[0],netinfo.mac[1],netinfo.mac[2],
                    netinfo.mac[3],netinfo.mac[4],netinfo.mac[5]);
    printf("SIP: %d.%d.%d.%d\r\n", netinfo.ip[0],netinfo.ip[1],netinfo.ip[2],netinfo.ip[3]);
    printf("GAR: %d.%d.%d.%d\r\n", netinfo.gw[0],netinfo.gw[1],netinfo.gw[2],netinfo.gw[3]);
    printf("SUB: %d.%d.%d.%d\r\n", netinfo.sn[0],netinfo.sn[1],netinfo.sn[2],netinfo.sn[3]);
    printf("DNS: %d.%d.%d.%d\r\n", netinfo.dns[0],netinfo.dns[1],netinfo.dns[2],netinfo.dns[3]);
    printf("===========================\r\n");
}

/*��SPI�ӿں���ע�ᵽW5500��socket����*/
static void RegisterSPItoW5500(void)
{
  /* 1.ע���ٽ������� */
  reg_wizchip_cris_cbfunc(SPI_CrisEnter, SPI_CrisExit);
  
  /* 2.ע��SPIƬѡ�źź��� */
#if _WIZCHIP_IO_MODE_ == _WIZCHIP_IO_MODE_SPI_VDM_
  reg_wizchip_cs_cbfunc(SPI_CS_Select, SPI_CS_Deselect);
#elif _WIZCHIP_IO_MODE_ == _WIZCHIP_IO_MODE_SPI_FDM_
  reg_wizchip_cs_cbfunc(SPI_CS_Select, SPI_CS_Deselect);// CS must be tried with LOW.
#else
   #if (_WIZCHIP_IO_MODE_ & _WIZCHIP_IO_MODE_SIP_) != _WIZCHIP_IO_MODE_SIP_
      #error "Unknown _WIZCHIP_IO_MODE_"
   #else
      reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
   #endif
#endif
      
  /* 3.ע���д���� */
  reg_wizchip_spi_cbfunc(SPI_ReadByte, SPI_WriteByte);
}

/*��ʼ��W5500����оƬ:ֱ�ӵ��ùٷ��ṩ�ĳ�ʼ����*/
static void InitW5500SocketBuf(void)
{
  /* WIZCHIP SOCKET Buffer initialize */
  if(ctlwizchip(CW_INIT_WIZCHIP,(void*)memsize) == -1){
      printf("WIZCHIP Initialized fail.\r\n");
      while(1);
  }
}

/* PHY��·״̬���*/
static void PhyLinkStatusCheck(void)
{
  uint8_t tmp;
  do{
    if(ctlwizchip(CW_GET_PHYLINK, (void*)&tmp) == -1){
      printf("Unknown PHY Link stauts.\r\n");
    }
  }while(tmp == PHY_LINK_OFF);
}

/*DHCP��ʼ��*/
static void DhcpInitHandler(void)
{
   /* >> DHCP Client 				  */
  /************************************************/
  
  //must be set the default mac before DHCP started.
  setSHAR(gWIZNETINFO.mac);

  DHCP_init(SOCK_DHCP, gDATABUF);//gDATABUF��ȡ���ݵĻ�����
  
  // if you want defiffent action instead defalut ip assign,update, conflict,
  // if cbfunc == 0, act as default.
  //����:�����IP�����µ�IP����ͻ��IP
  reg_dhcp_cbfunc(my_ip_assign, my_ip_assign, my_ip_conflict);
}

/*��̬����IP��ַ*/
void DhcpRunInLoop(void)
{
  static uint8_t my_dhcp_retry = 0;
  char value[16]={0};
  int32_t t=0;
  
  
  switch(DHCP_run())
  {
    case DHCP_IP_ASSIGN://DHCP IP����
    case DHCP_IP_CHANGED://DHCP IP����
        /* If this block empty, act with default_ip_assign & default_ip_update */
        // This example calls my_ip_assign in the two case.
        break;
    
    case DHCP_IP_LEASED://TO DO YOUR NETWORK APPs.
        //����õ�DHCP�����ַʱ����ִ������App
        //yeelink_get("19610","34409",value);//���騰aD����aDT??3����??oyeelink������?��?ID--LED1
        yeelink_get("19657","34484",value);//���騰aD����aDT??3����??oyeelink������?��?ID--LED2
        printf("%s\n\r",value);	//char value[16]={0};
        printf("\n\r");
        for(t=0;t<11;t++){
          delay_ms(1000);
        } 
        break;
    
    case DHCP_FAILED://��̬IP����ʧ��
        /* ===== Example pseudo code =====  */
        // The below code can be replaced your code or omitted.
        // if omitted, retry to process DHCP
        my_dhcp_retry++;
        if(my_dhcp_retry > MY_MAX_DHCP_RETRY)//��̬IP����ʧ�ܴ���ʮ�Σ���ʹ�þ�̬��IP
        {
            printf(">> DHCP %d Failed\r\n", my_dhcp_retry);
            my_dhcp_retry = 0;
            DHCP_stop();// if restart, recall DHCP_init()
            network_init();// apply the default static network and print out netinfo to serial
            DNS_Analyse();//��������
        }
        break;
        
    default:
        break;
  }  
}

/*DNS����*/
void DNS_Analyse(void)
{
    int32_t ret = 0;
  
    /*��ʼ��DNS�������򣬲�ͨ��������غ�����ȡ��machtalk����������ʵIP��ַ*/
    /* DNS client initialization */
    DNS_init(SOCK_DNS, gDATABUF);
    Timer_Start();/*����Timer3*/
    /* DNS procssing */
    if ((ret = DNS_run(gWIZNETINFO.dns, domain_name, domain_ip)) > 0){ // try to 1st DNS
        printf("> 1st DNS Reponsed\r\n");
    }else if(ret == -1){
        printf("> MAX_DOMAIN_NAME is too small. Should be redefined it.\r\n");
        Timer_Stop();/*�ر�Timer3*/
        while(1);
    }else{
        printf("> DNS Failed\r\n");
        Timer_Stop();/*�ر�Timer3*/
        while(1);
    }

    //DNS�����ɹ���IP��ַ�洢��domain_ip�����У������socket��̻��õ��������ֵ��
    if(ret > 0){
        printf("> Translated %s to %d.%d.%d.%d\r\n",domain_name,domain_ip[0],domain_ip[1],domain_ip[2],domain_ip[3]);
    }
    Timer_Stop();
}



/*����W5500����*/
void NetworkInitHandler(void)
{
    RegisterSPItoW5500();/*��SPI�ӿں���ע�ᵽW5500��socket����*/
    InitW5500SocketBuf();/*��ʼ��W5500����оƬ:ֱ�ӵ��ùٷ��ṩ�ĳ�ʼ����*/
    PhyLinkStatusCheck();/* PHY��·״̬���*/
    DhcpInitHandler();   /*DHCP��ʼ��*/
}

/* ѭ����ȡ��������ֵ��������ȡ���Ľ���ύ���������ڴ�ӡ�������ȻҲ���Ը������ֵ��
 * ����LED�ƣ�ѭ����ȡ���ʱ��������10s�������yeelink���Ƶġ�
 */
uint8_t yeelink_get(const char *device_id,const char *sensors_id,char *value)
{
    int ret;
    char* presult;
    char remote_server[] = "api.yeelink.net";
    char str_tmp[128] = {0};

    // ���󻺳�������Ӧ������
    static char http_request[DATA_BUF_SIZE] = {0};	//����Ϊ��̬��������ֹ��ջ���
    static char http_response[DATA_BUF_SIZE] = {0};	//����Ϊ��̬��������ֹ��ջ���
    sprintf(str_tmp,"/v1.0/device/%s/sensor/%s/datapoints",device_id,sensors_id);

    // ȷ�� HTTP�����ײ�
    // ����POST /v1.0/device/98d19569e0474e9abf6f075b8b5876b9/1/1/datapoints/add HTTP/1.1\r\n
    sprintf( http_request , "GET %s HTTP/1.1\r\n",str_tmp);


    // �������� ���� Host: api.machtalk.net\r\n
    sprintf( str_tmp , "Host:%s\r\n" , remote_server);
    strcat( http_request , str_tmp);

    // �������� ���� APIKey: d8a605daa5f4c8a3ad086151686dce64
    //sprintf( str_tmp , "U-ApiKey:%s\r\n" , "d8a605daa5f4c8a3ad086151686dce64");//��Ҫ�滻Ϊ�Լ���APIKey
    sprintf( str_tmp , "U-ApiKey:%s\r\n" , "e5da11d13d2e5f540ef1a99b3506e081");//APIKey--qinfei
    strcat( http_request , str_tmp);
    //
    strcat( http_request , "Accept: */*\r\n");
    // ���ӱ������ʽ Content-Type:application/x-www-form-urlencoded\r\n
    strcat( http_request , "Content-Type: application/x-www-form-urlencoded\r\n");
    strcat( http_request , "Connection: close\r\n");
    // HTTP�ײ���HTTP���� �ָ�����
    strcat( http_request , "\r\n");

    //������ͨ��TCP���ͳ�ȥ
    //�½�һ��Socket���󶨱��ض˿�5000
    ret = socket(SOCK_TCPS,Sn_MR_TCP,5000,0x00);
    if(ret != SOCK_TCPS){
        printf("%d:Socket Error\r\n",SOCK_TCPS);
        while(1);
    }
    //����TCP������
    ret = connect(SOCK_TCPS,domain_ip,80);
    if(ret != SOCK_OK){
        printf("%d:Socket Connect Error\r\n",SOCK_TCPS);
        while(1);
    }	
    //��������
    ret = send(SOCK_TCPS,(unsigned char *)http_request,strlen(http_request));
    if(ret != strlen(http_request)){
        printf("%d:Socket Send Error\r\n",SOCK_TCPS);
        while(1);
    }

    // �����Ӧ
    ret = recv(SOCK_TCPS,(unsigned char *)http_response,DATA_BUF_SIZE);
    if(ret <= 0){
        printf("%d:Socket Get Error\r\n",SOCK_TCPS);
        while(1);
    }
    http_response[ret] = '\0';
    //�ж��Ƿ��յ�HTTP OK
    presult = strstr( (const char *)http_response , (const char *)"200 OK\r\n");
    if( presult != NULL ){
        static char strTmp[DATA_BUF_SIZE]={0};//����Ϊ��̬��������ֹ��ջ���
        sscanf(http_response,"%*[^{]{%[^}]",strTmp);
        //��ȡ������Ϣ
        char timestamp[64]={0};
        char timestampTmp[64]={0};
        char valueTmp[64]={0};
        sscanf(strTmp,"%[^,],%[^,]",timestampTmp,valueTmp);
        strncpy(timestamp,strstr(timestampTmp,":")+2,strlen(strstr(timestampTmp,":"))-3);
        strncpy(value,strstr(valueTmp,":")+1,strlen(strstr(valueTmp,":"))-1);
    }else{
        printf("Http Response Error\r\n");
        printf("%s",http_response);
    }
    close(SOCK_TCPS);
    return 0;
}
/*********************************END OF FILE**********************************/

