#ifndef TCPMANAGER_H
#define TCPMANAGER_H

#include "CustomLogger.h"
#include "Consumidor.h"

#using <System.dll>

using namespace System::IO;
using namespace System::Net;
using namespace System::Net::Sockets;
using namespace System::Text;
using namespace System::Threading;

#define T_MENSAJES			1
#define T_TERMINALES		2
#define T_GRUPOS			3
#define T_PERTENENCIAS		4

// Mensajes FR_TETRA <-> AcCore
#define TRAMA_INI				"---"
#define TRAMA_END				",,,"

// Mensajes FR_TETRA <-  AcCore
#define AC_ENVIA_SDS				100
#define AC_MOD_DGNA					101
#define AC_ENVIA_SDS_STATUS			102

#define AC_CALL_DUPLEX				110
#define AC_CALL_SIMPLEX				111
#define AC_CALL_GROUP				112
#define AC_PTT						115

#define AC_PINCHA_LINEA				120

#define AC_CAMBIA_GRUPO_LINEA		130

#define AC_MOD_MON_SSI				140

#define AC_MON_FORCE_CALL_END		142
#define AC_LISTEN_MON_SSI			143
#define AC_CALL_LISTEN_AMBIENCE		144

#define AC_RESET_MON				149

// Mensajes FR_TETRA ->  AcCore
#define FRT_AVISA_CAMBIOS		800
#define KEEP_ALIVE				801
#define CC_LLAMADA_ENTRANTE		810
#define CC_LLAMADA_ESTABLECIDA	811
#define CC_LLAMADA_ACABADA		812
#define CC_PTT					815
#define PINCHA_LINEA_ACK		820
#define CAMBIA_GRUPO_LINEA_ACK	830
#define AC_MOD_MON_SSI_ACK		840
#define AC_MON_CALL_CONN		841
#define AC_MON_CALL_DISCONN		842
#define AC_LISTEN_MON_ACK		843

#define CONN_AC_EST				850


#define LAME_FR_CAMB_CH         900
#define LAME_FR_SCANNER         901

ref class CustomLogger;
ref class Consumidor;
ref class EventoCola;

ref class TcpManager
{
private:
	TcpListener^ listener;

public:
	TcpManager( System::String^ local_ip, int port );
    void listen();
	void stay_connected();

	void avisaAcCore( System::String^ msg );

	void processAcCoreOrder( array<System::Byte>^ myReadBuffer );

	void log_evento( EventoCola^ evento_a_encolar );
	
	static void DoAcceptTcpClientCallback( System::IAsyncResult^ result );

	static void AsyncRead( System::IAsyncResult^ manager );

	volatile int numCon;

	TcpClient^ acCore;
	System::Int32 accore_port;
	IPAddress^ accore_ip;

	AutoResetEvent^ waitConnection;
	AutoResetEvent^ waitRead;

	StringBuilder^ incompleteRead;

	CustomLogger^ logger;
	Consumidor^ consumidor;

	System::Int32 num_empty_reads;
};

public delegate void AsyncListenDelegate();
public delegate void AsyncConnectDelegate();

#endif