/* ---------------------------------------------------------------------
 $RCSfile: CAcapiDll.h,v $
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
 $Revision: 1.21 $
 ---------------------------------------------------------------------
 Description:
  
 ---------------------------------------------------------------------
 Class list:
 ---------------------------------------------------------------------
 $Log: CAcapiDll.h,v $
 Revision 1.21  2006/10/31 08:07:06  rothkampu
 *** empty log message ***

 Revision 1.20  2006/10/25 10:33:37  rothkampu
 Send_ONCE_SDS für Windows herausgenommen

 Revision 1.19  2006/09/27 09:16:03  rothkampu
 Version 1.2.8

 Revision 1.18  2006/08/30 15:13:41  rothkampu
 *** empty log message ***

 Revision 1.17  2006/08/25 10:04:17  guentherf
 ACCEPTED GROUP

 Revision 1.16  2006/08/23 12:29:12  rothkampu
 Include und CallWait implementiert

 Revision 1.15  2006/06/29 14:39:05  rothkampu
 *** empty log message ***

 Revision 1.14  2006/06/29 13:43:13  guentherf
 with version number

 Revision 1.13  2006/06/29 13:16:29  guentherf
 change for MPS

 Revision 1.12  2006/05/17 09:07:15  guentherf
 group attach test hinzugefügt

 Revision 1.11  2006/04/25 07:00:09  guentherf
 neue Testfälle

 Revision 1.10  2006/03/29 07:52:46  guentherf
 *** empty log message ***

 Revision 1.9  2006/02/02 16:17:29  guentherf
 *** empty log message ***

 Revision 1.8  2005/12/02 12:00:10  guentherf
 *** empty log message ***

 Revision 1.7  2005/02/17 11:58:28  rothkampu
 *** empty log message ***

 Revision 1.6  2005/02/11 15:29:29  rothkampu
 Release 1.0.1

 Revision 1.5  2004/12/21 12:32:54  rothkampu
 *** empty log message ***

 Revision 1.4  2004/12/17 16:19:14  rothkampu
 *** empty log message ***

 Revision 1.3  2004/12/17 10:51:32  rothkampu
 *** empty log message ***

 Revision 1.2  2004/11/25 17:38:53  rothkampu
 *** empty log message ***

 Revision 1.1  2004/11/25 08:43:25  rothkampu
 Zwischenspeicherung


 ---------------------------------------------------------------------
*/
#ifndef __CACAPI_DLL_H
#define __CACAPI_DLL_H

/* ---------------------------------------------------------------------
   Include-Files
   ---------------------------------------------------------------------
*/
#ifdef WIN32
  #include <windows.h>
#else
  #include <pthread.h>
#endif
#include <string>
#include <list>
#include <queue>
#if ACAPI_DLL_EXPORTS
  #include "BaseTypes.h"
  #include "IAcapiDllListener.h"
  #include "CAcapiControl.h"
#endif
#include "CAcapiDllInterface.h"


/* ---------------------------------------------------------------------
   NAMESPACE
   ---------------------------------------------------------------------
*/
using namespace std;

/* ---------------------------------------------------------------------
   External Deklarationen
   ---------------------------------------------------------------------
*/
#if ACAPI_DLL_EXPORTS
  #define ACAPI_DLL_API __declspec(dllexport)
#else
#define ACAPI_DLL_API __declspec(dllimport)
#endif

extern "C" ACAPI_DLL_API char *getVersionNumber          ();
extern "C" ACAPI_DLL_API long getSentQueueSize           ();
extern "C" ACAPI_DLL_API long getRecQueueSize            ();
extern "C" ACAPI_DLL_API bool acapiDllStart              (char *licence_key, char *host_name, char logger_state);
extern "C" ACAPI_DLL_API bool acapiDllStartWithInterface (char *licence_key, char *host_name, char logger_state, CAcapiDllInterface *rec_interface);
extern "C" ACAPI_DLL_API bool acapiDllStop               ();
extern "C" ACAPI_DLL_API long connectSdsService          (char *app_name, char *password, char *ssi);
extern "C" ACAPI_DLL_API long connectCCService           (char *app_name, char *password, char *ssi, char *trunk_number);
extern "C" ACAPI_DLL_API long connectRtpCCService        (char *app_name, char *password, char *ssi, long rtpPayloadType);
extern "C" ACAPI_DLL_API long connectMonitoringService   (char *app_name, char *password, char *ssi, short trunk_number, short channel_number, short subchannel_number);
extern "C" ACAPI_DLL_API long connectRtpMonitoringService(char *app_name, char *password, char *ssi, long rtpPayloadType);
extern "C" ACAPI_DLL_API long connectSupplementaryService(char *app_name, char *password, char *ssi);
extern "C" ACAPI_DLL_API bool closeService               (short app_handle);
extern "C" ACAPI_DLL_API bool sendSdsData                (short app_handle, short msg_number, char *target_ssi, short sds_length, char *sds_data);
extern "C" ACAPI_DLL_API bool sendSdsDataWithAck         (short app_handle, short msg_number, char *target_ssi, short sds_length, char *sds_data, short sds_ack);
extern "C" ACAPI_DLL_API bool sendSdsDataWithPriority    (short app_handle, short msg_number, char *target_ssi, short sds_length, char *sds_data, short sds_ack, short priority);
extern "C" ACAPI_DLL_API bool sendExSdsData              (short app_handle, short msg_number, long gateway_id, char *dialing_string, short sds_length, char *sds_data, short sds_ack, short priority);
extern "C" ACAPI_DLL_API bool sendSdsStatus              (short app_handle, short msg_number, char *target_ssi, short sds_status);
extern "C" ACAPI_DLL_API bool sendSdsStatusWithPriority  (short app_handle, short msg_number, char *target_ssi, short sds_status, short priority);
extern "C" ACAPI_DLL_API bool sendExSdsStatus            (short app_handle, short msg_number, long gateway_id, char *dialing_string, short sds_status, short priority);
extern "C" ACAPI_DLL_API bool dynGroupAdd                (short app_handle, char *group_ssi, char *affected_ssi, short class_of_usage);
extern "C" ACAPI_DLL_API bool dynGroupAddWithName        (short app_handle, char *group_ssi, char *affected_ssi, short class_of_usage, char* mnemonic_group_name);
extern "C" ACAPI_DLL_API bool dynGroupDel                (short app_handle, char *group_ssi, char *affected_ssi);
extern "C" ACAPI_DLL_API bool dynGroupInterrogate        (short app_handle, char *group_ssi);
extern "C" ACAPI_DLL_API bool OOCIAssign                 (short app_handle, char *external_subs, long gateway_ssi, long ssi, short cli_type);
extern "C" ACAPI_DLL_API bool OOCIDeassign               (short app_handle, char *external_subs, long gateway_ssi, short typeOfDeassignment);
extern "C" ACAPI_DLL_API bool OOCIInterrogate            (short app_handle, long gateway_ssi, long startSSI, long endSSI);
extern "C" ACAPI_DLL_API bool OOCICancelGateway          (short app_handle, long gateway_ssi); 
extern "C" ACAPI_DLL_API bool CallForwardActivate	 	 (short app_handle, char *forwardedPartySSI, char * forwardToParty, short forwardingType, short forwardingService);
extern "C" ACAPI_DLL_API bool CallForwardDeactivate		 (short app_handle, char *forwardingPartySSI, short forwardingType);
extern "C" ACAPI_DLL_API bool CallForwardCancelAll		 (short app_handle);
extern "C" ACAPI_DLL_API bool CallForwardInterrogate	 (short app_handle, long start_ssi, long endSSI); 
extern "C" ACAPI_DLL_API bool GroupReport				 (short app_handle, char *groupSsi, long requestHandle, long areaSelection);

extern "C" ACAPI_DLL_API bool monFleetMonitoringReq      (short app_handle, char *fleet_number, short mon_service);
extern "C" ACAPI_DLL_API bool monMonitoringReq           (short app_handle, char *begin_ssi, char *end_ssi, short mon_service);
extern "C" ACAPI_DLL_API bool monOrderReqSingle          (short app_handle, int regMonitoring, int sdsMonitoring, int ccMonitoring, long ssi, char *dialing_string);
extern "C" ACAPI_DLL_API bool monOrderReqRange           (short app_handle, int regMonitoring, int sdsMonitoring, int ccMonitoring, long ssi, long end_ssi);
extern "C" ACAPI_DLL_API bool monOrderReqFleet           (short app_handle, int regMonitoring, int sdsMonitoring, int fleet_number);
extern "C" ACAPI_DLL_API bool monOrderReqGateway         (short app_handle, int regMonitoring, int sdsMonitoring, int ccMonitoring, int gateway_ssi);
extern "C" ACAPI_DLL_API bool monOrderClose				 (short app_handle, int mon_handel);

extern "C" ACAPI_DLL_API bool monClose                   (short app_handle, short mon_handle, char *begin_ssi, char *end_ssi);
extern "C" ACAPI_DLL_API bool monInterceptReq            (short app_handle, short mon_handle, char *call_party_ssi, long call_id, short trunk_number1, short channel_number1, short sub_channel1, short trunk_number2, short channel_number2, short sub_channel2);
extern "C" ACAPI_DLL_API bool monRtpInterceptReq         (short app_handle, short mon_handle, char *call_party_ssi, long call_id, char *rtpIpAddress1, long rtpPort1, long rtpPayloadType1, char *rtpIpAddress2, long rtpPort2, long rtpPayloadType2);
extern "C" ACAPI_DLL_API bool monInterceptDisconnect     (short app_handle, short mon_handle, char *disconnected_party_ssi, long call_id);
extern "C" ACAPI_DLL_API bool monUplinkTXDemand          (short app_handle, short mon_handle, char *demandPartySSI, long call_id, long txPriority, bool noNfLoopBack);
extern "C" ACAPI_DLL_API bool monUplinkTXCeased		     (short app_handle, short mon_handle, char *callParty, long call_id);
extern "C" ACAPI_DLL_API bool monForcedCallEnd			 (short app_handle, short mon_handle, long call_id, long result);

extern "C" ACAPI_DLL_API bool __cdecl callDuplex         (short app_handle, char *target_ssi, short trunk_number, short channel_number, short hook_signalling, short priority_type, short priority);
extern "C" ACAPI_DLL_API bool __cdecl callDuplex_        (short app_handle, char *target_ssi, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, bool encryption);
extern "C" ACAPI_DLL_API bool __cdecl callRtpDuplex      (short app_handle, char *target_ssi, char *rtpIpAddress, long rtpPort, long rtpPayloadType, short hook_signalling, short priority_type, short priority, bool encryption);
extern "C" ACAPI_DLL_API bool __cdecl callExDuplex       (short app_handle, long gateway_id, char *dialing_string, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, bool encryption);

extern "C" ACAPI_DLL_API bool __cdecl callSimplex        (short app_handle, char *target_ssi, short trunk_number, short channel_number, short hook_signalling, short priority_type, short priority);
extern "C" ACAPI_DLL_API bool __cdecl callSimplex_       (short app_handle, char *target_ssi, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, bool encryption);
extern "C" ACAPI_DLL_API bool __cdecl callRtpSimplex     (short app_handle, char *target_ssi, char *rtpIpAddress, long rtpPort, long rtpPayloadType,  short hook_signalling, short priority_type, short priority, bool encryption);
extern "C" ACAPI_DLL_API bool __cdecl callExSimplex      (short app_handle, long gateway_id, char *dialing_string, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, bool encryption);

extern "C" ACAPI_DLL_API bool __cdecl callGroup          (short app_handle, char *target_ssi, short trunk_number, short channel_number, short hook_signalling, short priority_type, short priority);
extern "C" ACAPI_DLL_API bool __cdecl callGroup_         (short app_handle, char *target_ssi, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, bool encryption);
extern "C" ACAPI_DLL_API bool __cdecl callRtpGroup       (short app_handle, char *target_ssi, char *rtpIpAddress, long rtpPort, long rtpPayloadType, short hook_signalling, short priority_type, short priority, bool encryption);
extern "C" ACAPI_DLL_API bool __cdecl callExGroup        (short app_handle, long gateway_id, char *dialing_string, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, bool encryption);

extern "C" ACAPI_DLL_API bool __cdecl callBroadcast      (short app_handle, char *target_ssi, short trunk_number, short channel_number, short hook_signalling, short priority_type, short priority);
extern "C" ACAPI_DLL_API bool __cdecl callBroadcast_     (short app_handle, char *target_ssi, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, bool encryption);
extern "C" ACAPI_DLL_API bool __cdecl callRtpBroadcast   (short app_handle, char *target_ssi, char *rtpIpAddress, long rtpPort, long rtpPayloadType, short hook_signalling, short priority_type, short priority, bool encryption);
extern "C" ACAPI_DLL_API bool __cdecl callExBroadcast    (short app_handle, long gateway_id, char *dialing_string, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, bool encryption);

extern "C" ACAPI_DLL_API bool __cdecl callAmbienceListening (short app_handle, char *target_ssi, short trunk_number, short channel_number, short hook_signalling, short priority_type, short priority, short com_type);
extern "C" ACAPI_DLL_API bool __cdecl callAmbienceListening_(short app_handle, char *target_ssi, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, short com_type, bool encryption);
extern "C" ACAPI_DLL_API bool __cdecl callRtpAmbienceListening(short app_handle, char *target_ssi, char *rtpIpAddress, long rtpPort, long rtpPayloadType, short hook_signalling, short priority_type, short priority, short com_type, bool encryption);
extern "C" ACAPI_DLL_API bool __cdecl callExAmbienceListening(short app_handle, long gateway_id, char *dialing_string, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, short com_type, bool encryption);

extern "C" ACAPI_DLL_API bool __cdecl callDemand         (short app_handle, long call_id, short priority, bool noNfLoopBack);
extern "C" ACAPI_DLL_API bool __cdecl callDemand_        (short app_handle, long call_id, short priority, bool noNfLoopBack, bool encryption);

extern "C" ACAPI_DLL_API bool __cdecl callCeased         (short app_handle, long call_id);
extern "C" ACAPI_DLL_API bool __cdecl callPTT            (short app_handle, long call_id, short active, short priority);
extern "C" ACAPI_DLL_API bool __cdecl callPTT_           (short app_handle, long call_id, short active, short priority, bool encryption);

extern "C" ACAPI_DLL_API bool __cdecl callAccept         (short app_handle, long call_id, short trunk_number, short channel_number);
extern "C" ACAPI_DLL_API bool __cdecl callAccept_        (short app_handle, long call_id, short trunk_number, short channel_number, short sub_channel, bool encryption);
extern "C" ACAPI_DLL_API bool __cdecl callRtpAccept      (short app_handle, long call_id, char *rtpIpAddress, long rtpPort, long rtpPayloadType, bool encryption);

extern "C" ACAPI_DLL_API bool __cdecl callAcceptAmbienceListening(short app_handle, long call_id, short trunk_number, short channel_number);
extern "C" ACAPI_DLL_API bool __cdecl callAcceptAmbienceListening_(short app_handle, long call_id, short trunk_number, short channel_number, short sub_channel, bool encryption);
extern "C" ACAPI_DLL_API bool __cdecl callRtpAcceptAmbienceListening(short app_handle, long call_id, char *rtpIpAddress, long rtpPort, long rtpPayloadType, bool encryption);

extern "C" ACAPI_DLL_API bool __cdecl callAcceptGroup    (short app_handle, long call_id, char *group_ssi, short trunk_number, short channel_number);
extern "C" ACAPI_DLL_API bool __cdecl callAcceptGroup_   (short app_handle, long call_id, char *group_ssi, short trunk_number, short channel_number, short sub_channel, bool encryption);
extern "C" ACAPI_DLL_API bool __cdecl callRtpAcceptGroup (short app_handle, long call_id, char *group_ssi, char *rtpIpAddress, long rtpPort, long rtpPayloadType, bool encryption);

extern "C" ACAPI_DLL_API bool __cdecl callDisconnect     (short app_handle, long call_id, short disconnect_cause);
extern "C" ACAPI_DLL_API bool __cdecl callWait           (short app_handle, char *call_ssi, char *other_ssi, long  call_id);
extern "C" ACAPI_DLL_API bool __cdecl callContinue       (short app_handle, char *call_ssi, char *other_ssi, long  call_id);
extern "C" ACAPI_DLL_API bool __cdecl callWaitAck        (short app_handle, char *call_ssi, char *other_ssi, long  call_id, short result);
extern "C" ACAPI_DLL_API bool __cdecl callContinueAck    (short app_handle, char *call_ssi, char *other_ssi, long  call_id, short result);
extern "C" ACAPI_DLL_API bool __cdecl callInclude        (short app_handle, char *include_ssi, char *calling_party, short hook_signalling, long call_id);
extern "C" ACAPI_DLL_API bool __cdecl callCancelInclude  (short app_handle, char *include_ssi, char *calling_party, long call_id);
extern "C" ACAPI_DLL_API bool __cdecl callForcedEnd      (short app_handle, short call_id, short disconnectCause);
extern "C" ACAPI_DLL_API bool __cdecl groupAttach    (short app_handle, char *call_party_ssi, char *group_ssi);
extern "C" ACAPI_DLL_API bool __cdecl groupDetach    (short app_handle, char *call_party_ssi, char *group_ssi);

extern "C" ACAPI_DLL_API const int  getMessage           (char *msg);
extern "C" ACAPI_DLL_API const char *getLogger           ();

/* ---------------------------------------------------------------------
   KLASSENDEFINITIONEN
   ---------------------------------------------------------------------
*/
#if ACAPI_DLL_EXPORTS
class CAcapiDll : public IAcapiDllListener
#else
class CAcapiDll 
#endif
{
  public:
    CAcapiDll();
    CAcapiDll(char *ip_addr, char *licence_key, char logger_state, CAcapiDllInterface *rec_interface);
    ~CAcapiDll();
    char *getVersionNumber           ();
    bool acapiDllStart               (char *licence_key, char *host_name, char logger_state);
	bool acapiDllStartWithInterface  (char *licence_key, char *host_name, char logger_state, CAcapiDllInterface *rec_interface);
    bool acapiDllStart               ();
    bool acapiDllStop                ();
    long connectSdsService           (char *app_name, char *password, char *ssi);
    long connectCCService            (char *app_name, char *password, char *ssi, char *trunk_number);
    long connectRtpCCService         (char *app_name, char *password, char *ssi, long rtpPayloadType);
    long connectMonitoringService    (char *app_name, char *password, char *ssi, short trunk_number, short channel_number, short subchannel_number);
    long connectRtpMonitoringService (char *app_name, char *password, char *ssi, long rtpPayloadType);
    long connectSupplementaryService (char *app_name, char *password, char *ssi);
    bool closeService                (short app_handle);
    bool sendSdsData                 (short app_handle, short msg_number, char *target_ssi, short sds_length, char *sds_data, bool sds_ack, short prority);
    bool sendSdsStatus               (short app_handle, short msg_number, char *target_ssi, short sds_status, short prority);
    bool sendExSdsData               (short app_handle, short msg_number, long gateway_id, char *dialing_string, short sds_length, char *sds_data, bool sds_ack, short prority);
    bool sendExSdsStatus             (short app_handle, short msg_number, long gateway_id, char *dialing_string, short sds_status, short prority);
    bool dynGroupAdd                 (short app_handle, char *group_ssi, char *affected_ssi, short class_of_usage);
    bool dynGroupAddWithName         (short app_handle, char* group_ssi, char* affected_ssi, short class_of_usage, char* mnemonic_group_name);
    bool dynGroupDel                 (short app_handle, char *group_ssi, char *affected_ssi);
    bool dynGroupInterrogate         (short app_handle, char *group_ssi);
    bool OOCIAssign                  (short app_handle, char *external_subs, long gateway_ssi, long ssi, short cli_type);
    bool OOCIDeassign                (short app_handle, char *external_subs, long gateway_ssi, short typeOfDeassignment);
    bool OOCIInterrogate             (short app_handle, long gateway_ssi, long startSSI, long endSSI);
	bool OOCICancelGateway           (short app_handle, long gateway_ssi); 
	bool CallForwardActivate	 	 (short app_handle, char *forwardedPartySSI, char * forwardToParty, short forwardingType, short forwardingService);
	bool CallForwardDeactivate		 (short app_handle, char *forwardingPartySSI, short forwardingType);
	bool CallForwardCancelAll		 (short app_handle);
	bool CallForwardInterrogate		 (short app_handle, long start_ssi, long endSSI); 
	bool GroupReport				 (short app_handle, char *groupSsi, long requestHandle, long areaSelection);

	bool monMonitoringReq            (short app_handle, char *begin_ssi,  char *end_ssi, short mon_service);
    bool monFleetMonitoringReq       (short app_handle, char *fleet_number, short mon_service);
	bool monOrderReqSingle           (short app_handle, int regMonitoring, int sdsMonitoring, int ccMonitoring, long ssi, char *dialing_string);
    bool monOrderReqRange            (short app_handle, int regMonitoring, int sdsMonitoring, int ccMonitoring, long ssi, long end_ssi);
    bool monOrderReqFleet            (short app_handle, int regMonitoring, int sdsMonitoring, int fleet_number);
    bool monOrderReqGateway          (short app_handle, int regMonitoring, int sdsMonitoring, int ccMonitoring, int gateway_ssi);
    bool monOrderClose				 (short app_handle, int mon_handel);
	bool monClose                    (short app_handle, short mon_handle);
    bool monClose                    (short app_handle, short mon_handle, char *begin_ssi, char *end_ssi);
    bool monInterceptReq             (short app_handle, short mon_handle, char *call_party_ssi, long call_id, short trunk_number1, short channel_number1, short sub_channel1, short trunk_number2, short channel_number2, short sub_channel2);
    bool monRtpInterceptReq          (short app_handle, short mon_handle, char *call_party_ssi, long call_id, char *rtpIpAddress1, long rtpPort1, long rtpPayloadType1, char *rtpIpAddress2, long rtpPort2, long rtpPayloadType2);
    bool monInterceptDisconnect      (short app_handle, short mon_handle, char *disconnected_party_ssi, long call_id);
	bool monUplinkTXDemand           (short app_handle, short mon_handle, char *demandPartySSI, long call_id, long txPriority, bool noNfLoopBack);
	bool monUplinkTXCeased		     (short app_handle, short mon_handle, char *callParty, long call_id);
	bool monForcedCallEnd			 (short app_handle, short mon_handle, long call_id, long result);

    bool callDuplex        (short app_handle, char *target_ssi, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, bool encryption);
    bool callSimplex       (short app_handle, char *target_ssi, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, bool encryption);
    bool callGroup         (short app_handle, char *target_ssi, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, bool encryption);
    bool callBroadcast     (short app_handle, char *target_ssi, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, bool encryption);
    bool callAmbienceListening (short app_handle, char *target_ssi, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, short com_type, bool encryption);

	bool callRtpDuplex     (short app_handle, char *target_ssi, char *rtpIpAddress, long rtpPort, long rtpPayloadType, short hook_signalling, short priority_type, short priority, bool encryption);
    bool callRtpSimplex    (short app_handle, char *target_ssi, char *rtpIpAddress, long rtpPort, long rtpPayloadType, short hook_signalling, short priority_type, short priority, bool encryption);
    bool callRtpGroup      (short app_handle, char *target_ssi, char *rtpIpAddress, long rtpPort, long rtpPayloadType, short hook_signalling, short priority_type, short priority, bool encryption);
    bool callRtpBroadcast  (short app_handle, char *target_ssi, char *rtpIpAddress, long rtpPort, long rtpPayloadType, short hook_signalling, short priority_type, short priority, bool encryption);
    bool callRtpAmbienceListening (short app_handle, char *target_ssi, char *rtpIpAddress, long rtpPort, long rtpPayloadType, short hook_signalling, short priority_type, short priority, short com_type, bool encryption);

	bool callExDuplex      (short app_handle, long gateway_id, char *dialing_string, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, bool encryption);
    bool callExSimplex     (short app_handle, long gateway_id, char *dialing_string, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, bool encryption);
    bool callExGroup       (short app_handle, long gateway_id, char *dialing_string, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, bool encryption);
    bool callExBroadcast   (short app_handle, long gateway_id, char *dialing_string, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, bool encryption);
    bool callExAmbienceListening (short app_handle, long gateway_id, char *dialing_string, short trunk_number, short channel_number, short sub_channel, short hook_signalling, short priority_type, short priority, short com_type, bool encryption);

    bool callDemand        (short app_handle, long call_id, short priority, bool noNfLoopBack, bool encryption );
    bool callCeased        (short app_handle, long call_id);
    bool callPTT           (short app_handle, long call_id, short active, short priority , bool encryption);
    bool callAccept        (short app_handle, long call_id, short trunk_number, short channel_number, short sub_channel, bool encryption);
    bool callAcceptAmbienceListening(short app_handle, long call_id, short trunk_number, short channel_number, short sub_channel, bool encryption);
    bool callAcceptGroup   (short app_handle, long call_id, char *group_ssi, short trunk_number, short channel_number, short sub_channel, bool encryption);

	bool callRtpAccept        (short app_handle, long call_id, char *rtpIpAddress, long rtpPort, long rtpPayloadType, bool encryption);
    bool callRtpAcceptAmbienceListening(short app_handle, long call_id, char *rtpIpAddress, long rtpPort, long rtpPayloadType, bool encryption);
    bool callRtpAcceptGroup   (short app_handle, long call_id, char *group_ssi, char *rtpIpAddress, long rtpPort, long rtpPayloadType, bool encryption);

	bool callDisconnect    (short app_handle, long call_id, short disconnect_cause);
    bool callWait          (short app_handle, char *call_ssi, char *other_ssi, long  call_id);
    bool callContinue      (short app_handle, char *call_ssi, char *other_ssi, long  call_id);
    bool callWaitAck       (short app_handle, char *call_ssi, char *other_ssi, long  call_id, short result);
    bool callContinueAck   (short app_handle, char *call_ssi, char *other_ssi, long  call_id, short result);
    bool callInclude       (short app_handle, char *include_ssi, char *calling_party, short hook_signalling, long call_id);
    bool callCancelInclude (short app_handle, char *include_ssi, char *calling_party, long call_id);
    bool callForcedEnd     (short app_handle, short call_id, short disconnectCause);

    bool groupAttach    (short app_handle, char *call_party_ssi, char *group_ssi);
    bool groupDetach    (short app_handle, char *call_party_ssi, char *group_ssi);


    string getMsgQueue();
    void   putMsgQueue(string msg);

    string getLoggerQueue();
    void putLoggerQueue(std::string msg);

    long getSentQueueSize();
    long getRecQueueSize();

  private:
#if ACAPI_DLL_EXPORTS
    /*
    Klasse CAcapiControl
    **/
    CAcapiControl* _acapi_control;

    /*
    Queue für ausgehende Messages 
    **/
    queue <string> _out_msg_queue;
    #ifdef WIN32
      CRITICAL_SECTION _out_msg_queue_CriticalSection;
    #else
      pthread_mutex_t _out_msg_queue_CriticalSection;
    #endif


    /*
    Queue für eingehende Messages
    **/
    queue <string> _in_msg_queue;
    #ifdef WIN32
      CRITICAL_SECTION _in_msg_queue_CriticalSection;
    #else
      pthread_mutex_t _in_msg_queue_CriticalSection;
    #endif

    /*
    Queue für Logger-Nachrichten
    **/
    queue <string> _logger_queue;
    #ifdef WIN32
      CRITICAL_SECTION _logger_queue_CriticalSection;
    #else
      pthread_mutex_t _logger_queue_CriticalSection;
    #endif

    CAcapiDllInterface *m_rec_interface;
#endif
};

#endif