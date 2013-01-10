#include "Config_data.h"

Config_data::Config_data()
{
	// hasta que no se compruebe que efectivamente se puede acceder a ellos
	this->xmls_accessible = false;
}

bool Config_data::parse_data ( System::String^ xml_document )
{
	try
	{
		this->config_xml_route = xml_document;
		XmlDocument^ xd = gcnew XmlDocument();
		xd->Load( xml_document );

		XmlNode^ node;

		node = xd->SelectSingleNode("/fr_tetra/logger");
		this->logger_txt = node->ChildNodes->Item(0)->InnerText;
		this->warning_level= System::Convert::ToInt32( node->ChildNodes->Item(1)->InnerText );

		node = xd->SelectSingleNode("/fr_tetra/db");
		this->db_dsn		= node->ChildNodes->Item(0)->InnerText;
		this->db_uid		= node->ChildNodes->Item(1)->InnerText;
		this->db_pwd		= node->ChildNodes->Item(2)->InnerText;

		node =  xd->SelectSingleNode("/fr_tetra/acapi");
		this->acapi_user = node->ChildNodes->Item(0)->InnerText;
		this->acapi_pwd = node->ChildNodes->Item(1)->InnerText;
		this->acapi_ip = node->ChildNodes->Item(2)->InnerText;
		this->acapi_app_ack_timeout_sec = System::Convert::ToInt32( node->ChildNodes->Item(3)->InnerText );

		node =  xd->SelectSingleNode("/fr_tetra/nms");
		this->nms_ip = System::Net::IPAddress::Parse( node->ChildNodes->Item(0)->InnerText );
		this->acapi_nms_xml_subscriber_route = System::String::Format( "\\\\{0}{1}",  this->nms_ip, node->ChildNodes->Item(1)->InnerText );
		this->acapi_nms_xml_groups_route = System::String::Format( "\\\\{0}{1}",  this->nms_ip, node->ChildNodes->Item(2)->InnerText );
		try
		{
			this->subscriber_xml_last_modified = System::DateTime::Parse( node->ChildNodes->Item(3)->InnerText );
			this->pertenencia_xml_last_modified = System::DateTime::Parse( node->ChildNodes->Item(4)->InnerText );
		}
		catch ( System::Exception^ e )
		{	// si alguna fecha está mal formateada
		}


		node =  xd->SelectSingleNode("/fr_tetra/asterisk");
		this->asterisk_ip = System::Net::IPAddress::Parse( node->ChildNodes->Item(0)->InnerText );

		//XmlNodeList^ lineas = xd->SelectNodes( "/fr_tetra/lineas/linea" );
		//this->lineas_ini = gcnew array<Linea_tetra^>(lineas->Count);
		//int i = 0;
		//for each( node in lineas )
		//{
		//	lineas_ini[i] = gcnew Linea_tetra();
		//	lineas_ini[i]->despachador		= System::Convert::ToBoolean( node->Attributes->GetNamedItem( "despachador" )->Value );
		//	lineas_ini[i]->ssi_linea		= System::Convert::ToInt32( node->Attributes->GetNamedItem( "ssi" )->Value );
		//	lineas_ini[i]->puerto_asterisk	= System::Convert::ToInt32( node->Attributes->GetNamedItem( "puerto_asterisk" )->Value );
		//	i ++;
		//}

		//node =  xd->SelectSingleNode("/fr_tetra/central");
		//this->central_gssi = System::Convert::ToInt32( node->ChildNodes->Item(0)->InnerText );

		node = xd->SelectSingleNode("/fr_tetra/retorno_al_grupo_defecto/enabled");
		this->change_group_to_default = System::Convert::ToBoolean( node->InnerText );

		node =  xd->SelectSingleNode("/fr_tetra/acCore");
		this->acCore_local_ip = node->ChildNodes->Item(0)->InnerText;;
		this->acCore_local_port = System::Convert::ToInt32( node->ChildNodes->Item(1)->InnerText );

		node =  xd->SelectSingleNode("/fr_tetra/global_monitoring");
		this->global_monitoring_begin_ssi = System::Convert::ToInt32( node->ChildNodes->Item(0)->InnerText );
		this->global_monitoring_end_ssi = System::Convert::ToInt32( node->ChildNodes->Item(1)->InnerText );
		this->global_monitoring_num_wait_iterations = System::Convert::ToInt32( node->ChildNodes->Item(2)->InnerText );

		node =  xd->SelectSingleNode("/fr_tetra/global_dgna");
		this->global_dgna_num_wait_iterations = System::Convert::ToInt32( node->ChildNodes->Item(0)->InnerText );
		this->global_dgna_num_max_retries = System::Convert::ToInt32( node->ChildNodes->Item(1)->InnerText );

		return true;
	}
	catch (System::Exception^ e)
	{
		return false;
	}
}

/*
	Salva en el archivo de configuracion la ultima fecha de modificacion de los archivos xmls
	De esta manera al arrancar el FR_Tetra se sabrá si ha habido algún cambio
*/
void Config_data::save_xmls_mod_date ( System::DateTime^ subscriber_xml_last_modified, System::DateTime^ pertenencia_xml_last_modified )
{
	try
	{
		XmlDocument^	xd = gcnew XmlDocument();
		xd->Load( this->config_xml_route );

		XmlNode^ node = xd->SelectSingleNode("/fr_tetra/nms");

		if ( subscriber_xml_last_modified != nullptr )
		{
			node->ChildNodes->Item(3)->InnerText = subscriber_xml_last_modified->ToString();
		}
		if ( pertenencia_xml_last_modified != nullptr )
		{
			node->ChildNodes->Item(4)->InnerText = pertenencia_xml_last_modified->ToString();
		}

		xd->Save( this->config_xml_route );
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "Problema en parse_data", 4 );
	}
}

///*
//
//*/
//bool Config_data::parse_nms_subscriber_xml ( Database^ db )
//{
//	try
//	{
//		array<Subscriber^>^ subscriber_list = this->read_subscribers();
//
//		db->update_subscribers( subscriber_list );
//		this->subscriber_list_last = subscriber_list;
//
//		return true;
//	}
//	catch ( System::Exception^ e )
//	{
//		return false;
//	}
//}

/*
PARAMETROS subscriber_1.xml
					genType		genIndCmcGroup	typeGroup
terminals			1			1				1
applications		7			1				1
static groups		2			2				1
static extendable	2			2				2
dgna				2			2				3
broadcast groups	2			2				2
*/
array<Subscriber^>^ Config_data::read_subscribers ( )
{
	try
	{
		XmlDocument^ xd = gcnew XmlDocument();
		xd->Load( this->acapi_nms_xml_subscriber_route );

		XmlNodeList^ subscriber_node_list = xd->SelectNodes( "/root/sdrTable" );
		XmlNode^ subscriber_node;
		array<Subscriber^>^ subscriber_list = gcnew array<Subscriber^>( subscriber_node_list->Count );
		bool grupo;
		System::Int32 ssi;
		System::String^ nombre;
		wchar_t tipo;

		System::Int32 genType;
		System::Int32 typeGroup;
		int i = 0;
		for each( subscriber_node in subscriber_node_list )
		{
			genType			= System::Convert::ToInt32( subscriber_node->Attributes->GetNamedItem( "genType" )->Value );
			typeGroup		= System::Convert::ToInt32( subscriber_node->Attributes->GetNamedItem( "typeGroup" )->Value );
			ssi				= System::Convert::ToInt32( subscriber_node->Attributes->GetNamedItem( "genIdxTsiSsi" )->Value );
			nombre			= subscriber_node->Attributes->GetNamedItem( "subscriberName" )->Value;
			switch ( genType )
			{
			case 1:		// terminal
				grupo = false;
				tipo = SUBSCRIBER_TERMINAL;
				break;
			case 7:		// application
				grupo = false;
				tipo = SUBSCRIBER_APP;
				break;
			case 2:		// group
				grupo = true;
				if( typeGroup == 1 )		// static
					tipo = SUBSCRIBER_GRUPO_ESTATICO;
				if( typeGroup == 2 )		// static dynamically extendable o Broadcast??!! por ahora extendable
					tipo = SUBSCRIBER_GRUPO_EXTENSIBLE;
				if( typeGroup == 3 )		// dgna
					tipo = SUBSCRIBER_GRUPO_DINAMICO;
				break;
			}
			subscriber_list[i] = gcnew Subscriber( grupo, ssi, nombre, tipo );
			i++;
		}

		return subscriber_list;
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "Problema en read_subscribers", 4 );
		return nullptr;
	}
}

///*
//*/
//bool Config_data::parse_nms_groups_xml ( Database^ db )
//{
//	try
//	{
//		array<Pertenencia^>^ pertenencia_list = this->read_pertenencias_estaticas();
//
//		db->update_pertenencias_estaticas( pertenencia_list );
//
//		this->pertenencia_list_last = pertenencia_list;
//
//		return true;
//	}
//	catch ( System::Exception^ e )
//	{
//		return false;
//	}
//}

/*
*/
array<Pertenencia^>^ Config_data::read_pertenencias_estaticas ( )
{
	try
	{
		XmlDocument^ xd = gcnew XmlDocument();
		xd->Load( this->acapi_nms_xml_groups_route );

		XmlNodeList^ pertenencia_node_list = xd->SelectNodes( "/root/tosgrp" );
		XmlNode^ pertenencia_node;
		array<Pertenencia^>^ pertenencia_list = gcnew array<Pertenencia^>( pertenencia_node_list->Count );

		System::Int32 genIdxTsiSsi;
		System::Int32 tosGroupShortSubscriberIdentity;
		int i = 0;
		for each( pertenencia_node in pertenencia_node_list )
		{
			genIdxTsiSsi						= System::Convert::ToInt32( pertenencia_node->Attributes->GetNamedItem( "genIdxTsiSsi" )->Value );
			tosGroupShortSubscriberIdentity		= System::Convert::ToInt32( pertenencia_node->Attributes->GetNamedItem( "tosGroupShortSubscriberIdentity" )->Value );

			pertenencia_list[i] = gcnew Pertenencia( tosGroupShortSubscriberIdentity, genIdxTsiSsi );
			i++;
		}

		return pertenencia_list;
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "Problema en read_pertenencias_estaticas", 4 );
		return nullptr;
	}
}

/*
	Esta rutina se ejecuta en un bucle infinito en un Thread aparte creado en main
	
	[
	se encontró un bug en visual studio que hacía que el ordenador se bloquease directamente con un pantallazo
	al hacer un ping y cerrar la sesión de debug.
	]

	Para evitar quedarse bloqueado intentando leer los archivos de red desde esta rutina se lanza a su vez
	un Thread distinto y se observa si su ejecucion tarda más de lo debido ( TIME_CHECK_XMLS_DESTROY_THREAD )
	si es así se supone que el NMS no es accesible y se intenta abortar la Thread, aunque si está bloqueada
	en la llamada "File::GetLastWriteTime" no abortará hasta que se desbloquee.

	Se comprueba que se puede acceder a las direcciones de red de los xmls del NMS
	Es importante porque si se pierde la conexión y se sigue intentando acceder a estos xmls para 
	detectar cambios habrá importantes retardos

	se modifica la variable "xmls_accessible" que servirá para chequear cambios o no en estos xmls
*/
void Config_data::check_nms_connection ( )
{	
	Thread^ try_open_xmls;
	System::DateTime ^before, ^after;
	System::Int64 duration_ms;

	while ( true )
	{
		// Consumidor comienza a desencolar y tratar eventos
		try_open_xmls = gcnew Thread( gcnew ThreadStart( this, &Config_data::try_open_nms_xmls ) );

		before = System::DateTime::Now;

		try_open_xmls->Start();
		try_open_xmls->Join( TIME_CHECK_XMLS_DESTROY_THREAD );

		after = System::DateTime::Now;
		duration_ms = ( after->Ticks - before->Ticks ) / 10000;

		// Si la Thread no ha terminado alrededor de "TIME_CHECK_XMLS_DESTROY_THREAD" ms quiere decir que no esta pudiendo acceder al recurso de red
		if ( duration_ms > ( TIME_CHECK_XMLS_DESTROY_THREAD - 10 ) )
		{
			if ( this->xmls_accessible )	// comunico la desconexion la primera vez
				this->logger->Write( "XMLs en NMS NO se pueden acceder!!!", 4 );
			this->xmls_accessible = false;
		}

		try_open_xmls->Abort( );
		try_open_xmls->Join();

		// Según el NMS esté alcanzable o no se esperará un tiempo distinto para volver a chequear el status
		if ( this->xmls_accessible )
			Thread::Sleep( TIME_CHECK_NMS_CONN );
		else
			Thread::Sleep( TIME_CHECK_NMS_NOT_CONN );
	}
}

/*
	La ejecucion de este código es rápida cuando se tiene acceso al path de archivo (decenas de milisegundos)
	Si el path se encuentra inaccesible en la red puede tardar mucho (decenas de segundos)
*/
void Config_data::try_open_nms_xmls ( )
{
	System::DateTime ^before, ^after;
	System::Int64 duration_ms;
	try
		{
			before = System::DateTime::Now;

			File::GetLastWriteTime( this->acapi_nms_xml_subscriber_route );
			File::GetLastWriteTime( this->acapi_nms_xml_groups_route );

			if ( ! this->xmls_accessible )	// comunico la reconexion la primera vez
				this->logger->Write( "XMLs en NMS SI se pueden acceder!!!", 1 );
			this->xmls_accessible = true;
		}
		catch( System::Exception^ e )
		{
			if ( this->xmls_accessible )	// comunico la perdida de conexion la primera vez
				this->logger->Write( "XMLs en NMS NO se pueden acceder!!!", 4 );
			this->xmls_accessible = false;
		}
		finally
		{
			after = System::DateTime::Now;
			duration_ms = ( after->Ticks - before->Ticks ) / 10000;
			this->logger->Write(  System::String::Format("____________check_nms_connection!!!   FIN en {0} ms", duration_ms ), -2 );
		}
}

/*
	Se comprobará primero si los archivos xml han sido modificados.
	Es posible que aunque hayan sido modificados el contenido sea el mismo. Por lo que 
		se comprabará su contenido. Cada vez que se leen los archivos se deja una copia
		de la información relevante en memoria. Si al comparar la información leída con la 
		previa almacenada se encuentran cambios se llamará a las rutinas de Database que
		actualizarán en consecuencia los registros añadidos, modificados o eliminados.

	En el primer check ( el programa arranca ), cuando las variables valen nullptr
*/
bool Config_data::check_xml_changes ( Database^ db, Consumidor^ consumidor )
{
	try
	{
		bool different_subscriber_file, different_pertenencias_file;
		bool update_subscribers, update_pertenencias;
		
		if ( this->xmls_accessible )
		{
			System::DateTime ^before, ^after;
			System::Int64 duration_ms;

			before = System::DateTime::Now;
			System::DateTime^ subscriber_xml_last_modified_new = File::GetLastWriteTime( this->acapi_nms_xml_subscriber_route );

			after = System::DateTime::Now;
			duration_ms = ( after->Ticks - before->Ticks ) / 10000;
			this->logger->Write( System::String::Format( "check_xml_changes [readLastWriteTime_subscriber] en {0} ms", duration_ms ), -1 );

			if ( this->subscriber_xml_last_modified == nullptr )		// No se pudo leer un dateTime apropiado del config.xml
			{
				different_subscriber_file = true;
			}
			else
			{
				// Comparo las String y no los valores porque la representacion String tiene precision de minutos
				different_subscriber_file = ! ( this->subscriber_xml_last_modified->ToString() == subscriber_xml_last_modified_new->ToString() );
			}


			before = System::DateTime::Now;
			System::DateTime^ pertenencia_xml_last_modified_new = File::GetLastWriteTime( this->acapi_nms_xml_groups_route );

			after = System::DateTime::Now;
			duration_ms = ( after->Ticks - before->Ticks ) / 10000;
			this->logger->Write( System::String::Format( "check_xml_changes [readLastWriteTime_pertenencias] en {0} ms", duration_ms ), -1 );

		
			if ( this->pertenencia_xml_last_modified == nullptr )		// No se pudo leer un dateTime apropiado del config.xml
			{
				different_pertenencias_file = true;
			}
			else
			{
				// Comparo las String y no los valores porque la representacion String tiene precision de minutos
				different_pertenencias_file = ! ( this->pertenencia_xml_last_modified->ToString() == pertenencia_xml_last_modified_new->ToString() );
			}

			if ( different_subscriber_file )
			{
				before = System::DateTime::Now;

				this->logger->Write( System::String::Format( "DIFFERENT SUBSCRIBER FILE last mod date: {0} new mod date {1}!!!", this->subscriber_xml_last_modified, subscriber_xml_last_modified_new ), 1 );
				this->subscriber_xml_last_modified = subscriber_xml_last_modified_new;
				this->save_xmls_mod_date( subscriber_xml_last_modified_new, nullptr );

				// -----------Se comprueban cambios en la ultima lectura de subscriptores------------------
				array<Subscriber^>^ subscriber_list_new = this->read_subscribers();

				if ( this->subscriber_list_last == nullptr )									// si es la primera vez que se leen
				{
					update_subscribers = true;
				}
				else if ( this->subscriber_list_last->Length != subscriber_list_new->Length )		// si tienen distinto tamaño
				{
					update_subscribers = true;
				}
				else
				{
					Subscriber ^_last, ^_new;
					for ( int i=0; i < subscriber_list_new->Length; i++ )							// si hay algun subscriber distinto
					{
						_last = this->subscriber_list_last[i];
						_new = subscriber_list_new[i];

						if (
							_last->grupo	!= _new->grupo	||
							_last->ssi		!= _new->ssi	||
							_last->nombre	!= _new->nombre	||
							_last->tipo		!= _new->tipo
							)
						{
							update_subscribers = true;
							break;							// Con detectar una única diferencia nos basta
						}
					}
				}

				// actualizo los subscribers si se ha detectado algún cambio
				if ( update_subscribers )
				{
					if ( db->update_subscribers( subscriber_list_new ) )
						this->subscriber_list_last = subscriber_list_new;

					consumidor->manager->avisaAcCore(System::String::Format( "{0}{1}:{2}:{3}{4}",TRAMA_INI, FRT_AVISA_CAMBIOS, T_TERMINALES, -1, TRAMA_END ) );
				}
				else
				{
					this->logger->Write( "SUBSCRIBER FILE SIN CAMBIOS", 2 );
				}

				after = System::DateTime::Now;
				duration_ms = ( after->Ticks - before->Ticks ) / 10000;
				this->logger->Write( System::String::Format( "check_xml_changes [different subscriber file] en {0} ms", duration_ms ), 1 );
			}

			if ( different_pertenencias_file )
			{
				before = System::DateTime::Now;

				this->logger->Write( System::String::Format( "DIFFERENT PERTENCIAS FILE last mod date: {0} new mod date {1}!!!", this->pertenencia_xml_last_modified, pertenencia_xml_last_modified_new ), 1 );
				this->pertenencia_xml_last_modified = pertenencia_xml_last_modified_new;

				this->save_xmls_mod_date( nullptr, pertenencia_xml_last_modified_new );

				// -----------Se comprueban cambios en la ultima lectura de pertenencias estáticas ------------------

				array<Pertenencia^>^ pertenencia_list_new = this->read_pertenencias_estaticas();

				if ( this->pertenencia_list_last == nullptr )									// si es la primera vez que se leen
				{
					update_pertenencias = true;
				}
				else if ( this->pertenencia_list_last->Length != pertenencia_list_new->Length )		// si tienen distinto tamaño
				{
					update_pertenencias = true;
				}
				else
				{
					Pertenencia ^_last, ^_new;
					for ( int i=0; i < pertenencia_list_new->Length; i++ )							// si hay algun subscriber distinto
					{
						_last = this->pertenencia_list_last[i];
						_new = pertenencia_list_new[i];

						if (
							_last->g_ssi	!= _new->g_ssi	||
							_last->t_ssi	!= _new->t_ssi
							)
						{
							update_pertenencias = true;
							break;							// Con detectar una única diferencia nos basta
						}
					}
				}

				// actualizo las pertenencias estaticas si se ha detectado algún cambio
				if ( update_pertenencias )
				{
					if ( db->update_pertenencias_estaticas( pertenencia_list_new ) )
						this->pertenencia_list_last = pertenencia_list_new;

					consumidor->manager->avisaAcCore(System::String::Format( "{0}{1}:{2}:{3}{4}",TRAMA_INI, FRT_AVISA_CAMBIOS, T_PERTENENCIAS, -1, TRAMA_END ) );
				}
				else
				{
					this->logger->Write( "PERTENENCIAS FILE SIN CAMBIOS", 2 );
				}

				after = System::DateTime::Now;
				duration_ms = ( after->Ticks - before->Ticks ) / 10000;
				this->logger->Write( System::String::Format( "check_xml_changes [different pertenencias file] en {0} ms", duration_ms ), 1 );
			}
			return true;
		}
		else
		{
			//no se tenia acceso a los archivos xmls
			return false;
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "Problema en check_xml_changes", 4 );
		return false;
	}
}