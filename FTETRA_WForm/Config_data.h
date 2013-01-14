#ifndef CONFIG_DATA_H
#define CONFIG_DATA_H

#include "Consumidor.h"
#include "Database.h"
#include "CustomLogger.h"


#using <System.Xml.dll>
#using <System.dll>
#using <System.Data.dll>

#define TIME_CHECK_XMLS_DESTROY_THREAD		10000
#define TIME_CHECK_NMS_CONN					4000
#define TIME_CHECK_NMS_NOT_CONN				4000 //120000		// cuando ya se ha detectado que el NMS está desconectado se esperará mayor tiempo para volver a chequear el estado

using namespace System::Xml;

ref class Database;
ref class Pertenencia;
ref class Subscriber;
ref class CustomLogger;
ref class Consumidor;

ref class Config_data
{
public:
	Config_data();

	bool parse_data (System::String^ xml_document);

	array<Pertenencia^>^ read_pertenencias_estaticas ();
	array<Subscriber^>^ read_subscribers ();

	//bool parse_nms_subscriber_xml ( Database^ db );
	//bool parse_nms_groups_xml ( Database^ db );

	void check_nms_connection ( );
	void try_open_nms_xmls ( );
	bool check_xml_changes ( Database^ db, Consumidor^ consumidor );

	void save_xmls_mod_date ( System::DateTime^ subscriber_xml_last_modified, System::DateTime^ pertenencia_xml_last_modified );

	System::String^ config_xml_route;

	System::String^	logger_txt;
	System::String^	name_folder;
	System::Int32	warning_level;

	System::String^	db_dsn;
	System::String^	db_uid;
	System::String^	db_pwd;
	//bool			db_enabled;

	System::String^	acapi_license_key;
	System::String^	acapi_user;
	System::String^	acapi_pwd;
	System::String^	acapi_ip;
	System::Int32	acapi_debug_level;
	System::Int32	acapi_app_ack_timeout_sec;
	System::String^ acapi_nms_xml_subscriber_route;
	System::String^ acapi_nms_xml_groups_route;

	System::Net::IPAddress^ asterisk_ip;

	//array<Linea_tetra^>^	lineas_ini;


	//System::Int32	central_gssi;
	bool	change_group_to_default;

	System::String^ acCore_local_ip;
	System::Int32 acCore_local_port;

	System::Int32 global_monitoring_begin_ssi;
	System::Int32 global_monitoring_end_ssi;
	System::Int32 global_monitoring_num_wait_iterations;

	System::Int32 global_dgna_num_wait_iterations;
	System::Int32 global_dgna_num_max_retries;


	array<Subscriber^>^		subscriber_list_last;
	System::DateTime^		subscriber_xml_last_modified;
	array<Pertenencia^>^	pertenencia_list_last;
	System::DateTime^		pertenencia_xml_last_modified;

	bool					xmls_accessible;				// variable que valdrá false en caso de no poder acceder a los xmls del NMS
	System::Net::IPAddress^ nms_ip;

	CustomLogger^			logger;
};
#endif
