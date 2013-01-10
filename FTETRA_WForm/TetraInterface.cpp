#include "TetraInterface.h"
#include "stdio.h"

int sdsAckCount=0;
int sdsDataCount=0;


TetraInterface::TetraInterface()  :
                             buffer(),
                             argv(),
                             argc(0)
{

}

TetraInterface::~TetraInterface(void)
{

}


int TetraInterface::connect( std::string _key, std::string _ip_addr, short _debug_level, std::string _user, std::string _passwd )
{
	int result;
	char key[40];

	strcpy(this->ip_addr, _ip_addr.c_str());
	this->debug_level = _debug_level;
	strcpy(this->user, _user.c_str());
	strcpy(this->passwd, _passwd.c_str());

	strcpy(key, _key.c_str());

	printf ( "IP-Addr=%s debuglevel=%d\n", this->ip_addr, debug_level );
	printf ("----------------------------------------------------------\n");
  
	result = acapiDllStartWithInterface(key, ip_addr, debug_level, this);		// Licenzkey, IP-Adress TAP, Logger-Interface
	printf("||||--------START_DLL--------||||   Resultado: %d\n", result);

	return result;
}

void TetraInterface::setCallbackPointer(ANSWERCB _cb)
{
	this->managed_callback = _cb;
}

short TetraInterface::connect_service( int app_num, int _ssi )
{
	char ssi[10];
	if( sprintf( ssi, "%d", _ssi ) < 0 )
		printf( "interface tetra, problema en connect_service" );
	switch ( app_num )
	{
		case CC_APP:
			return connectRtpCCService( this->user, this->passwd, ssi, CC_RTP_PAYLOAD_TYPE );
			break;
		case SDS_APP:
			return connectSdsService( this->user, this->passwd, ssi);
			break;
		case SS_APP:
			return connectSupplementaryService( this->user, this->passwd, ssi );
			break;
		case MON_APP:
			return connectRtpMonitoringService( this->user, this->passwd, ssi, CC_RTP_PAYLOAD_TYPE );
			break;
	}
}

/*
	Se transmite una orden de monitorización desde el servicio de monitorización "mon_app" para los ssis
	entre "_begin_ssi" y "_end_ssi" del servicio "mon_service". Losposivbles servicios de monitorización son los siguientes:
		bit 0: registration , -- Registration, Deregistration, Roaming (MM)
		bit 1: sdsInitiator , -- Short Data Service Unitdata as Initiator (SDS)
		bit 2: sdsDestination , -- Short Data Service Unitdata as Destination (SDS)
		bit 3: circuitModeTarget , -- Circuit Mode Voice / Data Calls as Initiator (CC)
		bit 4:circuitModeDestination, -- Circuit Mode Voice / Data Calls as Destination (CC)
	return true si fue posible mandar la orden al servidor ACAPI
*/
bool TetraInterface::monitoring_order( short mon_app, int _begin_ssi, int _end_ssi, short mon_service )
{
	char begin_ssi[10];
	char end_ssi[10];
	if( sprintf( begin_ssi, "%d", _begin_ssi ) < 0 )
		printf( "interface tetra, problema en monitoring_order" );
	if( sprintf( end_ssi, "%d", _end_ssi ) < 0 )
		printf( "interface tetra, problema en monitoring_order" );
	return monMonitoringReq( mon_app, begin_ssi, end_ssi, mon_service );
}


/*
reg_monitoring: requested services for call registration monitoring
	0: No service
	1: Request for Registration Monitoring, no initial briefing
	2: Request for Registration Monitoring, initial briefing for registered subscribers
	3: Request for Registration Monitoring, initial briefing for all subscribers
sds_monitoring : requested services for sds monitoring
	0: No service
	1: Request for SDS Monitoring
	2: Request for SDS Monitoring, but suppress SDS-TL-Reports -- (only possible for a monTarget of type fleetNumber)
cc_monitoring : requested services for call control monitoring
	0: No service
	1: Request for Monitoring of Circuit Mode Calls.
ssi: first SSI number for monitoring range
end_ssi: last ssi number for monitoring range
*/
bool TetraInterface::monitoring_orderreq_range( short mon_app, int regMonitoring, int sdsMonitoring, int ccMonitoring, long ssi, long end_ssi )
{
	return monOrderReqRange( mon_app, regMonitoring, sdsMonitoring, ccMonitoring, ssi, end_ssi );
}

/*
reg_monitoring: requested services for call registration monitoring
	0: No service
	1: Request for Registration Monitoring, no initial briefing
	2: Request for Registration Monitoring, initial briefing for registered subscribers
	3: Request for Registration Monitoring, initial briefing for all subscribers
sds_monitoring : requested services for sds monitoring
	0: No service
	1: Request for SDS Monitoring
	2: Request for SDS Monitoring, but suppress SDS-TL-Reports -- (only possible for a monTarget of type fleetNumber)
cc_monitoring : requested services for call control monitoring
	0: No service
	1: Request for Monitoring of Circuit Mode Calls.
ssi:  SSI number for monitoring
*/
bool TetraInterface::monitoring_orderreq_single( short mon_app, int regMonitoring, int sdsMonitoring, int ccMonitoring, long ssi )
{
	char lame[10];

	memset( &lame, 0, sizeof( lame ) );

	return monOrderReqSingle( mon_app, regMonitoring, sdsMonitoring, ccMonitoring, ssi, lame );
}

/*
	Transmite una orden de cerrar un servicio de monitorizacion al servidor ACAPI
*/
bool TetraInterface::monitoring_close_order( short mon_app, int mon_handle )
{
	return monOrderClose( mon_app, mon_handle );
}

bool TetraInterface::mon_intercept( short app_handle, short mon_handle, int _app_ssi, long call_id, char *src_ip, long rtp_port, long rtp_payload )
{
	char app_ssi[10];
	itoa( _app_ssi, app_ssi, 10 );

	// Se establece que para el caso de escuchas de llamadas duplex se escuhara por el puerto asociado a la linea y por el puerto par siguiente
	// Las comunicaciones RTP siempre deben hacerse por puertos pares ya que los impares se usan para RTCP ( control ).
	// Por ello es necesario que los puertos asociados a las lineas sean pares y esten separados al menos 4 unidades
	return monRtpInterceptReq ( app_handle, mon_handle, app_ssi, call_id, src_ip, rtp_port, rtp_payload, src_ip, rtp_port+2, rtp_payload );
}

bool TetraInterface::mon_intercept_disconnect( short app_handle, short mon_handle, int _app_ssi, long call_id )
{
	char app_ssi[10];
	itoa( _app_ssi, app_ssi, 10 );

	return monInterceptDisconnect ( app_handle, mon_handle, app_ssi, call_id );
}

bool TetraInterface::mon_force_end_call( short app_handle, short mon_handle, long call_id, long result )
{
	return monForcedCallEnd ( app_handle, mon_handle, call_id, result );
}

//****************************************************************************************
void TetraInterface::prompt()
{
  //write a prompt
  std::cout << ">";
}

//****************************************************************************************
void TetraInterface::commandLine()
{
  bool finished = false;

  help();
  while(!finished)
  {
    prompt();
    readCommand();
    finished = processCommand();
  }
}

//****************************************************************************************
// In der Methode readCommand wird das Commando von der System::Console gelesen
void TetraInterface::readCommand()
{
  //read a line
  std::cin.getline(buffer, sizeof(buffer));
  //split the line in arguments
  argc = 0;
  {
    int bufferPos = 0;
    bool whitespace = true;
    while(buffer[bufferPos] != 0)
    {
      if(buffer[bufferPos] == ' ')
      {
        if(!whitespace)
        {
          buffer[bufferPos] = 0;
          whitespace = true;
        }
      }
      else
      {
        if(whitespace)
        {
          argv[argc] = &(buffer[bufferPos]);
          ++ argc;
          whitespace = false;
        }
      }
      ++ bufferPos;
    }
  }
  //debug output
  for(int i = 0; i < argc; ++ i)
  {
    std::cout << "  argv[" << i << "] = \"" << argv[i] << "\"\n";
  }
}

//****************************************************************************************
// in der help-Methode wird die help-Message ausgegeben.
void TetraInterface::help()
{
  //write a help message
  std::cout << "Available commands:\n";
  std::cout << "  help\n";
  std::cout << "  exit\n";
  std::cout << "  connect_sds_service <login name> <password> <ssi>\n";
  std::cout << "  connect_cc_service <login name> <password> <ssi> <trunk id>\n";
  std::cout << "  connect_mon_service <login name> <password> <ssi>\n";
  std::cout << "  connect_supp_service <login name> <password> <ssi>\n";
  std::cout << "  send_sds <app_handle> <destination ssi> <sds message>\n";
  std::cout << "  send_status <app_handle> <destination ssi> <sds status>\n";
  std::cout << "  group_attach <app_handle> <group_ssi> <affected ssi> <class of usage>\n";
  std::cout << "  group_detach <app_handle> <group_ssi> <affected ssi> <class of usage>\n";
  std::cout << "  group_add <app_handle> <group_ssi> <affected ssi> <class of usage>\n";
  std::cout << "  group_del <app_handle> <group_ssi> <affected ssi>\n";
  std::cout << "  group_intorraget <app_handle> <group_ssi>\n";
  std::cout << "  monitoring <app_handle> <begin_SSI> <end_SSI> <monitoring service>\n";
  std::cout << "  mon_close <app_handle> <monitoring_handle>\n";
  std::cout << "  cc_duplex <app_handle> <destination ssi> <trunk id> <channel id> <hook signalling> <priority type> <priority>\n";
  std::cout << "  cc_RtpDuplex <app_handle> <destination ssi> <rtpIpAddress> <rtpPort> <rtpPayloadType> <hook signalling> <priority type> <priority> <encryption>\n";
  std::cout << "  cc_ExDuplex <app_handle> <gateway_id> <dialing_string> <trunk_number> <channel_number> <sub_channel> <hook signalling> <priority type> <priority> <encryption>\n";
  std::cout << "  cc_simplex <app_handle> <destination ssi> <trunk id> <channel id> <hook signalling> <priority type> <priority>\n";
  std::cout << "  cc_group <app_handle> <destination ssi> <trunk id> <channel id> <hook signalling> <priority type> <priority>\n";
  std::cout << "  cc_ptt <app_handle> <call_id> <active (0/1)> <priority>\n";
  std::cout << "  cc_accept <app_handle> <trunk id> <channel id>\n";
  std::cout << "  cc_disconnect <app_handle> <disconnect cause>\n";
  std::cout << "  cc_wait <app_handle> <call_ssi> <other_ssi> <call_id>  \n";
  std::cout << "  cc_wait_ack <app_handle> <call_ssi> <other_ssi> <call_id>\n";
  std::cout << "  cc_continue <app_handle> <call_ssi> <other_ssi> <call_id>\n";
  std::cout << "  cc_continue_ack <app_handle> <call_ssi> <other_ssi> <call_id>  <result> \n";
  std::cout << "  cc_include <app_handle> <include_ssi> <other_ssi> <hook signalling> <call_id>\n";
  std::cout << "  cc_cancel_include <app_handle> <include_ssi> <other_ssi> <call_id>\n";

}

//****************************************************************************************
// processCommand wertet das eingegebene Commando aus und ruft dann die entsprechende
// Methode der ACAPI-Dll auf.
int msg_number = 0;

bool TetraInterface::processCommand()
{
  bool finished = false;

  //process command
  if(argc > 0)
  {
    if(strcmp(argv[0], "help") == 0)
    {
      help();
    }
    else if(strcmp(argv[0], "exit") == 0)
    {
      if(argc == 1)
      {
        finished = true;
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "connect_sds_service") == 0)
    {
      if(argc == 4)
      {
        long app_handle = connectSdsService(argv[1], argv[2], argv[3]);
		    printf("returned app_handle = %ld", app_handle);
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "connect_cc_service") == 0)
    {
      if(argc == 5)
      {
        long app_handle = connectRtpCCService(argv[1], argv[2], argv[3], atoi(argv[4]));
		    printf("returned app_handle = %ld", app_handle);
      }
      else
      {
        help();
      }
    }
	 else if(strcmp(argv[0], "connect_cc_serviceS") == 0)
    {
      if(argc == 5)
      {
        long app_handle = connectCCService(argv[1], argv[2], argv[3], argv[4]);
		    printf("returned app_handle = %ld", app_handle);
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "connect_supp_service") == 0)
    {
      if(argc == 4)
      {
        long app_handle = connectSupplementaryService(argv[1], argv[2], argv[3]);
		    printf("returned app_handle = %ld", app_handle);
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "connect_mon_service") == 0)
    {
      if(argc == 5)
      {
        long app_handle = connectRtpMonitoringService(argv[1], argv[2], argv[3], atoi(argv[4]));
		    printf("returned app_handle = %ld", app_handle);
      }
      else
      {
        help();
      }
    }
	else if(strcmp(argv[0], "connect_mon_serviceS") == 0)
    {
      if(argc == 5)
      {
        long app_handle = connectMonitoringService(argv[1], argv[2], argv[3], 2, 3, 1);
		    printf("returned app_handle = %ld", app_handle);
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "send_sds") == 0)
    {
      if(argc == 4)
      {
        sendSdsData(atoi(argv[1]), msg_number++, argv[2], strlen(argv[3]), argv[3]);
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "send_status") == 0)
    {
      if(argc == 4)
      {
        sendSdsStatus(atoi(argv[1]), msg_number++, argv[2], atoi(argv[3]));
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "group_attach") == 0)
    {
      if(argc == 4)
      {
        groupAttach    (atoi(argv[1]), argv[2], argv[3]);
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "group_detach") == 0)
    {
      if(argc == 4)
      {
        groupDetach    (atoi(argv[1]), argv[2], argv[3]);
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "group_add") == 0)
    {
      if(argc == 5)
      {
        dynGroupAdd(atoi(argv[1]), argv[2], argv[3], atoi(argv[4]));
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "group_del") == 0)
    {
      if(argc == 4)
      {
        dynGroupDel(atoi(argv[1]), argv[2], argv[3]);
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "group_intorraget") == 0)
    {
      if(argc == 3)
      {
        dynGroupInterrogate(atoi(argv[1]), argv[2]);
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "monitoring") == 0)
    {
      if(argc == 5)
      {
        monMonitoringReq(atoi(argv[1]), argv[2], argv[3], atoi(argv[4]));
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "mon_close") == 0)
    {
      if(argc == 3)
      {
        monClose(atoi(argv[1]), atoi(argv[2]), argv[3], argv[4]);
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "cc_duplex") == 0)
    {
      if(argc == 8)
      {
        callDuplex (atoi(argv[1]), argv[2], atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]));
      }
      else
      {
        help();
      }
    }
	else if(strcmp(argv[0], "cc_RtpDuplex") == 0)
	{
		if(argc == 10)
		{
			callRtpDuplex(atoi(argv[1]), argv[2], argv[3], atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), atoi(argv[8]), atoi(argv[9]));
		}
	}
	else if(strcmp(argv[0], "cc_ExDuplex") == 0)
	{
		if(argc == 11)
		{
			callExDuplex(atoi(argv[1]), atoi(argv[2]), argv[3], atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]), atoi(argv[8]), atoi(argv[9]), atoi(argv[10]));
		}
	}
    else if(strcmp(argv[0], "cc_simplex") == 0)
    {
      if(argc == 8)
      {
        callSimplex (atoi(argv[1]), argv[2], atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]));
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "cc_group") == 0)
    {
      if(argc == 8)
      {
        callGroup (atoi(argv[1]), argv[2], atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]));
      }
      else
      {
        help();
      }
    } 
    else if(strcmp(argv[0], "cc_ptt") == 0)
    {
      if(argc == 4)
      {
        callPTT (atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "cc_accept") == 0)
    {
      if(argc == 4)
      {
        callAccept (atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "cc_disconnect") == 0)
    {
      if(argc == 3)
      {
        callDisconnect (atoi(argv[1]), atoi(argv[2]), atoi(argv[4]));
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "cc_wait") == 0)
    {
      if(argc == 5)
      {
        callWait (atoi(argv[1]), argv[2], argv[3], atol(argv[4]));
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "cc_continue") == 0)
    {
      if(argc == 5)
      {
        callContinue (atoi(argv[1]), argv[2], argv[3], atol(argv[4]));
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "cc_wait_ack") == 0)
    {
      if(argc == 6)
      {
        callWaitAck (atoi(argv[1]), argv[2], argv[3], atol(argv[4]), atoi(argv[5]));
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "cc_continue_ack") == 0)
    {
      if(argc == 6)
      {
        callContinueAck (atoi(argv[1]), argv[2], argv[3], atol(argv[4]), atoi(argv[5]));
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "cc_include") == 0)
    {
      if(argc == 6)
      {
        callInclude (atoi(argv[1]), argv[2], argv[3], atol(argv[4]), atoi(argv[5]));
      }
      else
      {
        help();
      }
    }
    else if(strcmp(argv[0], "cc_cancel_include") == 0)
    {
      if(argc == 5)
      {
        callCancelInclude (atoi(argv[1]), argv[2], argv[3], atoi(argv[5]));
      }
      else
      {
        help();
      }
    }
	else if(strcmp(argv[0], "1") == 0)
    {
      if(argc == 1)
      {
        //simulaSDS();
      }
    }
    else
    {
      help();
    }
  }

  return finished;
}

//  callback registrado en la DLL, recibirá todos los mensajes del servidor ACAPI
void TetraInterface::receivedMessage(std::string message)
{
	//printf ("				received message = %s\n", message.c_str());
	char c_msg[200];
	char keyAux[3];
	strncpy (keyAux, message.c_str(), 3);
	strncpy (c_msg, message.c_str(), 200);
	keyAux[2] = 0;
	int key= strtol(keyAux,0,16);

	// paso el callback al consumidor para que la trate desde managed code
	managed_callback(key, message);
}

int TetraInterface::send_sds( int app_handle, int disp, char *sds_data, int length, char *dst_ssi, bool ack )
{
	// disp => enviar desde despachadores u operadores
	return sendSdsDataWithAck( app_handle, msg_number++, dst_ssi, length, sds_data, ack ? 1:0 )
		? 1 : -1;
}

bool TetraInterface::send_sds_status( int app_handle, int disp, int dst_ssi, int status_number )
{
	char ssi[10];
	
	itoa( dst_ssi, ssi, 10 );
	// disp => enviar desde despachadores u operadores
	return sendSdsStatus ( app_handle, msg_number, ssi, status_number);
}

bool TetraInterface::modifica_pertenencias( int handle, bool estatico, bool add, int group_ssi, int individual_ssi )
{
	char g_ssi[10];
	char t_ssi[10];
	
	itoa( group_ssi, g_ssi, 10 );
	itoa( individual_ssi, t_ssi, 10 );

	if ( estatico )
	{
		if ( add )
		{
			return groupAttach( handle, t_ssi, g_ssi );
		}
		else
		{
			return groupDetach( handle, t_ssi, g_ssi );
		}
	}
	else
	{
		if ( add )
		{
			return dynGroupAdd( handle, g_ssi, t_ssi, DGNA_CLASS_OF_USAGE );
		}
		else
		{
			return dynGroupDel( handle, g_ssi, t_ssi );
		}

	}
}

bool TetraInterface::call_duplex( int app_handle, int ssi_dst, char *src_ip, int rtp_port, int rtp_payload, int hook_signalling, int priority_type, int priority, bool encryption )
{
	char called_ssi[10];
	itoa( ssi_dst, called_ssi, 10 );

	return callRtpDuplex( app_handle, called_ssi, src_ip, rtp_port, rtp_payload, hook_signalling, priority_type, priority, encryption );
}

bool TetraInterface::call_simplex( int app_handle, int ssi_dst, char *src_ip, int rtp_port, int rtp_payload, int hook_signalling, int priority_type, int priority, bool encryption )
{
	char called_ssi[10];
	itoa( ssi_dst, called_ssi, 10 );

	return callRtpSimplex( app_handle, called_ssi, src_ip, rtp_port, rtp_payload, hook_signalling, priority_type, priority, encryption );
}

bool TetraInterface::call_group( int app_handle, int ssi_dst, char *src_ip, int rtp_port, int rtp_payload, int hook_signalling, int priority_type, int priority, bool encryption )
{
	char called_ssi[10];
	itoa( ssi_dst, called_ssi, 10 );

	return callRtpGroup( app_handle, called_ssi, src_ip, rtp_port, rtp_payload, hook_signalling, priority_type, priority, encryption );
}

bool TetraInterface::call_listen_ambience( int app_handle, int ssi_dst, char *src_ip, int rtp_port, int rtp_payload, int hook_signalling, int priority_type, int priority, int comm_type, bool encryption )
{
	char called_ssi[10];
	itoa( ssi_dst, called_ssi, 10 );

	return callRtpAmbienceListening( app_handle, called_ssi, src_ip, rtp_port, rtp_payload, hook_signalling, priority_type, priority, comm_type, encryption );
}

bool TetraInterface::acepta_llamada( int app_handle, int call_id, char *src_ip, int rtp_port, int rtp_payload, bool encryption )
{
	return callRtpAccept( app_handle, call_id, src_ip, rtp_port, rtp_payload, encryption );
}

bool TetraInterface::acepta_llamada_grupo( int app_handle, int call_id, int _group_ssi, char *src_ip, int rtp_port, int rtp_payload, bool encryption )
{
	char group_ssi[10];
	itoa( _group_ssi, group_ssi, 10 );

	return callRtpAcceptGroup( app_handle, call_id, group_ssi, src_ip, rtp_port, rtp_payload, encryption );
}

/*
	Transmite una orden cc_disconnect al servidor acapi
DisconnectCause :
	0 causeUnknown
	1 userRequestedDisconnect
	2 calledPartyBusy
	3 calledPartyNotReachable
	4 calledPartyDoesNotSupportEncryption
	5 congestionInInfrastructure
	6 notAllowedTrafficCase
	7 incompatibleTrafficCase
	8 requestedServiceNotAvailable
	9 preEmptiveUseOfResource
	10 invalidCallIdentifier
	11 callRejectedByCalledParty
	12 noIdleCcEntity
	13 expiryOfTimer
	14 dmxRequestedDisconnect
	15 acknowledgedServiceNotCompleted
	16 resourceFailed
	17 unknownResource
	18 inactiveResource
	19 resourceUsedInOtherCall
	20 cellReSelection
	21 callSetupRepetition
	22 icLeavingCallNoCallEnd
	23 unknownTetraIdentity
	24 ssSpecificDisconnection
	25 unknownExternalSubscriberIdentity
	26 callRestorationOfOtherUserFailed
	27 wrongCommunicationType
	28 Call Transfer: Calling Party of CC-Setup-Req PDU
	29 UnknownVirtualDest -- InterceptFailCause 1
	30 routingFailureToVirtualDest -- InterceptFailCause 2
	31 offlineCircuitModeMonitoringGateway -- InterceptFailCause 3
	32 unknownCallIdentifier -- InterceptFailCause 4
	33 monToSameVirtualDestAlreadyActive -- InterceptFailCause 7
	34 unknownMonitoringHandle -- Connection refused because the monitoring handle is unknown
	35 callInWrongState -- Monitoring of call refused because call is in the wrong state
*/
bool TetraInterface::cuelga_llamada  ( int app_handle, int call_id, int cause )
{
	return callDisconnect( app_handle, call_id, cause );
}

/*
active:	1 = PTT pressed
		0 = PTT opened
priority:
		0 = lowLevel
		1 = highLevel
		2 = preEmptiveLevel
		3 = emergencyLevel
		4 = extraEmergencyLevel1 (Enables applications to override even emergency level)
		5 = extraEmergencyLevel2 (Enables applications to override even emergency levels)
*/
bool TetraInterface::ptt ( int app_handle, int call_id, short active, short priority )
{
	if ( active == 1 )
	{
		return callDemand ( app_handle, call_id, priority, CC_PTT_NONFLOOPBACK );
	}
	else if ( active == 0 )
	{
		return callCeased ( app_handle, call_id );
	}
	else
	{
		return false;
	}

	// Ahora se usa callDemand por el parametro CC_PTT_NONFLOOPBACK, para que no haya retorno de audio
	// return callPTT( app_handle, call_id, active, priority );
}

/*
active:	1 = PTT pressed
		0 = PTT opened
priority:
		0 = lowLevel
		1 = highLevel
		2 = preEmptiveLevel
		3 = emergencyLevel
		4 = extraEmergencyLevel1 (Enables applications to override even emergency level)
		5 = extraEmergencyLevel2 (Enables applications to override even emergency levels)
*/
bool TetraInterface::mon_ptt ( int app_handle, int mon_handle, int _involved_ssi, int call_id, short active, short priority )
{
	char involved_ssi[10];
	itoa( _involved_ssi, involved_ssi, 10 );

	if ( active == 1 )
	{
		return monUplinkTXDemand ( app_handle, mon_handle, involved_ssi, call_id, priority, CC_PTT_NONFLOOPBACK );
	}
	else
	{
		return monUplinkTXCeased ( app_handle, mon_handle, involved_ssi, call_id );
	}
}