#ifndef TETRAINTERFACE_H
#define TETRAINTERFACE_H

#define ACAPI_DLL_KEY		"Pg8sb10x1yvMDS2PgMhL"
#define ACAPI_DEBUG_LEVEL	0
	
#define OPER_USER	0
#define DISP_USER	1

#define SDS_APP		0
#define MON_APP		1
#define SS_APP		2
#define CC_APP		3

#define HANDLE_INFO	0
#define ESTADO_INFO	1

// ---------------- Tipos de llamada ---------------------------
#define CC_TIPO_POINTTOPOINT			0		// SIMPLEX
#define CC_TIPO_POINTTOMULTIPOINT		1		// SIMPLEX
#define CC_TIPO_POINTTOMULTIPOINTACK	2		// SIMPLEX		// not implemented by the infrastructure
#define CC_TIPO_BROADCAST				3		// SIMPLEX
#define CC_TIPO_DUPLEX					4
#define CC_TIPO_NO_DUPLEX				5		// mon_cc_info solo da informacion de duplex

#define CC_AMBIENCE_LISTEN				9

// ------------- Estado de una llamada ---------------------------
#define CC_EMPTY		-1		// no se está llamando
#define CC_PROGRESSING	0		// cc_information
#define CC_QUEUED		1		// cc_information
#define CC_SUBS_PAGED	2		// cc_information				// not implemented by tetra infrastructure
#define CC_RINGING		3		// cc_information
#define CC_CONTINUE		4		// cc_information
#define CC_HANG_EXPIRED	5		// cc_information
#define CC_ORDER_SENT	6		// Se mando la orden al servidor acapi, aun no ha habido respuesta ("ack deberia ser instantaneo")
#define CC_ACKED		7		// cc_setup_ack
#define CC_ESTABLISHED	8		// cc_connect_ind / cc_connect_ack
#define CC_RECEIVED		9		// cc_setup_ind
#define CC_ACCEPTING	10		// Se ha emitido una orden de acceptar llamada
#define CC_HANGING_UP	11		// Se ha emitido una orden de colgar llamada

// ------------- Estado de una llamada MON (mon_cc_information) ---------------------------
#define MON_EMPTY								-1		// no se está llamando
#define MON_CALL_SETUP							0
#define MON_CALL_SETUP_REJECT					1
#define MON_ALERT_FROM_CALLED_PARTY				2
#define MON_ACCEPT_FROM_CALLED_PARTY			3
#define MON_CALL_CONNECT						4
#define MON_INCLUDE_CALL_SETUP					5
#define MON_INCLUDE_CALL_SETUP_REJECT			6
#define MON_INCLUDE_ALERT_FROM_CALLED_PARTY		7
#define MON_INCLUDE_ACCEPT_FROM_CALLED_PARTY	8
#define MON_INCLUDE_CALL_CONNECT				9
#define MON_RESERVED							10

//------------------ctes llamadas-------------
#define CC_RTP_PAYLOAD_TYPE		0		// 0-PCMU, 8-PCMA, 99-TetraSpeech
#define CC_RTP_PORT				5555
#define CC_HOOK_SIGNALLING		1		// 0 hook signalling, 1 NO
#define CC_ENCRYPTION			0		// 0-clear mode, 1-tetra EndToEnd encrytion
#define CC_RTP_SRC_IP			"172.20.222.87"
#define CC_PRIORITY_TYPE		2		// 0 = priorityNotDefined, 1 = lowPriority, 2 = highPriority, 3 = emergencyPriority

#define CC_HOOK_SIGNALLING_TRUE		0

#define CC_PTT_NONFLOOPBACK			1

//------------------ctes DGNA-------------
#define DGNA_CLASS_OF_USAGE			4		// 4 - Default, 5 - Selected, 8 - Background

#include "windows.h"
#include "stdio.h"
#include "string"
#include <iostream>
#include "CAcapiDll.h"

/*
*	UNMANAGED CODE!!!!
*/
using namespace std;

typedef void (__stdcall *ANSWERCB)(int, string);

class TetraInterface : public CAcapiDllInterface
{
public:
    virtual ~TetraInterface(void);
    TetraInterface();

	int connect( std::string _key, std::string _ip_addr, short _debug_level, std::string _user, std::string _passwd );
    void commandLine();
	void receivedMessage(std::string message);					// callback registrado en la DLL, recibirá todos los mensajes del servidor ACAPI

	short connect_service( int app_num, int _ssi );

	bool monitoring_order( short mon_app, int _begin_ssi, int _end_ssi, short mon_service );
	bool monitoring_orderreq_range( short mon_app, int regMonitoring, int sdsMonitoring, int ccMonitoring, long ssi, long end_ssi );
	bool monitoring_orderreq_single( short mon_app, int regMonitoring, int sdsMonitoring, int ccMonitoring, long ssi );

	bool monitoring_close_order( short mon_app, int mon_handle );
	bool mon_intercept( short app_handle, short mon_handle, int _app_ssi, long call_id, char *src_ip, long rtp_port, long rtp_payload );
	bool mon_intercept_disconnect( short app_handle, short mon_handle, int _app_ssi, long call_id );

	bool mon_force_end_call( short app_handle, short mon_handle, long call_id, long result );

	bool mon_ptt ( int app_handle, int mon_handle, int _involved_ssi, int call_id, short active, short priority );

	void setCallbackPointer( ANSWERCB _cb );
	ANSWERCB managed_callback;

	//	Actions from AC core
	int send_sds( int app_handle, int disp, char *sds_data, int length, char *dst_ssi, bool ack );
	bool send_sds_status( int app_handle, int disp, int dst_ssi, int status_number );

	bool modifica_pertenencias( int handle, bool estatico, bool add, int group_ssi, int individual_ssi );

	bool call_duplex ( int app_handle, int ssi_dst, char *src_ip, int rtp_port, int rtp_payload, int hook_signalling, int priority_type, int priority, bool encryption );
	bool call_simplex ( int app_handle, int ssi_dst, char *src_ip, int rtp_port, int rtp_payload, int hook_signalling, int priority_type, int priority, bool encryption );
	bool call_group ( int app_handle, int ssi_dst, char *src_ip, int rtp_port, int rtp_payload, int hook_signalling, int priority_type, int priority, bool encryption );

	bool call_listen_ambience( int app_handle, int ssi_dst, char *src_ip, int rtp_port, int rtp_payload, int hook_signalling, int priority_type, int priority, int comm_type, bool encryption );

	bool acepta_llamada( int app_handle, int call_id, char *src_ip, int rtp_port, int rtp_payload, bool encryption );
	bool acepta_llamada_grupo( int app_handle, int call_id, int _group_ssi, char *src_ip, int rtp_port, int rtp_payload, bool encryption );

	bool cuelga_llamada  ( int app_handle, int call_id, int cause );
	bool ptt ( int app_handle, int call_id, short active, short priority );


	short debug_level;
	char ip_addr[20];
	char user[30];
	char passwd[30];

  private:
    void prompt();
    void readCommand();
    void help();
    bool processCommand();

    char buffer[4096];
    char *argv[2048];
    int argc;
};

#endif
