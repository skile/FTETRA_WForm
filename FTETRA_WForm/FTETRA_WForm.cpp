// FTETRA_WForm.cpp : main project file.

#include "stdafx.h"
#include "Form1.h"
#include "TetraInterface.h"
#include "Consumidor.h"
#include "Database.h"
#include "CustomLogger.h"
#include "TcpManager.h"
#include "Config_data.h"


#using <mscorlib.dll>
#using <system.dll>

using namespace FTETRA_WForm;

using namespace System::Threading;
using namespace System::Runtime::InteropServices;

bool call_tetra_interface ( Config_data^ data_ini, TetraInterface* tetra );

/*static void tetra_start()
{
	
	//IT WAS IN THE ORIGINAL FORM

	// Enabling Windows XP visual effects before any controls are created
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false); 

	// Create the main window and run it
	Application::Run(gcnew Form1());


}*/

[STAThreadAttribute]
int main(array<System::String ^> ^args)
{
	try
	{
		Config_data^ data_ini = gcnew Config_data();

		if ( ! data_ini->parse_data("config.xml") )
		{
			System::Console::WriteLine( "Unable to parse config data" );
			System::Console::ReadLine();
			return 1;
		}

		//data_ini->parse_nms_xml();

		CustomLogger^ logger = gcnew CustomLogger( data_ini->warning_level, 2 );			// escribirá con un nivel >= 0, en la consola y el log

		if ( ! logger->RegisterWriter( data_ini->logger_txt ) )
		{
			System::Console::WriteLine( "Unable to register logger" );
			System::Console::ReadLine();
			return 1;
		}
		logger->Write( "", 3 );
		logger->Write( "FR_Tetra", 3 );
		logger->Write( "", 3 );

		data_ini->logger = logger;

		TetraInterface *tetra = new TetraInterface();

		TcpManager^ manager = gcnew TcpManager( data_ini->acCore_local_ip, data_ini->acCore_local_port );
		manager->logger = logger;
		Database^ db = gcnew Database();
		db->logger = logger;
		//db->db_enabled =  data_ini->db_enabled;
		if ( ! db->connect( System::String::Format( "DSN={0};Uid={1};Pwd={2};", data_ini->db_dsn, data_ini->db_uid, data_ini->db_pwd ) ) )
		{
			System::Console::ReadLine();
			//return 1;
		}

		Consumidor^ consumidor = gcnew Consumidor();
		consumidor->logger = logger;
		consumidor->tetra = tetra;
		consumidor->manager = manager;
		consumidor->db = db;
		consumidor->read_data = data_ini;
		consumidor->mon_num_iter_espera = data_ini->global_monitoring_num_wait_iterations;
		consumidor->dgna_num_iter_espera = data_ini->global_dgna_num_wait_iterations;
		consumidor->dgna_num_max_retries = data_ini->global_dgna_num_max_retries;
		consumidor->asterisk_ip = data_ini->asterisk_ip;
		//consumidor->central_grupo_defecto = data_ini->central_gssi;
		consumidor->app_ack_timeout = System::TimeSpan( 0, 0, data_ini->acapi_app_ack_timeout_sec );
		consumidor->mon_ssi_ini = data_ini->global_monitoring_begin_ssi;
		consumidor->mon_ssi_end = data_ini->global_monitoring_end_ssi;

		consumidor->change_group_to_default = data_ini->change_group_to_default;

		if ( ! consumidor->genera_lineas(  ) )
		{
			logger->Write( "Problema al generar las lineas acapi", 5 );
			System::Console::ReadLine();
			return 1;
		}

		manager->consumidor = consumidor;

		// Registrar el método del consumidor asociado [managed] para ser llamado desde tetraInterface [unmanaged]
		System::IntPtr pointer = consumidor->callbackIntPtr();
		tetra->setCallbackPointer( static_cast<ANSWERCB>( pointer.ToPointer() ) );


		if ( ! call_tetra_interface( data_ini, tetra ) )
		{
			logger->Write( "Unable to start the acapi DLL", 5 );
			System::Console::ReadLine();
			return 1;
		}
	
		// Registrar el timer del consumidor, se ejecutará en una Thread distinta
		TimerCallback^ timer_cb = gcnew TimerCallback( consumidor, &Consumidor::timer_check );
		// Se inicia inmediatamente cada "data_ini->global_timer_interval" [ms]
		System::Threading::Timer^ timer = gcnew System::Threading::Timer( timer_cb, nullptr, 0, GLOBAL_TIMER_INTERVAL );

		//----------------- RUTINA PARA COMPROBAR QUE EL NMS ES ALCANZABLE Y TIENE LOS XMLs EN SU SITIO 
		// Registrar el timer de Config_data que chequeará si los xml del NMS son alcanzables
		Thread^ check_nms_conn = gcnew Thread( gcnew ThreadStart( data_ini, &Config_data::check_nms_connection ) );
		check_nms_conn->Start();

		////__________________________________________________________________________
		//// Registrar el timer del consumidor, se ejecutará en una Thread distinta
		// TimerCallback^ timer_dummy_cb = gcnew TimerCallback( consumidor, &Consumidor::timer_dummy );
		//// Se inicia inmediatamente cada "data_ini->global_timer_interval" [ms]
		// Timer^ timer_dummy = gcnew Timer( timer_dummy_cb, nullptr, 0, 500);
		////__________________________________________________________________________

		// Consumidor comienza a desencolar y tratar eventos
		Thread^ consume = gcnew Thread( gcnew ThreadStart( consumidor, &Consumidor::trabaja ) );
		consume->Start();

		/*
		// Se lanza el thread que ejecute la Visual Tetra
		Thread^ visual_tetra = gcnew Thread( gcnew ThreadStart( FTETRA_WForm, &FTETRA_WForm::tetra_start ) );
		visual_tetra->Start();



		*/
		//IT WAS IN THE ORIGINAL FORM

		// Enabling Windows XP visual effects before any controls are created
		Application::EnableVisualStyles();
		Application::SetCompatibleTextRenderingDefault(false); 

		// Create the main window and run it
		Application::Run(gcnew Form1(consumidor));





		/*
		AsyncListenDelegate^ asyncListen = gcnew AsyncListenDelegate( manager, &TcpManager::listen );
		System::IAsyncResult^ result = asyncListen->BeginInvoke( nullptr, nullptr );
		*/

		AsyncConnectDelegate^ asyncConnect = gcnew AsyncConnectDelegate( manager, &TcpManager::stay_connected );
		System::IAsyncResult^ result = asyncConnect->BeginInvoke( nullptr, nullptr );

		//tetra->commandLine();

		System::String^ line;
		array<System::String^>^ campos = nullptr;
		array<wchar_t>^ separador = {' '};
	
		while( true )
		{
			try
			{
				line = System::Console::ReadLine();
				campos = line->Split( separador, System::StringSplitOptions::RemoveEmptyEntries );
				line = campos[0];
				if ( line == "sds" )
				{
					consumidor->envia_sds( System::String::Format( "0:TEST_TEST:{0}:{1}:{2}", System::Convert::ToInt32( campos[1] ), campos[2]->Length, campos[2] ) );
				}
				if ( line == "status" )
				{
					consumidor->envia_sds_status( System::String::Format( "0:TEST_TEST:{0}:{1}", System::Convert::ToInt32( campos[1] ), System::Convert::ToInt32( campos[2] ) ) );
				}
				if ( line == "llamad" )
				{
					// duplex
					consumidor->call_ssi( AC_CALL_DUPLEX, System::String::Format( "{0}:{1}:7", campos[1], campos[2] ), nullptr );
				}
				if ( line == "llamas" )
				{
					// simplex
					consumidor->call_ssi( AC_CALL_SIMPLEX, System::String::Format( "{0}:{1}:7", campos[1], campos[2] ), nullptr );
				}
				if ( line == "llamag" )
				{
					// simplex
					consumidor->call_ssi( AC_CALL_GROUP, System::String::Format( "{0}:{1}:7", campos[1], campos[2] ), nullptr );
				}
				if ( line == "ambience" )
				{
					// ambience
					consumidor->call_ssi( AC_CALL_LISTEN_AMBIENCE, System::String::Format( "{0}:{1}:7", campos[1], campos[2] ), nullptr );
				}
				if ( line == "pincha" )
				{
					consumidor->pincha_linea( System::String::Format( "{0}:1", campos[1] ) );
				}
				if ( line == "despincha" )
				{
					consumidor->pincha_linea( System::String::Format( "{0}:0", campos[1] ) );
				}
				if ( line == "pttn" )
				{
					consumidor->ptt( System::String::Format( "{0}:1:0", campos[1] ) );
				}
				if ( line == "pttf" )
				{
					consumidor->ptt( System::String::Format( "{0}:0:0", campos[1] ) );
				}
				if ( line == "cambiag" )
				{
					consumidor->cambia_grupo_asociado( System::String::Format( "{0}:{1}", campos[1], campos[2] ) );
				}
				if ( line == "add" )
				{
					consumidor->modifica_grupo( System::String::Format( "A:1:D:{0}:{1}" , campos[1], campos[2] ) );
				}
				if ( line == "del" )
				{
					consumidor->modifica_grupo( System::String::Format( "D:1:D:{0}:{1}" , campos[1], campos[2] ) );
				}
				if ( line == "clear" )
				{
					db->clear_pertenencias_all();
				}
				if ( line == "reset_registration" )
				{
					db->reset_register_status();
					consumidor->manager->avisaAcCore(System::String::Format( "{0}{1}:{2}:{3}{4}",TRAMA_INI, FRT_AVISA_CAMBIOS, T_TERMINALES, -1, TRAMA_END ) );
				}
				if ( line == "reset_attached" )
				{
					consumidor->reset_attached( System::Convert::ToInt32( campos[1] ) );					
				}
				//if ( line == "s" )
				//{
				//	data_ini->parse_nms_subscriber_xml( db );
				//}
				//if ( line == "p" )
				//{
				//	data_ini->parse_nms_groups_xml( db );
				//}
				if ( line == "db" )
				{
					db->fill_db();
				}
				if ( line == "dbp" )
				{
					db->fill_db_pertenencias();
				}
				if ( line == "t" )
				{
					consumidor->timer_dummy(nullptr);
				}
				if ( line == "mon" )
				{
					consumidor->modifica_item_mon( System::String::Format( "{0}:{1}:{2}", campos[1], campos[2], campos[3] ) );
				}
				if ( line == "listen" )
				{
					consumidor->escucha_item_mon( System::String::Format( "{0}:{1}:{2}", campos[1], campos[2], campos[3] ), nullptr );
				}
				if ( line == "reset" )
				{
					consumidor->reset_mon_items( System::String::Format( "{0}", campos[1] ) );
				}
				if ( line == "end" )
				{
					consumidor->force_end_mon_call( System::String::Format( "{0}:{1}", campos[1], campos[2] ) );
				}
				if ( line == "pause" )
				{
					Monitor::Enter( consumidor->permiso_continua_lock );
						consumidor->permiso_continua = ! consumidor->permiso_continua;
						if ( consumidor->permiso_continua )
							consumidor->continua->Set();
						logger->Write( System::String::Format( "permiso_continua {0}", consumidor->permiso_continua ), 1 );
					Monitor::Exit( consumidor->permiso_continua_lock );
				}
				if ( line == "check_nms" )
				{
					data_ini->check_xml_changes( db, consumidor );
				}
				if ( line == "logger" )
				{
					logger->warningLevel = System::Convert::ToInt32( campos[1] );
				}
				if ( line == "logger" )
				{
					logger->warningLevel = System::Convert::ToInt32( campos[1] );
				}
				if ( line == "exit" )
				{
					return 0;
				}
				// ------------------ SIMU --------------------------
				if ( line == "pincha_all" )
				{
					consumidor->pincha_all( System::Convert::ToInt32( campos[1] ) );
				}
				if ( line == "cuelga_all" )
				{
					consumidor->cuelga_all();
				}
				if ( line == "simu_calls" )
				{
					consumidor->simu_calls( System::Convert::ToInt32( campos[1] ), System::Convert::ToInt32( campos[2] ) );
				}


				// ------------------ LAME FR --------------------------
				if ( line == "fr_ch" )
				{
					consumidor->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}{4}",TRAMA_INI, LAME_FR_CAMB_CH, campos[1], campos[2], TRAMA_END ) );
				}
				if ( line == "fr_scan_on" )
				{
					consumidor->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}{4}",TRAMA_INI, LAME_FR_SCANNER, campos[1], campos[2], TRAMA_END ) );
				}
				if ( line == "fr_scan_off" )
				{
					consumidor->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}{4}",TRAMA_INI, LAME_FR_SCANNER, campos[1], campos[2], TRAMA_END ) );
				}
			}
			catch ( System::Exception^ e )
			{
				logger->Write( "Error parsing command line", 4 );
			}
			//consumidor->envia_sds("0:PEPE:2001:40:blablablablablablablablablablabla");
		}     
		 //System::Threading::Thread::Sleep(5000);
		 //SetCallBackPointer(System::Runtime::InteropServices::Marshal::GetFunctionPointerForDelegate(Consumidor::myDelegate));

		 //SetCallbackPointer

	  /*bool print_help = true;

	  int state = 0;
	  TetraInterface *user;

	  if(argc > 1)
	  {
	
		if(strcmp(argv[1], "help") == 0)
		{
		  help();
		}
		else if(strcmp(argv[1], "command") == 0)
		{
		  if(argc == 5)
		  {
			user = new TetraInterface(argv[2], atoi(argv[3]), argv[4]);
			user->commandLine();
			print_help = false;
		  }
		}
	  }
	  if (print_help)
	  {
		help();
	  }*/

		//simulaAll();


		return 0;
	}
	catch ( System::Exception^ e )
	{
		return 1;
	}

}

bool call_tetra_interface ( Config_data^ data_ini, TetraInterface* tetra )
{
	// Necesario para pasar strings a codigo no gestionado (tetrainterface)
	char acapi_user[20];
		memset( &acapi_user, 0, sizeof( acapi_user ) );
	char acapi_pwd[20];
		memset( &acapi_pwd, 0, sizeof( acapi_pwd ) );
	char acapi_ip[20];
		memset( &acapi_ip, 0, sizeof( acapi_ip ) );

	System::IntPtr p = Marshal::StringToHGlobalAnsi( data_ini->acapi_user );
	char *aux_char = static_cast<char*>( p.ToPointer() );
	strcat( acapi_user, aux_char );

	p = Marshal::StringToHGlobalAnsi( data_ini->acapi_pwd );
	aux_char = static_cast<char*>( p.ToPointer() );
	strcat( acapi_pwd, aux_char );

	p = Marshal::StringToHGlobalAnsi( data_ini->acapi_ip );
	aux_char = static_cast<char*>( p.ToPointer() );
	strcat( acapi_ip, aux_char );

	Marshal::FreeHGlobal(p);

	if( tetra->connect( ACAPI_DLL_KEY, acapi_ip, data_ini->acapi_debug_level, acapi_user, acapi_pwd ) != 1 )
	{
		return false;
	}
	else
	{
		return true;
	}
}
