#ifndef CONSUMIDOR_H
#define CONSUMIDOR_H

#include "Database.h"
#include "TetraInterface.h"
#include "CustomLogger.h"
#include "TcpManager.h"
#include "Config_data.h"


#using <System.dll>

using namespace System::Diagnostics;
using namespace System::Runtime::InteropServices;
using namespace System::Collections;
using namespace System::Collections::Concurrent;

ref class Database;
ref class TcpManager;
ref class Config_data;

#define EVENTO_EXPIRA_SEC				2

#define NUMBER_SDS_MSG_NUMBER_TRACK		5		// número de últimos msg_number recibidos en los sds para comprobar duplicados

#define GLOBAL_TIMER_INTERVAL			500

#define NO_ITER_GLOBAL_ROUTINE			8
#define NO_ITER_CHECK_REG_MON			24
#define NO_ITER_CHECK_PER_BORRAR_FALLO	80		// cada cuantas iteraciones de timer se comprueban las pertenencias en estado "PERTENENCIA_BORRAR_FALLO"
#define NO_ITER_KEEP_ALIVE				32		// cada cuantas iteraciones de timer se envía un keep alive
#define NO_ITER_CHECK_NMS_XML			80		// cada cuantas iteraciones de timer se chequea si ha habido cambios en los archivos xml del NMS

#define NO_MAX_MON_ITEMS_LINEA			10

ref class Servicio_tetra
{
	public:
		Servicio_tetra( bool _estado_ok, int _handle )
		{
			this->estado_ok = _estado_ok;
			this->handle = _handle;
			t_pregunta = nullptr;
		}
		bool estado_ok;
		int handle;
		System::DateTime^ t_pregunta;	// Este parámetro permite hacer lógica de timeout si se intenta conectar un servicio que no responde
};

ref class Comunicacion_tetra
{
	public:
		Comunicacion_tetra( short _tipo, short _estado, int _id )
		{
			this->tipo = _tipo;
			this->estado = _estado;
			this->id = _id;
			this->tx_req_permission = true;			// acapi default. cc_ceased/granted
			this->tx_ssi = -1;
		}
		short tipo;
		short estado;
		short priority;
		int id;
		int ssi_destino;							// ssi destino de la llamada, necesario para llamadas a grupos
		int ssi_origen;								// el iniciador de la llamada
		System::Net::IPAddress^ acapi_ip;
		int acapi_rtp_port;
		System::Net::IPAddress^ acapi_ip_mon;		// usado solo en el caso de llamadas duplex monitorizadas
		int acapi_rtp_port_mon;						// usado solo en el caso de llamadas duplex monitorizadas
		bool tx_req_permission;
		bool tx_granted;
		bool exist_tx;
		int	tx_ssi;
};

ref class Monitorizacion_item
{
	public:
		Monitorizacion_item( System::Int32 ssi_mon )
		{
			this->ssi_mon = ssi_mon;
			this->mon_handle = -1;
			this->comunicacion = gcnew Comunicacion_tetra( -1, -1, -1 );
		}
		System::Int32		ssi_mon;
		System::Int32		mon_handle;			// mon_handle == -1 significa que aún no ha sido ACKED por el servidor
		bool				interviniendo;		// true si se ha emitido orden intercept y ha sido confirmada adecuadamente
		bool				stop_intervention;	// true si se ha emitido orden mon_intercept_disconnect.
		Comunicacion_tetra^ comunicacion;
};

ref class Linea_tetra
{
	public:
		bool activa;
		bool despachador;
		bool pinchada;								// linea pinchada => se cogerán automáticamente las llamadas del grupo asociado, si llamada entrante y se pincha se acepta
		bool attached;
		System::Int32 ssi_linea;
		System::Int32 ssi_grupo_asociado;
		System::Int32 ssi_grupo_defecto;

		Servicio_tetra^ cc;
		Comunicacion_tetra^ comunicacion;

		Servicio_tetra^ sds;
		Servicio_tetra^ ss;

		Servicio_tetra^ mon;

		// "mon_item_interviniendo" contendrá la referencia al item de monitorizaciones que se esta interviniendo en un momento dado. Solo se puede intervenir un item a la vez. Nullptr si no se interviene ninguno
		Monitorizacion_item^ mon_item_interviniendo;
		Generic::List<Monitorizacion_item^>^ monitorizaciones;

		System::Int32 puerto_asterisk;
};

ref class EventoCola
{
public:
	short				tipo;
	System::String^		contenido;
	System::DateTime^	expira;
};

ref class Consumidor
{
private:
	 
public:
	Consumidor ();

	bool genera_lineas ();

	void timer_check ( System::Object^ objeto );

	void timer_dummy ( System::Object^ objeto );

	void check_logica_aviso_dgna ();
	void check_connection ();
	void call_check_pertenencias ();
	void call_check_attached();
	void reset_attached( System::Int32 ssi_linea );

	int find_index_by_handle ( int handle, int app_num );
	int find_index_by_ssi( int ssi_linea );

	int find_mon_index_by_ssi( int index_linea, int mon_ssi );
	int find_mon_index_by_handle( int index_linea, int mon_handle );

	// Variables para lógica de aviso en cambios de monitorizacion. Para evitar saturación base de datos
	int					mon_espera;
	int					mon_num_iter_espera;
	
	bool				permisoAvisoMon;	// servirá para no sobrecargar al manager con el initial briefing de location_updates recibidos al registrar un nuevo servicio de monitorizacion
	void logica_aviso_mod_mon ( System::String^ msg, System::Int32 monHandle, System::Int32 t_ssi, System::DateTime^ lastUpdate );
	void check_mon_reg();

	System::Int32		mon_ssi_ini;		// Variables relacionadas con la monitorización del registro de los terminales
	System::Int32		mon_ssi_end;		// Se monitorizará automáticamente todos los terminales entre "mon_ssi_ini" y "mon_ssi_end"
	System::Int32		mon_reg_handle;		// Cada petición de monitorización tiene un handle distinto, este es el especifico del registro/desregistro de terminales


	// Variables para lógica de aviso en cambios de dgna. Para evitar saturación base de datos
	int dgna_espera;
	int dgna_num_iter_espera;
	int dgna_num_max_retries;

	System::Int32 last_dgna_group;
	bool permisoAvisoDgna;
	void logica_aviso_mod_dgna ( System::Int32 group_ssi );

	System::IntPtr callbackIntPtr ();
	void ManagedAcapiCallback ( int tipo, std::string contenido );
	delegate void CallBackDelegate ( int, std::string );

	//Delegado encargado de recibir callback de [unmanaged] TetraInterface
	CallBackDelegate^ myDelegate;

	void trabaja ();
	void trataEvento ( EventoCola^ evento );
	System::Threading::AutoResetEvent^ continua;
	
	//-------------------Callbacks----------------------
	void connection_state_callback ( System::String^ _contenido );
	void service_state_callback ( short tipo, System::String^ _contenido );
	
	void sds_callback ( short tipo, System::String^ _contenido );
	int procesaSdsTL ( short tipo, System::String^ sds_data, int *handle, [Out]System::String^ %tl, [Out]System::String^ %contenido );

	void mon_status_callback ( short tipo, System::String^ _contenido );

	void mon_order_ack_callback( System::String^ _contenido );
	void mon_order_status_callback ( short tipo, System::String^ _contenido );
	void mon_register_callback ( short tipo, System::String^ _contenido );
	void mon_cc_info_callback ( System::String^ _contenido );
	void mon_disconnect_callback ( System::String^ _contenido );
	void mon_forced_call_end_ack_callback ( System::String^ _contenido );

	void mon_intercept_connect_callback( System::String^ _contenido );
	void mon_intercept_disconnect_callback( short tipo, System::String^ _contenido );
	void mon_tx_callback( short tipo, System::String^ _contenido );
	void mon_intercept_ack_callback( System::String^ _contenido );

	System::DateTime^ procesa_timeStamp ( System::String^ timeStamp );

	void dyn_group_callback ( short tipo, System::String^ _contenido );
	void group_callback ( short tipo, System::String^ _contenido );

	void print_estado_linea ( int indice );
	void print_estado_mon_ssi( System::Int32 index_linea, System::Int32 index_mon );

	void cc_setup_ack_callback ( System::String^ _contenido );
	void cc_disconnect_callback ( System::String^ _contenido );
	void cc_information_callback ( System::String^ _contenido );
	void cc_connect_callback ( System::String^ _contenido );
	void cc_setup_callback( System::String^ _contenido );
	void cc_accepted_callback( System::String^ _contenido );
	void cc_ceased_callback( System::String^ _contenido );
	void cc_grant_callback( System::String^ _contenido );

	//-----------------AcCore ordenes-------------------
	void envia_sds ( System::String^ info );
	void envia_sds_status( System::String^ info );

	void modifica_grupo_pertenencias ( bool estatico, bool add, System::Int32 group_ssi, System::Int32 individual_ssi );
	void modifica_grupo ( System::String^ _contenido );

	void call_ssi ( short tipo, System::String^ _contenido, System::DateTime^ expiracion );
	void acepta_llamada ( System::Int32 index_linea );
	void cuelga_llamada ( System::Int32 index_linea, System::Int32 causa );

	void pincha_linea ( System::String^ _contenido );

	void cambia_grupo_asociado ( System::String^ _contenido );

	void ptt ( System::String^ _contenido );

	void modifica_item_mon ( System::String^ _contenido );
	void escucha_item_mon ( System::String^ _contenido, System::DateTime^ expiracion );
	void stop_escucha( System::Int32 index_linea );
	void force_end_mon_call ( System::String^ _contenido );

	void reset_mon_items ( System::String^ _contenido );

	bool check_duplicated_sds( System::Int32 msg_number );
	void insert_sds_msg_number( System::Int32 msg_number );

	//----------------- simulacion -------------------
	void simu_calls ( System::Int32 no_lineas, System::Int32 ssi_destino_ini );
	void pincha_all ( System::Int32 pincha );
	void cuelga_all (  );
	//___________________________________________________


	void manager_connected ();

	void proceso_escucha_rtp( bool arranca, System::Int32 index_linea );

	ConcurrentQueue<EventoCola^>^	cola;

	TetraInterface*		tetra;
	TcpManager^			manager;
	Database^			db;
	CustomLogger^		logger;
	Config_data^		read_data;		

	array<Linea_tetra^>^	lineas;			// Parasincronizar el acceso a "lineas" se utilizará el lock "lineas_lock"
	System::Object^			lineas_lock;

	System::Net::IPAddress^ asterisk_ip;
	
	int					index_first_oper;	// me indicará en que posición de lineas se encuentra el primer operador
	int					index_first_disp;	// me indicará en que posición de lineas se encuentra el primer despachador
	volatile bool		conexion_acapi_ok;
	System::TimeSpan	app_ack_timeout;	// Si un servicio no responde tras este timeout se volverá a intentar conectar

	System::Int32		num_timer_iter;

	bool change_group_to_default;

	array<System::Int32>^	last_sds_msg_numbers;		// contendrá los últimos msg_number de SDS recibidos para evitar duplicados
	System::Int32			last_sds_number_pointer;	// puntero al ultimo msg_number registrado, se irán substituyendo los msg_number más antiguos de una forma cíclica

	// evitará que se consuman eventos, no se ejecutarán los checks periódicos, con objetivo de hacer cambios en caliente. Los eventos se seguirán encolando en la cola!!!
	System::Boolean	permiso_continua;
	Object^ permiso_continua_lock;

	array<Process^>^ vlc_emite;
	array<Process^>^ vlc_escucha;
};

#endif