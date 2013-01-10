#ifndef DATABASE_H
#define DATABASE_H

#using <mscorlib.dll>
#using <System.dll>
#using <System.Data.dll>
#include "CustomLogger.h"
#include "TetraInterface.h"
#include "Consumidor.h"

#using <System.Xml.dll>

using namespace System::Data;
using namespace System::Data::Odbc;
using namespace System::Threading;
using namespace System::Collections;

//#define numRep 10
#define PERTENENCIA_ESTATICA			99
#define PERTENENCIA_ERROR				-2
#define PERTENENCIA_NO_EXISTE			-1
#define PERTENENCIA_OK					0
#define PERTENENCIA_PENDIENTE_AÑADIR	1
#define PERTENENCIA_PENDIENTE_BORRAR	2
#define PERTENENCIA_AÑADIR_FALLO		3
#define PERTENENCIA_BORRAR_FALLO		4

#define PER_ATTACH_UNKNOWN				-1
#define PER_ATTACHED					0
#define PER_ATTACH_PENDING				1
#define PER_DETACH_PENDING				2
#define PER_DETACHED					3

#define SUBSCRIBER_TERMINAL			'P'
#define SUBSCRIBER_APP				'A'
#define SUBSCRIBER_GRUPO_ESTATICO	'S'
#define SUBSCRIBER_GRUPO_EXTENSIBLE	'X'
#define SUBSCRIBER_GRUPO_DINAMICO	'D'

// Los SDS sin full TL no tendrán un handle (message reference) por SDS, ya que no reciben reports posteriores
#define SDS_HANDLE_SIMPLE				-1
#define SDS_HANDLE_STATUS				-2

#define DB_INSERT	0
#define DB_UPDATE	1
#define DB_DELETE	2

#define DB_RADIO_ACTIVA 'S'

ref class Consumidor;
ref class Linea_tetra;

ref class Pertenencia
{
public:
	Pertenencia( System::Int32 _g_ssi, System::Int32 _t_ssi, System::Int32 _estado, System::DateTime^ _t_cambio, System::Int32 _intentos )
	{
		this->g_ssi = _g_ssi;
		this->t_ssi = _t_ssi;
		this->estado = _estado;
		this->t_cambio = _t_cambio;
		this->intentos = _intentos;
	}
	Pertenencia( System::Int32 _g_ssi, System::Int32 _t_ssi )
	{
		this->g_ssi = _g_ssi;
		this->t_ssi = _t_ssi;
	}
	System::Int32 g_ssi;
	System::Int32 t_ssi;
	System::Int32 estado;
	System::DateTime^ t_cambio;
	System::Int32 intentos;
	bool explorado;
	bool estatica;
};

ref class Subscriber
{
public:
	Subscriber( bool _grupo, System::Int32 _ssi, System::String^ _nombre, System::Char _tipo )
	{
		this->grupo = _grupo;
		this->ssi = _ssi;
		this->nombre = _nombre;
		this->tipo = _tipo;
	}
	bool grupo;
	System::Int32 ssi;
	System::String^ nombre;
	System::Char tipo;
	bool explorado;
};

ref class Database
{
private:

public:
	Database ();
	bool connect ( System::String^ connectionString );

	Generic::List<Linea_tetra^>^ read_lines_info ( );

	System::Int32 readTopSdsId();
	bool read_sds_status_text ( System::Int32 status_number, System::String^ %contenido );

	void reset_register_status ( );

	int insert_SDS ( bool recibido, bool despachador, System::Int32 origen, System::Int32 destino,
							System::String^ autor, int handle, System::String^ tl, System::String^ contenido,
							bool procesado, bool consumido, System::DateTime^ _t_tetra, System::DateTime^ _t_procesado, System::DateTime^ _t_consumido );

	bool update_status ( int ssi, System::DateTime^ time, bool registrado, System::String^ zona );

	ArrayList^ read_all_pertenencias_grupo( System::Int32 g_ssi );
	Pertenencia^ read_pertenencia ( int group_ssi, int individual_ssi );
	bool update_pertenencias ( int group_ssi, int individual_ssi, int estado, int operacion );
	bool clear_pertenencias ( int individual_ssi );
	void clear_pertenencias_all ();

	void clear_attached ( );
	bool update_attached ( int group_ssi, int individual_ssi, int attached );

	void check_pertenencias ( Consumidor^ consumidor, TetraInterface* tetra );
	void check_attached ( Consumidor^ consumidor, TetraInterface* tetra );

	bool update_subscribers ( array<Subscriber^>^ subscriber_list );
	bool update_pertenencias_estaticas ( array<Pertenencia^>^ pertenencias_estaticas_xml );

	void fill_db ( );
	void fill_db_pertenencias ( );
	

	OdbcConnection^ connection;				// Será ademas un lock para hacer thread-safe el acceso a la base de datos. Se usará el método Monitor::Enter(connection)-Monitor::Exit(connection)
	System::Int32 destino;

	System::DateTime before, after, start, end;

	CustomLogger^ logger;

	long long ticks;
	long long numInsert;

	int sds_nextid;

	bool db_enabled;
};
#endif