/* ---------------------------------------------------------------------
 $RCSfile: CAcapiControl.h,v $
 Project: Application Platform DLL
 Subproject:
 Date of Creation: 23.11.2004
 Main Author: Cymation Technology GmbH 
 Copyright: (c) 2004 Cymation Technology GmbH.
            All Rights Reserved.

            N O T I C E
            THIS MATERIAL IS CONSIDERED A TRADE SECRET BY Cymation Technology GmbH.
            UNAUTHORIZED ACCESS, USE, REPRODUCTION OR
            DISTRIBUTION IS PROHIBITED.
 $Revision: 1.25 $
 ---------------------------------------------------------------------
 Description:
  
 ---------------------------------------------------------------------
 Class list:
 ---------------------------------------------------------------------
 $Log: CAcapiControl.h,v $
 Revision 1.25  2007/06/26 07:46:48  rothkampu
 Version 1.2.17

 Revision 1.24  2006/10/31 08:07:06  rothkampu
 *** empty log message ***

 Revision 1.23  2006/10/25 10:33:37  rothkampu
 Send_ONCE_SDS für Windows herausgenommen

 Revision 1.22  2006/08/30 15:13:41  rothkampu
 *** empty log message ***

 Revision 1.21  2006/08/25 10:04:17  guentherf
 ACCEPTED GROUP

 Revision 1.20  2006/08/23 12:29:12  rothkampu
 Include und CallWait implementiert

 Revision 1.19  2006/06/29 13:16:29  guentherf
 change for MPS

 Revision 1.18  2006/05/17 09:07:15  guentherf
 group attach test hinzugefügt

 Revision 1.17  2006/04/25 07:00:09  guentherf
 neue Testfälle

 Revision 1.16  2006/03/29 07:52:46  guentherf
 *** empty log message ***

 Revision 1.15  2006/02/02 16:17:29  guentherf
 *** empty log message ***

 Revision 1.14  2006/02/01 16:47:51  rothkampu
 *** empty log message ***

 Revision 1.13  2006/02/01 16:27:28  rothkampu
 *** empty log message ***

 Revision 1.12  2006/02/01 15:59:28  rothkampu
 *** empty log message ***

 Revision 1.11  2005/12/02 12:00:10  guentherf
 *** empty log message ***

 Revision 1.10  2005/03/21 16:17:30  rothkampu
 *** empty log message ***

 Revision 1.9  2005/02/17 11:58:28  rothkampu
 *** empty log message ***

 Revision 1.8  2005/01/17 16:36:46  rothkampu
 *** empty log message ***

 Revision 1.7  2005/01/14 16:26:44  rothkampu
 *** empty log message ***

 Revision 1.6  2004/12/21 12:32:54  rothkampu
 *** empty log message ***

 Revision 1.5  2004/12/17 16:19:14  rothkampu
 *** empty log message ***

 Revision 1.4  2004/12/17 10:51:32  rothkampu
 *** empty log message ***

 Revision 1.3  2004/11/25 17:38:53  rothkampu
 *** empty log message ***

 Revision 1.2  2004/11/25 08:52:24  rothkampu
 *** empty log message ***

 Revision 1.1  2004/11/25 08:43:25  rothkampu
 Zwischenspeicherung


 ---------------------------------------------------------------------
*/
#ifndef __CACAPICONTROL_H
#define __CACAPICONTROL_H

/* ---------------------------------------------------------------------
   Include-Files
   ---------------------------------------------------------------------
*/

#ifdef WIN32
#include <windows.h>
#endif
#include <string>
#include <list>
#include "CreatePDU.h"
#include "CDataLink.h"
#include "IDataLinkListener.h"
#include "IAcapiDllListener.h"
#include "CServices.h"
#include "BaseTypes.h"
#include "asn/asn-incl.h"
//#include "asn/acapi.h"
/* ---------------------------------------------------------------------
   NAMESPACE
   ---------------------------------------------------------------------
*/
using namespace SNACC;
using namespace std;

/* ---------------------------------------------------------------------
   KLASSENDEFINITIONEN
   ---------------------------------------------------------------------
*/
class CAcapiControl  : public IDataLinkListener
{
  public:
    CAcapiControl();
    ~CAcapiControl();
    void Init(char *ip_addr, char *licence_key, char logger_state);
    void SetHostname (const std::string& host_name, unsigned short portNumber, char logger_state);
    bool Start();
    bool Stop();

    long connectService      (char *app_name, char *password, char *ssi, long l_service);
    long connectService      (char *app_name, char *password, char *ssi, long l_service, short trunk_number, short channel_number, short subchannel_number);
    long connectRtpService	 (char *app_name, char *password, char *ssi, long l_service, long rtpPayloadType);

    bool closeService        (short app_handle);
    bool sendSdsData         (short app_handle, short msg_number, char *target_ssi, short sds_length, char *sds_data, bool sds_ack, short priority);
    bool sendSdsStatus       (short app_handle, short msg_number, char *target_ssi, short sds_status, short priority);
    bool sendExSdsData       (short app_handle, short msg_number, long gateway_id, char *dialing_string, short sds_length, char *sds_data, bool sds_ack, short priority);
    bool sendExSdsStatus     (short app_handle, short msg_number, long gateway_id, char *dialing_string, short sds_status, short priority);
    bool dynGroupAdd         (short app_handle, char* group_ssi, char* affected_ssi, short class_of_usage, char* mnemonic_group_name);
    bool dynGroupDel         (short app_handle, char* group_ssi, char* affected_ssi);
    bool dynGroupInterrogate (short app_handle, char* group_ssi);
    bool OOCIAssign          (short app_handle, char *external_subs, long gateway_ssi, long ssi, short cli_type);
    bool OOCIDeassign        (short app_handle, char *external_subs, long gateway_ssi, short typeOfDeassignment);
    bool OOCIInterrogate     (short app_handle, long gateway_ssi, long startSSI, long endSSI);
    bool OOCICancelGateway   (short app_handle, long gateway_ssi);
    bool CallForwardActivate (short app_handle, char *forwardedPartySSI, char *forwardToParty, short forwardingType, short forwardingService);
    bool CallForwardDeactivate(short app_handle, char *forwardingPartySSI, short forwardingType);
	bool CallForwardCancelAll(short app_handle);
	bool CallForwardInterrogate (short app_handle, long start_ssi, long endSSI);
    bool GroupReport		 (short app_handle, char *groupSsi, long requestHandle, long areaSelection);

    bool monFleetMonitoringReq (short app_handle, char* fleet_number, short mon_service);
    bool monMonitoringReq    (short app_handle,char *begin_ssi, char *end_ssi, short mon_service);
	bool monOrderReq         (short app_handle, int regMonitoring, int sdsMonitoring, int ccMonitoring, int type, long ssi, long endSSI, char *dialing_string);
    bool monOrderClose       (short app_handle, int mon_handle);
    bool monClose            (short app_handle, short mon_handle);
    bool monClose            (short app_handle, short mon_handle, char *begin_ssi, char *end_ssi);
    bool monInterceptReq     (short app_handle, short mon_handle, char *call_party_ssi, long call_id, short trunk_number1, short channel_number1, short sub_channel1, short trunk_number2, short channel_number2, short sub_channel2);
    bool monRtpInterceptReq  (short app_handle, short mon_handle, char *call_party_ssi, long call_id, char *rtpIpAddress1, long rtpPort1, long rtpPayloadType1, char *rtpIpAddress2, long rtpPort2, long rtpPayloadType2);
    bool monInterceptDisconnect(short app_handle, short mon_handle, char *disconnected_party_ssi, long call_id);
	bool monUplinkTXDemand   (short app_handle, short mon_handle, char *demandPartySSI, long call_id, long txPriority, bool noNfLoopBack);
	bool monUplinkTXCeased   (short app_handle, short mon_handle, char *callParty, long call_id);
	bool monForcedCallEnd    (short app_handle, short mon_handle, long call_id, long result);

    bool callSubscriber      (short app_handle, char *target_ssi, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, bool duplex_request, short comm_typebool, bool ambience_listening, short encyption);
    bool callExSubscriber    (short app_handle, long gateway_id, char *dialing_string, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, bool duplex_request, short comm_typebool, bool ambience_listening, short encyption);
    bool callRtpSubscriber   (short app_handle, char  *target_ssi, char *rtpIpAddress, long  rtpPort, long  rtpPayloadType, short hook_signalling, short priority_type, short priority, bool  duplex_request, short comm_type,bool  ambience_listening, short encryption);
    bool callDemand          (short app_handle, long  call_id, short priority, bool noNfLoopBack, bool encryption);
    bool callCeased          (short app_handle, long  call_id);
    bool callAccept          (short app_handle, long  call_id, short trunk_number, short channel_number, short sub_channel, bool ambience_listening, bool encryption);
    bool callAcceptGroup     (short app_handle, long  call_id, char *group_ssi, short trunk_number, short channel_number, short sub_channel, bool ambience_listening, bool encryption);
	bool callRtpAccept       (short app_handle, long call_id, char *group_ssi, char *rtpIpAddress, long  rtpPort, long  rtpPayloadType, bool ambience_listening, bool encryption);

	bool callDisconnect      (short app_handle, long  call_id, short disconnect_cause);
    bool callWait            (short app_handle, char *call_ssi, char *other_ssi, long  call_id);
    bool callContinue        (short app_handle, char *call_ssi, char *other_ssi, long  call_id);
    bool callWaitAck         (short app_handle, char *call_ssi, char *other_ssi, long  call_id, short result);
    bool callContinueAck     (short app_handle, char *call_ssi, char *other_ssi, long  call_id, short result);
    bool callInclude         (short app_handle, char *include_ssi, char *calling_party, short hook_signalling, long call_id);
    bool callCancelInclude   (short app_handle, char *include_ssi, char *calling_party, long call_id);
    bool callForcedEnd       (short app_handle, short call_id, short disconnectCause);


    
    bool groupAttach    (short app_handle, char* call_party_ssi, char* group_ssi);
    bool groupDetach    (short app_handle, char* call_party_ssi, char* group_ssi);



    bool sendACAPI_Message_Ack(short app_handle);

    void dataReceived (ACAPI_PDU* acapi_pdu);

    void connectionClosed();

    void registerAcapiDllListener(IAcapiDllListener* pInterface);

    void putLoggerQueue(std::string msg);

    void putMsgQueue(std::string msg);

    long getSentQueueSize();

  private:
    bool fkt_aCAPI_Authorization_Req   (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_ACAPI_OpenService_Ack     (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_ACAPI_CloseService        (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_ACAPI_CloseServiceAck     (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_sDS_Data                  (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_sDS_DataAck               (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_sS_DynGroupAdd_Ack        (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_sS_DynGroupAssign         (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_sS_DynGroupDel_Ack        (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_sS_DynGroupDeassign       (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_sS_DynGroupInterrogate_Ack(ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_sS_ObjectIdentityAssign_Ack (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_sS_ObjectIdentityDeassign_Ack (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_sS_ObjectIdentityInterrogate_Ack (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_sS_ObjectIdentityCancelGateway_Ack (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
	bool fkt_sS_CallForwardActivate_Ack (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
	bool fkt_sS_CallForwardDeactivate_Ack (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
	bool fkt_sS_CallForwardCancel_Ack (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
	bool fkt_sS_CallForwardInterrogate_Ack  (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_name_sS_GroupReport_Ack (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
	bool fkt_name_sS_GroupReport_Info (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);


    bool fkt_mON_FleetMonitoring_Ack   (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_mON_Monitoring_Ack        (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
	bool fkt_mON__Order_Ack            (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
	bool fkt_mON__OrderClose           (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
	bool fkt_mON__OrderClose_Ack	   (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);

    bool fkt_mON_Close                 (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_mON_CloseAck              (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_mON_LocationUpdate        (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_mON_LocationDetach        (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_mON_SDS_Data              (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_mON_SDS_Ack               (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_mON_CC_Information        (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service); 
    bool fkt_mON_TxDemand              (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_mON_TxCeased              (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_mON_TxGrant               (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_mON_Disconnect            (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_mON_Intercept_Ack         (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_mON_Intercept_Connect     (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service); 
    bool fkt_mON_Intercept_Disconnect  (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_mON_Intercept_Disconnect_Ack (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_mON_DTMF_Sign             (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
	bool fkt_mON_ForcedCallEnd_Ack     (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);

    bool fkt_cC_Setup_Ack      (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_cC_Setup_Ind      (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_cC_Information    (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_cC_Connect_Ack    (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_cC_Connect_Ind    (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_cC_Disconnect     (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_cC_DisconnectAck  (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_cC_TxCeased       (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_cC_TxGrant        (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_sS_GroupAttach_Ack(ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_sS_GroupDetach_Ack(ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);

    
    bool fkt_cC_TxWait_Ind     (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_cC_TxWait_Ack     (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_cC_TxContinue_Ind (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
    bool fkt_cC_TxContinue_Ack (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
	bool fkt_cC_ForcedCallEnd_Ack (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);
	bool fkt_cC_CancelInclude_Ack (ACAPI_PDU* acapi_pdu, CServices::AcapiService* service);

    /*
    Klasse die die connection zum TAP hin verwaltet, inkl. des Senden und Empfangen von 
    Messages
    **/
    CDataLink* _data_link;

    /*
    Klasse zum erstellen von PDU´s
    **/
    CreatePDU* _create_pdu;

    /*
    Klasse zum Aufheben der Services
    **/
    CServices* _services;

    /*
    Listener für Klasse CAcapiDll
    **/
    IAcapiDllListener *_own_acapi_dll_listener;

};

#endif


