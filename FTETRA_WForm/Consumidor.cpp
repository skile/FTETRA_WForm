#include "Consumidor.h"


Consumidor::Consumidor ()
{
	this->cola = gcnew ConcurrentQueue<EventoCola^>();
	this->myDelegate = gcnew CallBackDelegate(this, &Consumidor::ManagedAcapiCallback);

	this->continua = gcnew AutoResetEvent(false);
	
	this->permisoAvisoDgna = true;
	this->last_dgna_group = -1;

	this->conexion_acapi_ok = false;

	this->mon_reg_handle = -1;

	this->num_timer_iter = 0;

	this->lineas_lock = gcnew System::Object();

	this->permiso_continua_lock = gcnew System::Object();
	this->permiso_continua = true;
}

void Consumidor::proceso_escucha_rtp( bool arranca, System::Int32 index_linea )
{
	try
	{
		/*ProcessStartInfo^ info;
		if ( vlc_emite == nullptr || vlc_escucha == nullptr )
		{
			vlc_emite	= gcnew array<Process^>( this->lineas->Length );
			vlc_escucha	= gcnew array<Process^>( this->lineas->Length );
		}

		if ( arranca )
		{
			if ( vlc_emite[index_linea] == nullptr )
			{
				info = gcnew ProcessStartInfo( "C:\\SICOM\\SICOM\\FrontTetra\\A-CAPI\\CC\\ffmpeg\\bin\\ffmpeg.exe", 
					System::String::Format( "-f dshow -i audio=\"micro (Realtek High Definition \" -ac 1 -ar 8000 -acodec pcm_mulaw -ab 64k -f rtp rtp://{0}:{1}?pkt_size=252",
					this->lineas[index_linea]->comunicacion->acapi_ip, this->lineas[index_linea]->comunicacion->acapi_rtp_port ) );
				vlc_emite[index_linea] = Process::Start( info );
			}
			if ( vlc_escucha[index_linea] == nullptr )
			{
				info = gcnew ProcessStartInfo( "C:\\Program Files\\VideoLAN\\VLC\\vlc.exe", System::String::Format( "rtp://@:{0}", this->lineas[index_linea]->puerto_asterisk ) );
				vlc_escucha[index_linea] = Process::Start( info );
			}
		}
		else
		{
			vlc_emite[index_linea]->Kill();
			vlc_escucha[index_linea]->Kill();
			vlc_emite[index_linea]		= nullptr;
			vlc_escucha[index_linea]	= nullptr;
		}*/
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "Problema en proceso_escucha_rtp !!!!!!", 4 );
	}
}

/*
	Rellena el array lineas adecuadamente.
*/
bool Consumidor::genera_lineas(  )
{
	try
	{
		// De la tabla Radios se lee la informacion de linea: activa-ssi_linea-despachador-puerto_asterix-ssi_grupo_defecto
		Generic::List<Linea_tetra^>^ lineas_leidas_lame = this->db->read_lines_info();

		if ( lineas_leidas_lame == nullptr )
			throw gcnew System::Exception();

		this->index_first_oper = -1;
		this->index_first_disp = -1;

		this->lineas = gcnew array<Linea_tetra^>( lineas_leidas_lame->Count );
		
		for ( int i = 0; i < this->lineas->Length; i ++ )
		{
			this->lineas[i] = lineas_leidas_lame[i];
			if ( this->lineas[i]->activa && ( ! this->lineas[i]->despachador ) && ( this->index_first_oper == -1 ) )
				this->index_first_oper = i;
			if ( this->lineas[i]->activa && this->lineas[i]->despachador && this->index_first_disp == -1 )
				this->index_first_disp = i;
			this->lineas[i]->cc = gcnew Servicio_tetra( false, -1 );
			this->lineas[i]->comunicacion = gcnew Comunicacion_tetra( -1, -1, -1 );

			this->lineas[i]->sds = gcnew Servicio_tetra( false, -1 );
			this->lineas[i]->ss = gcnew Servicio_tetra( false, -1 );

			this->lineas[i]->mon = gcnew Servicio_tetra( false, -1 );
			if ( this->lineas[i]->despachador )		// Si la linea es despachadora podrá realizar monitorizaciones arbitrarias
				this->lineas[i]->monitorizaciones = gcnew Generic::List<Monitorizacion_item^>();
			
			this->lineas[i]->attached = false;
			this->lineas[i]->ssi_grupo_asociado = -1;
		}

		if ( this->index_first_oper == -1 )
			this->logger->Write( "    genera_lineas, no hay ningún OPERADOR  !!!!!!", 4 );		// No levanto excepcion, permito funcionar sin operadores pero no sin despachadores
		if ( this->index_first_disp == -1 )
		{
			this->logger->Write( "    genera_lineas, no hay ningún despachador  !!!!!!", 5 );
			throw gcnew System::Exception();
		}
		return true;
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "          Problema generando lineas!!!!!!", 5 );
		return false;
	}
}

/*
	Comprobación periódica del estado general del sistema
*/
void Consumidor::timer_check ( System::Object^ objeto )
{
	// check AcCore socket !!
	// check dbConnection  !!
	try
	{
		Monitor::Enter( this->lineas_lock );
		Monitor::Enter( this->permiso_continua_lock );

		System::DateTime ^before, ^after, ^ini, ^end;
		System::Int64 duration_ms;

		if ( this->permiso_continua && this->conexion_acapi_ok )	// Si no se tiene permiso o no se está conectado no se hará ninguna rutina periódica
		{
		
			ini = System::DateTime::Now;
			
			if ( this->num_timer_iter % NO_ITER_GLOBAL_ROUTINE == 0 )
			{
				// Se comprueba la conexión de los servicios de todas las líneas
				this->check_connection();

				// Se comprueban terminales pendientes de ser borrados o añadidos a grupos
				this->logger->Write(  "check_pertenencias INI", -1 );
				before = System::DateTime::Now;
		
				this->call_check_pertenencias();

				after = System::DateTime::Now;
				duration_ms = ( after->Ticks - before->Ticks ) / 10000;
				this->logger->Write( System::String::Format( "check_pertenencias FIN en {0} ms", duration_ms ), -1 );

				// Se comprueban pertenencias aplicacion pendientes de ser attached o detached
				this->logger->Write(  "check_attached INI", -1 );
				before = System::DateTime::Now;

				this->call_check_attached();

				after = System::DateTime::Now;
				duration_ms = ( after->Ticks - before->Ticks ) / 10000;
				this->logger->Write( System::String::Format( "check_attached FIN en {0} ms", duration_ms ), -1 );
			}

			if ( this->num_timer_iter % NO_ITER_CHECK_REG_MON == 0 )
			{
				// Se comprueba que el servicio de monitorización de registro de terminales esté activo y además si habilitar el aviso al acCore de eventos de monitorización para no saturar con el initial briefing
				this->check_mon_reg();
			}

			// Se comprueba si se ha de avisar al acCore de una asignación a algún grupo DGNA reciente
			this->check_logica_aviso_dgna();

			// keep Alive!!!
			if ( this->num_timer_iter % NO_ITER_KEEP_ALIVE == 0 )
			{
				//this->manager->avisaAcCore( System::String::Format( "{0}{1}{2}", TRAMA_INI, KEEP_ALIVE, TRAMA_END  ) );
			}
			
			if ( this->num_timer_iter % NO_ITER_CHECK_NMS_XML == 0 )
			{
				before = System::DateTime::Now;

				this->read_data->check_xml_changes( this->db, this );

				after = System::DateTime::Now;
				duration_ms = ( after->Ticks - before->Ticks ) / 10000;
				this->logger->Write( System::String::Format( "check_xml_changes en {0} ms", duration_ms ), -1 );
			}
			

			end = System::DateTime::Now;
			duration_ms = ( end->Ticks - ini->Ticks ) / 10000;
			this->logger->Write( System::String::Format( "timer check en {0} ms", duration_ms ), -10 );

			this->num_timer_iter ++;
			if ( this->num_timer_iter == 10000 )
			{
				this->num_timer_iter = 0;
			}
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "error en timer_check !! !! !!", 4 );
	}
	finally
	{
		Monitor::Exit( this->permiso_continua_lock );
		Monitor::Exit( this->lineas_lock );
	}
}

/*
	Prueba para mantener sliding window open. Mandar un sds lame cada cierto tiempo.
	Mandamos un sds falso a un ssi que no existe con el objetivo de mantener el flujo de datos de aplicacion abierto!!
		NO ES OPTIMO Y HABRIA QUE SOLUCIONARLO, AMPLIAR APP window SIZE en el servidor ACAPI!!!
*/
void Consumidor::timer_dummy ( System::Object^ objeto )
{
	try
	{
		tetra->send_sds( this->lineas[this->index_first_oper]->sds->handle, 0, " ", 1, "100000", true );
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "error en timer_dummy !! !! !!", 4 );
	}
}

/*
	Se comprobará todos los servicios aplicables para cada línea
		-Todas las líneas tendrán activados los servicios de CC y SS (para poder ser añadidas a DGNA y unirse a los grupos asociados de cada una) Y SDS
		-La primera linea de operador mandará SDS en nombre de los operadores
		-La primera linea de operador mandará SDS en nombre de los despachadores
		-Todas las lineas de despachadores tendrán los servicios de monitorización habilitados. Será la primera línea de despachadores el
			servicio mon que se encargue del registro/desregistro de los terminales del sistema. Y el servicio SS de esta misma línea la que
			que haga las gestiones DGNA para todos los usuarios.
	
	La comprobación consiste en ver si el estado_ok del servicio es false, en cuyo caso se intenta conectar si aplica (según lo dicho previamente).
	Al dar la orden de conexion se actualiza el handle del servicio, que servirá para mapear los callbacks de tetra y ejecutar ordenes sobre estos.

	Además existe una lógica de timeout, se trackea el momento en el que se intenta conectar el servicio, no se volverá a intentar conectar el servicio
	hasta que se haya obtenido una respuesta (positiva o negativa) o haya pasado un timeout configurado en config.xml
	De esta manera se evita intentar conectar servicios que aún no han respondido pero si tendrían una respuesta positiva.

	MODIF: LAS LINEAS NO ACTIVAS NO TENDRÁN SERVICIOS CONECTADOS!
*/
void Consumidor::check_connection()
{
	try
	{
		Linea_tetra^ linea;
		System::DateTime^ t_timeout;
		bool timed_out;
		for ( int i = 0; i < this->lineas->Length; i ++ )
		{
			linea = this->lineas[i];
			if ( linea->activa )
			{
				// -------------------------- CC check --------------------------
				if ( linea->cc->t_pregunta != nullptr )
				{
					t_timeout = linea->cc->t_pregunta->Add( this->app_ack_timeout );
					timed_out =  ( (t_timeout)->CompareTo( System::DateTime::Now ) ) < 0;
				}
				else
					timed_out = false;

				if ( ( ( ! linea->cc->estado_ok ) && ( ( linea->cc->t_pregunta == nullptr ) ) ) || timed_out )
				{
					linea->cc->handle = this->tetra->connect_service( CC_APP, linea->ssi_linea );
					linea->cc->t_pregunta = System::DateTime::Now;
					this->logger->Write( System::String::Format( "PREGUNTA CC {0} timedOut? {1}", linea->ssi_linea, timed_out ) , 1 );
				}

				// -------------------------- SDS check --------------------------
				if ( linea->sds->t_pregunta != nullptr )
				{
					t_timeout = linea->sds->t_pregunta->Add( this->app_ack_timeout );
					timed_out =  ( (t_timeout)->CompareTo( System::DateTime::Now ) ) < 0;
				}
				else
					timed_out = false;

				// el primer ssi de linea operador será el encargado de recibir y enviar todos los sds de operador
				if ( ( ( ! linea->sds->estado_ok ) && ( linea->sds->t_pregunta == nullptr ) ) || timed_out )
				{
					linea->sds->handle = this->tetra->connect_service( SDS_APP, linea->ssi_linea );
					linea->sds->t_pregunta = System::DateTime::Now;
					this->logger->Write( System::String::Format( "PREGUNTA SDS {0} timedOut? {1}", linea->ssi_linea, timed_out ) , 1 );
				}

				// -------------------------- SS check --------------------------
				if ( linea->ss->t_pregunta != nullptr )
				{
					t_timeout = linea->ss->t_pregunta->Add( this->app_ack_timeout );
					timed_out =  ( (t_timeout)->CompareTo( System::DateTime::Now ) ) < 0;
				}
				else
					timed_out = false;

				// registramos el servicio SS para todas las lineas para poder ser agregadas a grupos
				if ( ( ( ! linea->ss->estado_ok ) && ( linea->ss->t_pregunta == nullptr ) ) || timed_out )
				{
					linea->ss->handle = this->tetra->connect_service( SS_APP, linea->ssi_linea );
					linea->ss->t_pregunta = System::DateTime::Now;
					this->logger->Write( System::String::Format( "PREGUNTA SS {0} timedOut? {1}", linea->ssi_linea, timed_out ) , 1 );
				}

				// -------------------------- MON check -------------------------
				if ( linea->mon->t_pregunta != nullptr )
				{
					t_timeout = linea->mon->t_pregunta->Add( this->app_ack_timeout );
					timed_out =  ( (t_timeout)->CompareTo( System::DateTime::Now ) ) < 0;
				}
				else
					timed_out = false;

				// el primer ssi de linea despachador será el encargado de DGNA y monitorización de registro y desregistro
				if ( ( ( ! linea->mon->estado_ok ) && ( linea->mon->t_pregunta == nullptr ) && ( linea->despachador ) ) || timed_out )
				{
					linea->mon->handle = this->tetra->connect_service( MON_APP, linea->ssi_linea );
					linea->mon->t_pregunta = System::DateTime::Now;
					// Si es el primer despachador habrá que reiniciar "mon_reg_handle", que indica la orden de monitorizacion de registro de terminales
					if ( i == this->index_first_disp )
					{
						this->mon_reg_handle = -1;
						this->logger->Write( "Reinicio handle de monitorización, servicio caido" , 2 );
					}
					this->logger->Write( System::String::Format( "PREGUNTA MON {0} timedOut? {1}", linea->ssi_linea, timed_out ) , 1 );
				}
			}
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "Problema checking connection", 4 );
	}
}

/*
	evento comunicado por el tcpManager cuando conecta con el AcCore
*/
void Consumidor::manager_connected ()
{
	try
	{
		// comunica el estado attached de todas las lineas
		this->manager->avisaAcCore( System::String::Format( "{0}{1}{2}",TRAMA_INI, CONN_AC_EST, TRAMA_END ) );

		for ( int i = 0; i < this->lineas->Length; i ++ )
		{
			if ( this->lineas[i]->activa )
			{
				this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}:{4}{5}",
								TRAMA_INI, CAMBIA_GRUPO_LINEA_ACK, i, this->lineas[i]->ssi_grupo_asociado,
								this->lineas[i]->attached ? 1 : 0, TRAMA_END ) );

				// Comunico todas los items monitorizados por cada despachador
				if ( this->lineas[i]->despachador )
				{
					for ( int j = 0; j < this->lineas[i]->monitorizaciones->Count; j ++ )
					{
						this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}:{4}{5}",
								TRAMA_INI, AC_MOD_MON_SSI_ACK, this->lineas[i]->ssi_linea, this->lineas[i]->monitorizaciones[j]->ssi_mon, 1, TRAMA_END ) );
					}
				}
			}
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "error en CONSUMIDOR::manager_connected", 4 );
	}
}

/*
	Si existe conexion al servidor acapi y el servicio de monitorizacion de la primera linea despachadora esta ok (estado_ok)
	se registra la monitorización de terminales. Si ya está registrada se esperan "mon_num_iter_espera" ciclos
	de timer para avisar al acCore
*/
void Consumidor::check_mon_reg()
{
	try
	{
		if ( this->conexion_acapi_ok && this->lineas[this->index_first_disp]->mon->estado_ok )
		{
			if ( this->mon_reg_handle == -1 )
			{	// servicio no registrado. No se puede avisar
				this->logger->Write( "-------MONITORIZACION terminales no registrada. Se resetea la información de registro", 2 );
				// se resetea la informacion de registro
				this->db->reset_register_status();
				// Se manda la orden de monitorizar a los terminales desde el servicio de monitorizacion de la primera linea despachadora (index_first_disp). 3-reg with initial briefing, 0-no sds mon, 0-no cc mon
				if ( ! this->tetra->monitoring_orderreq_range( this->lineas[this->index_first_disp]->mon->handle, 2, 0, 0, this->mon_ssi_ini, this->mon_ssi_end ) )
				//if ( ! this->tetra->monitoring_order( this->lineas[this->index_first_disp]->mon->handle, this->mon_ssi_ini, this->mon_ssi_end, 1 ) )
					this->logger->Write( "------- fallo mandando orden de monitorizacion al servidor acapi", 4 );
				this->mon_espera = 0;
				this->permisoAvisoMon = false;
			}
			else
			{	// servicio registrado, se puede avisar?
				if ( this->mon_espera == 0 )
					this->logger->Write( "-------espera briefing iniciada", 1 );
				if ( this->mon_espera == this->mon_num_iter_espera )
				{
					this->mon_espera++;
					// tras la espera aviso al AcCore que refresque toda la tabla (-1) 
					this->manager->avisaAcCore(System::String::Format( "{0}{1}:{2}:{3}{4}",TRAMA_INI, FRT_AVISA_CAMBIOS, T_TERMINALES, -1, TRAMA_END ) );
				}
				if ( this->mon_espera <= this->mon_num_iter_espera )
				{
					this->mon_espera++;
				}
				else
				{
					this->permisoAvisoMon = true;
				}
				
			}
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "error en check_mon_reg !! !! !!", 4 );
	}
}

void Consumidor::call_check_pertenencias()
{
	try
	{
		this->db->check_pertenencias( this, this->tetra );
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "error en call_check_pertenencias !! !! !!", 4 );
	}
}

void Consumidor::call_check_attached()
{
	try
	{
		this->db->check_attached( this, this->tetra );
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "error en call_check_attached !! !! !!", 4 );
	}
}

/*
	Cada vez que se establezca conexion ( servicios abran por primera vez ) habrá que hacer attach a cada linea con su grupo asociado
	Para hacer un reset general de todas las lineas habrá que pasar un '-1' como parámetro en index_linea

	Se puede dar la situación de que una línea pierda aisladamente su servicio SS => se deberá attach otra vez cuando lo recupere
		en este caso el parámetro index_linea tendrá el valor de la línea a la que esto ocurra
*/
void Consumidor::reset_attached( System::Int32 index_linea )
{
	bool existe_pertenencia;
	try
	{
		if ( index_linea == -1 )
			this->db->clear_attached();		// si reset todas limpio de golpe todos los estados attached de la base de datos

		for ( int i = 0; i < this->lineas->Length; i ++ )
		{
			// la linea tiene que estar activa y además (o lo realizo para todas las lineas o solo para la que coincida con 'index_linea')
			if ( this->lineas[i]->activa && ( (  index_linea == -1 ) || ( i == index_linea) ) )
			{
				this->lineas[i]->attached = false;
								
				//Si se pierde la conexión se resetean los grupos asociados si change_group_to_default == true
				if ( this->change_group_to_default )
				{
					this->lineas[i]->ssi_grupo_asociado = this->lineas[i]->ssi_grupo_defecto;
				}
				else
				{
					if ( this->lineas[i]->ssi_grupo_asociado == -1 )
						this->lineas[i]->ssi_grupo_asociado = this->lineas[i]->ssi_grupo_defecto;
				}

				existe_pertenencia = false;
				existe_pertenencia= this->db->update_attached(
											this->lineas[i]->ssi_grupo_asociado,
											this->lineas[i]->ssi_linea,
											PER_ATTACH_PENDING
															);
				if ( ! existe_pertenencia )
				{
					this->logger->Write( System::String::Format( "grupos asociado {0} de linea {1} no incluye al ssi_linea {2}",
										this->lineas[i]->ssi_grupo_asociado, i,this->lineas[i]->ssi_linea ), 2 );
					this->lineas[i]->ssi_grupo_asociado = -1;
				}
			}
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "error en reset_attached", 4 );
	}
}

/*
	Aviso de cambios en grupo tras haber pasado "dgna_num_iter_espera" iteraciones del timer
	ver "logica_aviso_mod_dgna"
*/
void Consumidor::check_logica_aviso_dgna()
{
	try
	{
		if ( ! this->permisoAvisoDgna )
		{
			this->dgna_espera ++;
			if ( this->dgna_espera > this->dgna_num_iter_espera )
			{
				this->permisoAvisoDgna = true;
				this->dgna_espera = 0;
				this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}{4}", TRAMA_INI, FRT_AVISA_CAMBIOS, T_PERTENENCIAS, this->last_dgna_group, TRAMA_END ) );
				this->last_dgna_group = -1;
			}
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "error en check_logica_aviso_dgna !! !! !!", 4 );
	}
}

System::IntPtr Consumidor::callbackIntPtr()
{
	System::IntPtr pointer = Marshal::GetFunctionPointerForDelegate(this->myDelegate);
	return pointer;
}

/*
Esta llamada se hace desde tetrainterface, para tratar los callbacks de la ACAPI.dll
*/
void Consumidor::ManagedAcapiCallback(int tipo, std::string contenido)
{
	EventoCola^ eventoAcapi = gcnew EventoCola();
	eventoAcapi->tipo = tipo;
	eventoAcapi->contenido = (gcnew System::String(contenido.c_str()));
	this->cola->Enqueue(eventoAcapi);

	if( this->permiso_continua )	// Solo continuaré consumiendo si hay permiso
		this->continua->Set();
}

/*
	Este método es el encargado de desencolar eventos y mandar a tratarlos.
	Los eventos se encolan desde el TcpManager (ordenes del acCore ) y desde el TetraInterface( callbacks acapi)
	Además se pueden encolar eventos desde el propio consumidor con un tiempo de expiración.
	Con la idea de tratar eventos que necesiten de alguna condicion para ser procesados. Por ejemplo una orden de llamar a un ssi,
	deberá ejecutarse solo si la linea está colgada. Habrá que mandar la orden colgar pero hasta que no se reciba confirmación no se debería procesar.
*/
void Consumidor::trabaja()
{
	try
	{
		EventoCola^ evento = nullptr;
		bool expirado;
		while( true )
		{
			if( this->cola->TryDequeue( evento ) )
			{	// Si ha expirado no se ejecutará!
				Monitor::Enter( this->lineas_lock );		// Lock que impide que haya incongruencias en lineas modificadas desde distintas hebras ( timer y consumidor leen/modifican las lineas )
				if ( evento->expira != nullptr )
				{
					expirado = evento->expira->CompareTo( System::DateTime::Now ) <= 0;
				}
				else
				{
					expirado = false;
				}

				if ( ! expirado )
					this->trataEvento( evento );
				else
					this->logger->Write( "EVENTO EXPIRADO!!!!", 5 );
					
				Monitor::Exit( this->lineas_lock );
			}
			else
			{
				this->logger->Write( "cola vacía, espero", -1 );
				this->continua->WaitOne();
				this->logger->Write( "continua signalled, continuo", -1 );
			}
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "fatal error en Consumidor::trabaja", 5 );
	}
}

/*
	Devuelve el indice de la linea buscando por servicio-handle
*/
int Consumidor::find_index_by_handle( int handle, int app_num )
{
	try
	{
		for ( int i = 0; i < this->lineas->Length; i ++ )
		{
			switch ( app_num )
			{
				case CC_APP:
					if ( this->lineas[i]->cc->handle == handle )
						return i;
					break;
				case SDS_APP:
					if ( this->lineas[i]->sds->handle == handle )
						return i;
					break;
				case SS_APP:
					if ( this->lineas[i]->ss->handle == handle )
						return i;
					break;
				case MON_APP:
					if ( this->lineas[i]->mon->handle == handle )
						return i;
					break;
			}
		}
		this->logger->Write( "Problema en find_index_by_handle (indice no encontrado!!)", 4 );
		return -1;
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "Problema en find find_index_by_handle", 4 );
		return -1;
	}
}

/*
	Devuelve el indice de la linea buscando por ssi_linea
*/
int Consumidor::find_index_by_ssi( int ssi_linea )
{
	try
	{
		for ( int i = 0; i < this->lineas->Length; i ++ )
		{
			if ( this->lineas[i]->ssi_linea == ssi_linea )
				return i;
		}
		this->logger->Write( "Problema en find_index_by_ssi (indice no encontrado!!)", 4 );
		return -1;
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "Problema en find find_index_by_ssi", 4 );
		return -1;
	}
}

int Consumidor::find_mon_index_by_ssi( int index_linea, int mon_ssi )
{
	try
	{
		for ( int i = 0; i < this->lineas[index_linea]->monitorizaciones->Count; i ++ )
		{
			if ( this->lineas[index_linea]->monitorizaciones[i]->ssi_mon == mon_ssi )
				return i;
		}
		this->logger->Write( "Problema en find_mon_index_by_ssi (mon_ssi no encontrado!!)", 4 );
		return -1;
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "Problema en find find_mon_index_by_ssi", 4 );
		return -1;
	}
}

int Consumidor::find_mon_index_by_handle( int index_linea, int mon_handle )
{
	try
	{
		for ( int i = 0; i < this->lineas[index_linea]->monitorizaciones->Count; i ++ )
		{
			if ( this->lineas[index_linea]->monitorizaciones[i]->mon_handle == mon_handle )
				return i;
		}
		this->logger->Write( "Problema en find_mon_index_by_handle (mon_handle no encontrado!!)", 4 );
		return -1;
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "Problema en find find_mon_index_by_handle", 4 );
		return -1;
	}
}

void Consumidor::trataEvento(EventoCola^ evento)
{
	switch(evento->tipo)	//decodificacion según la estructura de los mensaje callback. Mirar documentacion DLL para aquellos casos < 100
	{
		case 3:		// connection state
			this->logger->Write( System::String::Format( "trataEvento: connection_state: {0}", evento->contenido ), -1 );
			this->connection_state_callback( evento->contenido );
			break;
		case 4:		// sds_service_connection_state
		case 5:		// mon_service_connection_state
		case 6:		// supplementary_service_connection_state
		case 27:	// cc_service_connection_state
			this->logger->Write( System::String::Format( "trataEvento: service_state: {0}", evento->contenido ), -1 );
			this->service_state_callback( evento->tipo, evento->contenido );
			break;
		case 0:		// sds_data
		case 1:		// sds_status
			this->logger->Write( System::String::Format( "trataEvento: sds_callback = {0}", evento->contenido), 6 );
			this->sds_callback( evento->tipo, evento->contenido );
			break;
		case 2:		// sds_ack
			this->logger->Write( "NO SE TRATA trataEvento: sds_ack", -1 );
			break;
		case 11:	// mon_close
		case 22:	// mon_monitoring_ack
		case 23:	// mon_close_ack
			this->logger->Write( System::String::Format( "trataEvento monitorizacion depreciado!! (key, c_msg): ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->mon_status_callback( evento->tipo, evento->contenido );
			break;
		case 12:	// mon_location_update
		case 13:	// mon_location_detach
			this->logger->Write( System::String::Format( "trataEvento: mon_register_callback(key, c_msg):  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->mon_register_callback( evento->tipo, evento->contenido );
			break;
		case 57:	// mon_forced_call_end_ack_callback
			this->logger->Write( System::String::Format( "trataEvento mon_forced_call_end_ack_callback (key, c_msg): ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->mon_forced_call_end_ack_callback( evento->contenido );
			break;
		case 58:	// mon_order_ack
			this->logger->Write( System::String::Format( "trataEvento mon_order_ack (key, c_msg): ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->mon_order_ack_callback( evento->contenido );
			break;
		case 59:	// mon_orderclose
		case 60:	// mon_orderclose_ack
			this->logger->Write( System::String::Format( "trataEvento mon_order_status_callback (key, c_msg): ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->mon_order_status_callback( evento->tipo, evento->contenido );
			break;
		case 61:	// mon_intercept_ack_rtp
			this->logger->Write( System::String::Format( "trataEvento mon_intercept_ack_rtp (key, c_msg): ({0},{1})", evento->tipo, evento->contenido ), 0 );
			this->mon_intercept_ack_callback( evento->contenido );
			break;
		case 62:	// mon_intercept_connect_rtp
			this->logger->Write( System::String::Format( "trataEvento mon_intercept_connect_rtp (key, c_msg): ({0},{1})", evento->tipo, evento->contenido ), 0 );
			this->mon_intercept_connect_callback( evento->contenido );
			break;
		case 45:	// mon_intercept_disconnect
		case 46:	// mon_intercept_disconnect_ack
			this->logger->Write( System::String::Format( "trataEvento mon_intercept_disconnect (key, c_msg): ({0},{1})", evento->tipo, evento->contenido ), 0 );
			this->mon_intercept_disconnect_callback( evento->tipo, evento->contenido );
			break;
		case 17:	// mon_tx_demand
		case 18:	// mon_tx_ceased
		case 19:	// mon_tx_grant
			this->logger->Write( System::String::Format( "trataEvento mon_tx_callback (key, c_msg): ({0},{1})", evento->tipo, evento->contenido ), 0 );
			this->mon_tx_callback( evento->tipo, evento->contenido );
			break;
		case 16:	// mon_cc_information
			this->logger->Write( System::String::Format( "trataEvento: mon_cc_information_callback(key, c_msg):  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->mon_cc_info_callback( evento->contenido );
			break;
		case 20:	// mon_disconnect
			this->logger->Write( System::String::Format( "trataEvento: mon_disconnect_callback(key, c_msg):  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->mon_disconnect_callback( evento->contenido );
			break;
		case 7:		// dyn_group_add_ack
		case 8:		// dyn_group_del_ack
			this->logger->Write( System::String::Format( "trataEvento: DYN_group_callback(key, c_msg):  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->dyn_group_callback( evento->tipo, evento->contenido );
			break;
		case 36:	// group_attach_ack
		case 37:	// group_detach_ack
			this->logger->Write( System::String::Format( "trataEvento: group_callback(key, c_msg):  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->group_callback( evento->tipo, evento->contenido );
			break;

		case 42:	// cc_setup_ack
			this->logger->Write( System::String::Format( "trataEvento: cc_setup_ack(key, c_msg):  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->cc_setup_ack_callback( evento->contenido );
			break;
		case 28:	// cc_information
			this->logger->Write( System::String::Format( "trataEvento: cc_information(key, c_msg):  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->cc_information_callback( evento->contenido );
			break;
		case 29:	// cc_connect
			this->logger->Write( System::String::Format( "trataEvento: cc_connect(key, c_msg):  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->cc_connect_callback( evento->contenido );
			break;
		case 30:	// cc_disconnect
		case 31:	// cc_disconnect_ack
			this->logger->Write( System::String::Format( "trataEvento: cc_disconnect/_ack(key, c_msg):  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->cc_disconnect_callback( evento->contenido );
			break;
		case 32:	// cc_setup
			this->logger->Write( System::String::Format( "trataEvento: cc_setup(key, c_msg):  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->cc_setup_callback( evento->contenido );
			break;
		case 33:	// cc_accepted
			this->logger->Write( System::String::Format( "trataEvento: cc_accepted(key, c_msg):  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->cc_accepted_callback( evento->contenido );
			break;
		case 34:	// cc_ceased
			this->logger->Write( System::String::Format( "trataEvento: cc_ceased(key, c_msg):  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->cc_ceased_callback( evento->contenido );
			break;
		case 35:	// cc_grant
			this->logger->Write( System::String::Format( "trataEvento: cc_grant(key, c_msg):  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->cc_grant_callback( evento->contenido );
			break;

		case AC_ENVIA_SDS:	//	manager => envia SDS
			this->logger->Write( System::String::Format( "trataEvento: AcCore-Envia_Sds  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->envia_sds ( evento->contenido );
			break;
		case AC_MOD_DGNA:	//	manager => modifica miembros de DGNA
			this->logger->Write( System::String::Format( "trataEvento: AcCore-Modifica-DGNA  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->modifica_grupo ( evento->contenido );
			break;
		case AC_ENVIA_SDS_STATUS:	//	manager => envia SDS STATUS
			this->logger->Write( System::String::Format( "trataEvento: envia_sds_status  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->envia_sds_status ( evento->contenido );
			break;
		case AC_CALL_DUPLEX:
		case AC_CALL_SIMPLEX:
		case AC_CALL_GROUP:		// manager => llama
		case AC_CALL_LISTEN_AMBIENCE:
			this->logger->Write( System::String::Format( "trataEvento: AC_CALL_...  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->call_ssi ( evento->tipo, evento->contenido, evento->expira );
			break;
		case AC_PTT:
			this->logger->Write( System::String::Format( "trataEvento: AC_PTT  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->ptt ( evento->contenido );
			break;
		case AC_PINCHA_LINEA:
			this->logger->Write( System::String::Format( "trataEvento: AC_PINCHA_LINEA  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->pincha_linea ( evento->contenido );
			break;
		case AC_CAMBIA_GRUPO_LINEA:
			this->logger->Write( System::String::Format( "trataEvento: AC_CAMBIA_GRUPO_LINEA  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->cambia_grupo_asociado ( evento->contenido );
			break;
		case AC_MOD_MON_SSI:
			this->logger->Write( System::String::Format( "trataEvento: AC_MOD_MON_SSI  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->modifica_item_mon ( evento->contenido );
			break;
		case AC_LISTEN_MON_SSI:
			this->logger->Write( System::String::Format( "trataEvento: AC_LISTEN_MON_SSI  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->escucha_item_mon ( evento->contenido, evento->expira );
			break;
		case AC_MON_FORCE_CALL_END:
			this->logger->Write( System::String::Format( "trataEvento: AC_MON_FORCE_CALL_END  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->force_end_mon_call ( evento->contenido );
			break;
		case AC_RESET_MON:
			this->logger->Write( System::String::Format( "trataEvento: AC_RESET_MON  ({0},{1})", evento->tipo, evento->contenido ), -1 );
			this->reset_mon_items( evento->contenido );
			break;
		default:
			this->logger->Write( System::String::Format( "!!!trataEvento desconocido!!!:  {0} - {1}", evento->tipo, evento->contenido ), 3 );
			break;
	}
}

/*
connection_state	state
char				char
2					1

estado = 0 => ok
*/
void Consumidor::connection_state_callback( System::String^ _contenido )
{
	try
	{
		int estado = System::Convert::ToInt32( _contenido->Substring( 2, 1 ) );
		if  (estado == 1 || estado == 0 )
		{
			this->conexion_acapi_ok = ( estado == 0 );
			this->logger->Write( System::String::Format( "connection_state_callback: estado ok? {0}", this->conexion_acapi_ok ), 2 );

			if ( estado == 0 )
			{
				// Al arrancar los ssis de las lineas ( tipo applicacion no terminal ), no pertenecen a ningún grupo, tambien ocurre si se pierde la conexion
				this->reset_attached( -1 );
			}

			// reinicio todos los servicios
			for ( int i = 0; i < this->lineas->Length; i ++ )
			{
				this->lineas[i]->cc = gcnew Servicio_tetra( false, -1 );
				this->lineas[i]->sds = gcnew Servicio_tetra( false, -1 );
				this->lineas[i]->ss = gcnew Servicio_tetra( false, -1 );
				this->lineas[i]->mon = gcnew Servicio_tetra( false, -1 );
				this->lineas[i]->comunicacion = gcnew Comunicacion_tetra( -1, -1, -1 );
			}
			this->mon_reg_handle = -1;
			// se refrescan todas las pertenencias, los ssi applicacion podrian pertenecer a multiples grupos
		}
		else
			throw gcnew System::Exception();
	}
	catch (System::Exception^ e)
	{
		this->logger->Write( "problema procesando connection_state_callback", 4 );
	}
}

/*
X_service_connection_state	app_handle	state
char						int			char
2							4			1
estado = 0 => ok
*/
void Consumidor::service_state_callback( short tipo, System::String^ _contenido )
{
	try
	{
		int handle = System::Convert::ToInt32( _contenido->Substring( 2, 4 ), 16 );
		int estado = System::Convert::ToInt32( _contenido->Substring( 6, 1 ) );
		int index;
		System::String^ msg;
		if  ( ! ( estado == 1 || estado == 0 ) && handle >= 0 )
		{
			throw gcnew System::Exception();
		}
		// Se busca la linea afectada mediante "find_index_by_handle" y se actualiza el estado del servicio
		switch (tipo)
		{
			case 4:				// sds_service_connection_state
				index = this->find_index_by_handle( handle, SDS_APP );
				this->lineas[index]->sds->estado_ok = ( estado == 0 );
				if ( estado == 0 )		// reinicio t_pregunta si ya conectado
					this->lineas[index]->sds->t_pregunta = nullptr;
				msg = "sds_service_connection_state";
				break;
			case 5:				// mon_service_connection_state
				index = this->find_index_by_handle( handle, MON_APP );
				this->lineas[index]->mon->estado_ok = ( estado == 0 );
				if ( estado == 0 )		// reinicio t_pregunta si ya conectado
				{
					this->lineas[index]->mon->t_pregunta = nullptr;

					EventoCola^ evento = gcnew EventoCola();
					evento->tipo = AC_RESET_MON;
					evento->contenido = System::String::Format( "{0}", this->lineas[index]->ssi_linea );
			
					this->cola->Enqueue(evento);

					if( this->permiso_continua )	// Solo continuaré consumiendo si hay permiso
						this->continua->Set();
				}
				msg = "mon_service_connection_state";
				// comprobar que si se desconecta
				break;
			case 6:				// supplementary_service_connection_state
				index = this->find_index_by_handle( handle, SS_APP );
				this->lineas[index]->ss->estado_ok = ( estado == 0 );
				if ( estado == 0 )		// reinicio t_pregunta si ya conectado
					this->lineas[index]->ss->t_pregunta = nullptr;
				else if ( estado == 1 )	// cuando pierdo la conexion del servicio SS reseteo el estado attached de la linea
					this->reset_attached( index );
				msg = "supplementary_service_connection_state";
				break;
			case 27:			// cc_service_connection_state
				index = this->find_index_by_handle( handle, CC_APP );
				this->lineas[index]->cc->estado_ok = ( estado == 0 );
				if ( estado == 0 )		// reinicio t_pregunta si ya conectado
					this->lineas[index]->cc->t_pregunta = nullptr;
				msg = "cc_service_connection_state";
				break;
		}
		this->logger->Write( System::String::Format( "{0} handle: {1}, estado: {2}", msg, handle, estado ), 1 );
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema procesando service_state_callback", 4 );
	}
}

/*
	Todas las lineas son capaces de recibir mensajes, según sean recibidos o emitidos por/desde lineas despachadoras
se distinguirán mensajes despachadores operadores. Supuestamente los despachadores tendrán acceso a los mensajes de operadores
pero no al revés. Esta lógica se deberá implementar en AcOper. Los mensajes recibidos que tengan como destino un ssi de grupo
se consideraran como mensajes de operador.

	Si todas las lineas pueden recibir mensajes se recibirá un mensaje de grupo por cada linea perteneneciente a este grupo, para no
almacenar mensajes duplicados se hace una lógica para evitar duplicados. Cada mensaje se distingue por el id "meg_number", se guardarán
un número arbitrario de los últimos "msg_number" recibidos y de esta manera se podrán ignorar mensajes duplicados.

					key 0
sds_data	app_handle	source	target	msg_number	length	sds_data
char 		int			string	string	int			int		string
2 			4			8		8		4			4		length

					key 1
sds_status	app_handle	source	target	msg_number	sds_data
char 		int			string	string	int			string
2 			4			8		8		4			4

					key 2
sds_ack		app_handle	source	target	msg_number
char 		int			string	string	int
2 			4			8		8		4
*/
void Consumidor::sds_callback(short tipo, System::String^ _contenido)
{
	bool recibido = true;
	bool despachador = false;				//añadir logica, dirigido a operadores o despachadores?
	System::Int32 origen;
	System::Int32 destino;
	System::Int32 msg_number;
	System::Int32 index_linea;

	System::String^ autor = gcnew System::String( "UNKNOWN" );			// recibido desde la red tetra

	int handle;
	System::String^ tl = nullptr;
	System::String^ contenido = nullptr;
	bool procesado = false;
	bool consumido = false;
	System::DateTime^ t_tetra = nullptr;

	int entryInserted;

	try
	{
		origen		= System::Convert::ToInt32( _contenido->Substring( 6, 8 ), 16 );
		destino		= System::Convert::ToInt32( _contenido->Substring( 14, 8 ), 16 );
		msg_number	= System::Convert::ToInt32( _contenido->Substring( 22, 4 ), 16 );

		index_linea = this->find_index_by_ssi( destino );			// si el resultado es -1 significará que es un mensaje de grupo
		
		if ( index_linea != -1 )
		{
			despachador = this->lineas[index_linea]->despachador;
		}
		else
		{
			despachador = false;
			this->logger->Write( System::String::Format( "SDS de grupo recibido!!!! destino: {0}", destino ), 2 );
		}


		if ( this->check_duplicated_sds( msg_number ) )	// comprobación de duplicados
		{	// duplicado !!
			this->logger->Write( System::String::Format( "SDS duplicado ignorado!!!! msg_number {0} destino: {1}", msg_number, destino ), 2 );
			return;
		}


		if ( tipo == 0 || tipo == 1  )	// SDS-DATA o STATUS, ya que los status pueden ser SHORT reports
		{
			//------------------------------caso: tipo 0-------------------------------------
			//Procesar TL
			int length = System::Convert::ToInt32( _contenido->Substring( 26, 4 ), 16 );	// contendrá los datos en el caso de status!!
			System::String^ sds_data;
			if ( tipo == 0 )
				sds_data = _contenido->Substring( 30, length );

			switch( this->procesaSdsTL( tipo, sds_data, &handle, tl, contenido  ) )
			{
				case 0:		//msg recibido  simple / inmediate / Full_TL / status (NO SHORT REPORT)
					t_tetra = System::DateTime::Now;
					entryInserted = this->db->insert_SDS( recibido, despachador, origen, destino, autor, handle, tl,
											contenido, procesado, consumido, t_tetra, nullptr , nullptr);
					if ( entryInserted >= 0 )
					{
						// mantengo track de los msg_number solo si se han onseguido insertar en la bd
						this->insert_sds_msg_number( msg_number );

						// construyo trama para enviar al AcCore, 800->FRT_AVISA_CAMBIOS
						this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}{4}",
																TRAMA_INI, FRT_AVISA_CAMBIOS, System::Convert::ToString( T_MENSAJES ),
																System::Convert::ToString( entryInserted ), TRAMA_END ) );
						this->logger->Write( System::String::Format( "SDS recibido!!!! msg_no {0} contenido: {1}", msg_number, contenido ), 0 );
					}
					else
						this->logger->Write( System::String::Format( "SDS recibido! Problema insertando en DB!!: {0} ", contenido ), 4 );

					break;
				case 1:		// recibido			SDS-REPORT
					this->logger->Write( "SDS-REPORT recibido!!!!", 2 );
					break;
				case 2:		// consumido		SDS-REPORT
					this->logger->Write( "SDS-REPORT consumido!!!!", 2 );
					break;
				case -1:
					this->logger->Write( "SDS con formato no reconocido", 4 );
					break;
			}
		}
		else if ( tipo == 2 )
		{
			this->logger->Write( "SDS ack recibido", 1 );
		}

		// implementar ESTO en "procesaSDSTL" !!
		if ( tipo == 1 )		// SDS status y SDS-SHORT REPORT!!
		{
			System::Int32 status_number = System::Convert::ToInt32( _contenido->Substring( 26, 4 ), 16 );

			if ( this->check_duplicated_sds( msg_number ) )	// comprobación de duplicados
			{	// duplicado !!
				this->logger->Write( System::String::Format( "SDS_STATUS duplicado ignorado!!!! msg_number {0} destino: {1}", msg_number, destino ), 2 );
			}
			else
			{
				t_tetra = System::DateTime::Now;
				handle = SDS_HANDLE_STATUS;			// handle se usa para estudiar los informes de proceso y consumicion con FULL TL
				tl = System::String::Format( "Sx{0}", status_number );			// en el campo tl, guardaré el numero de status

				bool sds_status_defined = this->db->read_sds_status_text( status_number, contenido );
				// se lee el texto asociado al mensaje de estado para insertarlo en la tabla de mensajes
				if ( ! sds_status_defined )
				{
					this->logger->Write( System::String::Format( "ENVIAR sds status number {0} no definido!!!!???", status_number ), 2 );
					contenido = System::String::Format( "MENSAJE DE ESTADO {0} NO RECONOCIDO", status_number );
				}

				entryInserted = this->db->insert_SDS( recibido, despachador, origen, destino, autor, handle, tl,
										contenido, procesado, consumido, t_tetra, nullptr , nullptr);

				if ( entryInserted < 0 )
				{
					this->logger->Write( "recibe SDS_STATUS, error insertando en DB", 4 );
				}
				else
				{
					// mantengo track de los msg_number solo si se han onseguido insertar en la bd
					this->insert_sds_msg_number( msg_number );

					// construyo trama para enviar al AcCore, 800->FRT_AVISA_CAMBIOS
					this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}{4}",
																TRAMA_INI, FRT_AVISA_CAMBIOS, System::Convert::ToString( T_MENSAJES ),
																System::Convert::ToString( entryInserted ), TRAMA_END ) );
					this->logger->Write( System::String::Format( "SDS status recibido, origen:{0}, destino:{1}, msg_no:{2} contenido:{3} ",
										origen, destino, msg_number, System::Convert::ToInt32( _contenido->Substring( 26, 4 ), 16 ) ), 1 );
				}
			}
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "ocurrió algún error en recibeSDS()", 4 );
	}
}

/*
SDS_DATA:
-Simple Text Messaging			[pId][rsv-Coding][Text..]	[02][01][Text..]
-Simple InmediateText Messaging	[pId][rsv-Coding][Text..]	[09][01][Text..]
	
	[01] => ISO/IEC 8859-1 Latin 1 (8-bit) alphabet

-FULL_TL Text Messaging:
------------SDS-TRANSFER		[pId][MesgType-ReportType-ServiceSelection-Storage][msgRef][TimeStamp?-Coding][Text...]

	pid					[82]
	MesgType			(0000)
	ReportType			(00)-not supported	(01)-Destination memory full	(10)-received	(11)-consumed and received
	ServiceSelection	(0)-use SDS-SHORT-REPORT recommended/individual service	(1)-only SDS-REPORT/group service
	Storage				(0)-not allowed		(1)-allowed =>distinto formato TL, campos adicionales!!!
	
	Example:			82 0C 77 01 [Text..]
	
	consumed and received report, use SDS-Short-version, no storage, msgRef[77], no timeStamp( (0) ), coding(0000001-ASCII)
	
------------SDS-REPORT
					length	M/O/C
	pid 			8	M		[82]
	MesgType		4	M		(0001)
	AckRequired 	1	M		(0)no (1)si
	Reserved 		2	M		
	Storage 		1	M		(0)no (1)si
	Deliverystatus 	8 	M		(000XXXXX)SDS data transfer success	(001XXXXX)temp error	(010XXXXX)failed  [seccion 29.4.3.2
	msgRef 			8 	M

	Example:	82 10 05 77	message sent to group

SDS_STATUS: Se envia y recibe como SDS estatus => 2 bytes!!
------------SDS-SHORT REPORT
				length
	SDS-TL PDU 			6		(011111) This status message belongs to the SDS-TL protocol
	Short report type	2		(00)-not supported	(01)-Destination memory full	(10)-received	(11)-consumed and received
	Message reference	8

	Example:	7E 77		sds con msgref=77 received
				7F 77		sds con msgref=77 consumed

Documentacion->ETSI EN 300 392-2 (v2.5.1)
	Terrestrial Trunked Radio (TETRA), Voice plus Data (V+D), Part2: Air Interface (AI)
*/
int Consumidor::procesaSdsTL( short tipo, System::String^ sds_data, int *handle, [Out]System::String^ %tl, [Out]System::String^ %contenido )
{
	try
	{
		//array<System::Byte>^ bytes = System::Text::Encoding::GetEncoding( "Windows-1252" )->GetBytes( sds_data );
		array<System::Byte>^ bytes = System::Text::Encoding::GetEncoding( "iso-8859-1" )->GetBytes( sds_data );		// Windows-1252 codifica correctamente caracteres ASCII con un valor por encima de 127 (7 bits)

		if ( tipo == 0 )	// SDS-DATA
		{
			if ( bytes[0] == 2 || bytes[0] == 9 )		//simple and inmediate text messaging
			{
				this->logger->Write( "SDS 02/09", 1 );
				if ( bytes[1] == 1 )					//ASCII encoding
				{
					*handle = SDS_HANDLE_SIMPLE;
					tl = ( System::BitConverter::ToString( bytes, 0, 2 ) )->Replace( "-", System::String::Empty );
					contenido = sds_data->Substring( 2 );
					return 0;
				}
				else
				{
					this->logger->Write("----SIMPLE/INMEDIATE SDS recibido con Codificacion distinta a iso-8859-1 !!!", 2);
					return -1;
				}
			}
			if ( bytes[0] == 130 )						// 0x82		full TL text messaging
			{
				this->logger->Write( "SDS 82", 1 );
				*handle = -2;
				tl = ( System::BitConverter::ToString( bytes, 0, 1 ) )->Replace( "-", System::String::Empty );
				contenido = sds_data->Substring( 1 );
				return 0;
			}
			return -1;
		}
		else if ( tipo == 1 )	// SDS-STATUS
		{
			if ( bytes[0] == 2 || bytes[0] == 9 )		//simple and inmediate text messaging
			{
				this->logger->Write( "SDS 02/09", 1 );
				if ( bytes[1] == 1 )					//ASCII encoding
				{
					*handle = SDS_HANDLE_SIMPLE;
					tl = ( System::BitConverter::ToString( bytes, 0, 2 ) )->Replace( "-", System::String::Empty );
					contenido = sds_data->Substring( 2 );
					return 0;
				}
				else
				{
					this->logger->Write("----SIMPLE/INMEDIATE SDS recibido con Codificacion distinta a iso-8859-1 !!!", 2);
					return -1;
				}
			}
			if ( bytes[0] == 130 )						// 0x82		full TL text messaging
			{
				this->logger->Write( "SDS 82", 1 );
				*handle = -2;
				tl = ( System::BitConverter::ToString( bytes, 0, 1 ) )->Replace( "-", System::String::Empty );
				contenido = sds_data->Substring( 1 );
				return 0;
			}
			return -1;
		}
		else
		{
			return -1;
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write("EEEEEEEEEEEEEE problema en procesaSdsTL", 4);
		return -1;
	}
}

/*
	Orden recibida de mandar un sds. Según el parámetro [desp] valga 1-true o 0-false el mensaje se mandará desde el
primer despachador o el primer operador.
	Solo se enviará sds desde estos dos ssis particulares porque no se puede distinguir que operador posee que línea,
se pueden mandar sds sin ninguna línea pinchada.
	Sin embargo se pueden recibir mensajes destinados a cualquiera de los ssis asociados a las líneas. Debido a la recepción
de mensajes de grupo. Si solo pudieran recibir sds el primer despachador y operador solo se recibirían sds de los grupos asociados
a estos dos ssi particulares.

[desp]:[autor]:[ssiDest]:[long]:[text]
*/
void Consumidor::envia_sds( System::String^ info )
{
	try
	{
		array<System::String^>^ campos = nullptr;
		array<wchar_t>^ separador = {':'};
		campos = info->Split( separador, System::StringSplitOptions::RemoveEmptyEntries );
	
		bool recibido = false;
		bool despachador = false;
		System::Int32 origen =-1;
		System::Int32 destino = -1;	
		System::String^ autor = nullptr;

		int sendRes;

		int app_handle;

		System::String^ tl = nullptr;
		System::String^ contenido = nullptr;
		bool procesado = false;
		bool consumido = false;
		System::DateTime^ t_tetra = nullptr;

		char sds_data_char[255];
		memset( &sds_data_char, 0, sizeof( sds_data_char ) );
		char sds_ssi_dest_char[10];
		memset( &sds_ssi_dest_char, 0, sizeof( sds_ssi_dest_char ) );

		int length;
	
		if ( campos->Length == 5 )
		{
			despachador = campos[0]->Equals( "1" );
			if ( despachador )
			{
				origen = this->lineas[this->index_first_disp]->ssi_linea;
				app_handle = this->lineas[this->index_first_disp]->sds->handle;
			}
			else
			{
				origen = this->lineas[this->index_first_oper]->ssi_linea;
				app_handle = this->lineas[this->index_first_oper]->sds->handle;
			}
			autor = campos[1];
			destino = System::Convert::ToInt32( campos[2] );
			tl = "0201";
			length = System::Convert::ToInt32( campos[3] ) + 2;				// se suman los dos bytes de la cabecera tl
			contenido = campos[4];

			//Relleno la cabecera TL => [2][1] simple textMessaging
			sds_data_char[0] = 2;
			sds_data_char[1] = 1;

			System::IntPtr p = Marshal::StringToHGlobalAnsi( campos[4] );			//necesario para pasar String [Texto sds] a char nativo
			char *text_char = static_cast<char*>( p.ToPointer() );
			strcat( sds_data_char, text_char );										//Contenido sds con cabecera TL

			p = Marshal::StringToHGlobalAnsi( campos[2] );							//hago lo mismo para el ssi destino
			char *ssi_dst_ptr = static_cast<char*>( p.ToPointer() );
			strcat( sds_ssi_dest_char, ssi_dst_ptr );
			Marshal::FreeHGlobal(p);

			sendRes = tetra->send_sds( app_handle, despachador ? 1 : 0, sds_data_char, length, sds_ssi_dest_char, false );		// manda SDS, devuelve handle. -1 => ha habido un error
			if ( sendRes >= 0 )
			{
				t_tetra = System::DateTime::Now;
				int entryInserted = this->db->insert_SDS( recibido, despachador, origen, destino, autor, -1, tl,
											contenido, procesado, consumido, t_tetra, nullptr , nullptr );

				if ( entryInserted < 0 )
				{
					this->logger->Write( "envia SDS, error insertando en DB", 4 );
				}
				else
				{
					// construyo trama para enviar al AcCore, 800->FRT_AVISA_CAMBIOS
					this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}{4}",
																TRAMA_INI, FRT_AVISA_CAMBIOS, System::Convert::ToString( T_MENSAJES ),
																System::Convert::ToString( entryInserted ), TRAMA_END ) );
					this->logger->Write( System::String::Format( "envia SDS a {0} de {1} con contenido: {2}", destino, autor, contenido ), 1 );
				}
			}
			else
			{
				this->logger->Write( "tetra interface unable to send SDS", 4 );
			}
		}
		else
		{
			this->logger->Write( "envia SDS info bad format", 4 );
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "envia SDS exception", 4 );
	}
}

/*
	Orden recibida de mandar un sds status. Según el parámetro [desp] valga 1-true o 0-false el mensaje se mandará desde el
primer despachador o el primer operador.
	Solo se enviará sds desde estos dos ssis particulares porque no se puede distinguir que operador posee que línea,
se pueden mandar sds sin ninguna línea pinchada.
	Sin embargo se pueden recibir mensajes destinados a cualquiera de los ssis asociados a las líneas. Debido a la recepción
de mensajes de grupo. Si solo pudieran recibir sds el primer despachador y operador solo se recibirían sds de los grupos asociados
a estos dos ssi particulares.

	[status_number] => numero de sds status

[desp]:[autor]:[ssiDest]:[status_number]
*/
void Consumidor::envia_sds_status( System::String^ info )
{
	try
	{
		array<System::String^>^ campos = nullptr;
		array<wchar_t>^ separador = {':'};
		campos = info->Split( separador, System::StringSplitOptions::RemoveEmptyEntries );
	
		bool recibido = false;
		bool despachador = false;
		System::Int32 origen =-1;
		System::Int32 destino = -1;	
		System::String^ autor = nullptr;

		bool send_res;
		bool sds_status_defined;

		int app_handle;

		System::String^ tl = nullptr;
		System::String^ contenido = nullptr;
		bool procesado = false;
		bool consumido = false;
		System::DateTime^ t_tetra = nullptr;

		int status_number;
	
		if ( campos->Length == 4 )
		{
			despachador = campos[0]->Equals( "1" );
			if ( despachador )
			{
				origen = this->lineas[this->index_first_disp]->ssi_linea;
				app_handle = this->lineas[this->index_first_disp]->sds->handle;
			}
			else
			{
				origen = this->lineas[this->index_first_oper]->ssi_linea;
				app_handle = this->lineas[this->index_first_oper]->sds->handle;
			}
			autor = campos[1];
			destino = System::Convert::ToInt32( campos[2] );
			status_number = System::Convert::ToInt32( campos[3] );				// se suman los dos bytes de la cabecera tl

			tl = System::String::Format( "Sx{0}", status_number );			// en el campo tl, guardaré el numero de status

			send_res = tetra->send_sds_status( app_handle, despachador ? 1 : 0, destino, status_number );
			if ( send_res )
			{
				sds_status_defined = this->db->read_sds_status_text( status_number, contenido );
				if ( ! sds_status_defined )
				{
					this->logger->Write( System::String::Format( "sds status number {0} no definido!!!!???", status_number ), 2 );
					contenido = System::String::Format( "MENSAJE DE ESTADO {0} NO RECONOCIDO", status_number );
				}
				t_tetra = System::DateTime::Now;
				int entryInserted = this->db->insert_SDS( recibido, despachador, origen, destino, autor, -1, tl,
											contenido, procesado, consumido, t_tetra, nullptr , nullptr );

				if ( entryInserted < 0 )
				{
					this->logger->Write( "envia SDS, error insertando en DB", 4 );
				}
				else
				{
					// construyo trama para enviar al AcCore, 800->FRT_AVISA_CAMBIOS
					this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}{4}",
																TRAMA_INI, FRT_AVISA_CAMBIOS, System::Convert::ToString( T_MENSAJES ),
																System::Convert::ToString( entryInserted ), TRAMA_END ) );
					this->logger->Write( System::String::Format( "envia SDS a {0} de {1} con contenido: {2}", destino, autor, contenido ), 1 );
				}
			}
			else
			{
				this->logger->Write( "tetra interface unable to send SDS_STATUS", 4 );
			}
		}
		else
		{
			this->logger->Write( "envia SDS _STATUSinfo bad format", 4 );
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "envia SDS_STATUS exception", 4 );
	}
}

/*	---------------- ¡¡¡¡¡¡¡¡¡ DEPRECATED !!!!!!!!!!! --------------------
				mon_close/mon_monitoring_ack
key				app_handle	mon_handle	begin_ssi	end_ssi	service
char			int			int			string		string	char
2 				4 			4 			8 			8 		1

ACK->	bit: 0-registration,	1-sdsInitiator,	2-sdsDestination,	3-circuitModeTarget,	4-circuitModeDestination
CLOSE->	bit: 0-closeRequest,	1-noPermission,	2-subscriberError,	3-serviceNotAvail,		4-fleetUnknown				5-endTimeExpired

						mon_close_ack
----------------------DOCUMENTACION-------------------------
mon_close_ack	app_handle	mon_handle	begin_ssi	end_ssi	result
char			int			int			string		string	short
2 				4 			4 			8 			8 		2

----------------------¡¡¡OBSERVADO!!!-------------------------
mon_close_ack	app_handle	mon_handle	result
char			int			int			short
2 				4 			4 	 		2 
*/
void Consumidor::mon_status_callback( short tipo, System::String^ _contenido )
{
	System::Int32 monHandle, begin_ssi, end_ssi;
	wchar_t resultado;
	try
	{
		monHandle = System::Convert::ToInt32( _contenido->Substring( 6, 4 ), 16 );
		switch (tipo)
		{
			case 11:	// mon_close
			case 22:	// mon_monitoring_ack
				begin_ssi = System::Convert::ToInt32( _contenido->Substring( 10, 8 ), 16 );
				end_ssi = System::Convert::ToInt32( _contenido->Substring( 18, 8 ), 16 );
				resultado = _contenido[26];

				// monitorizacion de terminales acabada por el servidor acapi
				if ( ( tipo == 11 ) && ( begin_ssi == this->mon_ssi_ini ) && ( end_ssi == this->mon_ssi_end ) )
				{
					this->mon_reg_handle = -1;
					this->logger->Write( System::String::Format( "MON_CLOSE- servicio de monitorización desregistrado con handle: {0}", monHandle ), 2 );
				}
				// monitorizacion de registro, desregistro de terminales. Nos cercioramos que es el mismo servicio de monitorización y actualizamos terminalMonHandle de TetraInterface
				if ( ( tipo == 22 ) && ( begin_ssi == this->mon_ssi_ini ) && ( end_ssi == this->mon_ssi_end ) && resultado == '1' )
				{
					this->mon_reg_handle = monHandle;
					this->logger->Write( System::String::Format( "MON_MONITORING_ACK- servicio de monitorización establecido con handle: {0}", monHandle ), 0 );
				}
				break;
			case 23:	// mon_close_ack
				if ( monHandle == this->mon_reg_handle )
				{
					this->logger->Write( System::String::Format( "MON_CLOSE_ACK- Confirmación desregistro con handle: {0}", monHandle ), 0 );
					this->mon_reg_handle = -1;
				}
				break;
		}
	}
	catch (System::Exception^ e)
	{
		this->logger->Write( "Problema procesando mon_status_callback", 2 );
	}
}

/*
					mon_order_ack & type = 'R' range of subscriber
key				app_handle	mon_handle	reg_mon		sds_mon		cc_mon	type	begin_ssi	end_ssi
char			int			int			int			int			int		char	int			int
2 				4 			4 			1			1			1		1		4 			4

					mon_order_ack & type = 'S' single subscriber
key				app_handle	mon_handle	reg_mon		sds_mon		cc_mon	type	mon_ssi
char			int			int			int			int			int		char	int
2 				4 			4 			1			1			1		1		4

reg_mon	0-no service	1-no initial briefing	2-briefing for registered subscribers	3-briefing for all subscribers
sds_mon	0-no service	1-sds monitoring		2-sds monitoring without sds-TL reports
cc_mon	0-no service	1-monitoring of circuit mode calls

*/
void Consumidor::mon_order_ack_callback( System::String^ _contenido )
{
	System::Int32 app_handle, monHandle, begin_ssi, end_ssi;
	System::Int32 reg_mon, sds_mon, cc_mon;
	wchar_t ack_type;

	try
	{
		app_handle = System::Convert::ToInt32( _contenido->Substring( 2, 4 ), 16 );
		monHandle = System::Convert::ToInt32( _contenido->Substring( 6, 4 ), 16 );
		reg_mon		= System::Convert::ToInt32( _contenido->Substring( 10, 1 ), 16 );
		sds_mon		= System::Convert::ToInt32( _contenido->Substring( 11, 1 ), 16 );
		cc_mon		= System::Convert::ToInt32( _contenido->Substring( 12, 1 ), 16 );

		ack_type	= _contenido[13];

		if ( ack_type == 'R' )
		{
			begin_ssi	= System::Convert::ToInt32( _contenido->Substring( 14, 4 ), 16 );
			end_ssi		= System::Convert::ToInt32( _contenido->Substring( 18, 4 ), 16 );
			// monitorizacion de registro, desregistro de terminales. Nos cercioramos que es el mismo servicio de monitorización y actualizamos terminalMonHandle de TetraInterface
			if ( ( begin_ssi == this->mon_ssi_ini ) && ( end_ssi == this->mon_ssi_end ) && reg_mon > 0 && this->lineas[this->index_first_disp]->mon->handle == app_handle )
			{
				this->mon_reg_handle = monHandle;
				this->logger->Write( System::String::Format( "MON_ORDER_ACK- servicio de monitorización establecido con app: {0} handle: {1} y servicio registro: {2}", app_handle, monHandle, reg_mon ), 1 );
			}
		}
		else if ( ack_type == 'S' )
		{
			System::Int32 index_linea = find_index_by_handle( app_handle, MON_APP );
			begin_ssi	= System::Convert::ToInt32( _contenido->Substring( 14, 4 ), 16 );
			
			// monitorizacion de registro, desregistro de terminales. Nos cercioramos que es el mismo servicio de monitorización y actualizamos terminalMonHandle de TetraInterface
			System::Int32 index_mon = find_mon_index_by_ssi( index_linea, begin_ssi );
			if ( index_mon >= 0 )
			{
				if ( cc_mon > 0 )
				{
					this->lineas[index_linea]->monitorizaciones[index_mon]->mon_handle = monHandle;
					this->logger->Write( System::String::Format( ",,,,,,,,,,MON_ORDER_ACK desp:{0} ssi_mon:{1} handle:{2}",
								this->lineas[index_linea]->ssi_linea, begin_ssi, monHandle ), 0 );
					this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}:{4}{5}",
								TRAMA_INI, AC_MOD_MON_SSI_ACK, this->lineas[index_linea]->ssi_linea, begin_ssi, 1, TRAMA_END ) );
				}
				else
				{
					this->logger->Write( System::String::Format( ",,,,,,,,,,MON_ORDER_ACK SIN CC!! desp:{0} ssi_mon:{1} handle:{2}",
							this->lineas[index_linea]->ssi_linea, begin_ssi, monHandle ), 2 );
					this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}:{4}{5}",
								TRAMA_INI, AC_MOD_MON_SSI_ACK, this->lineas[index_linea]->ssi_linea, begin_ssi, 0, TRAMA_END ) );
				}
			}
			else
			{
				this->logger->Write( System::String::Format( ",,,,,,,,,,MON_ORDER_ACK INCOHERENTE! desp:{0} ssi_mon:{1} handle:{2}",
					this->lineas[index_linea]->ssi_linea, begin_ssi, monHandle ), 2 );
			}
		}
		else
		{
			this->logger->Write( System::String::Format( "mon_order_ack type distinto de 'R' o 'S', recibido tipo:{0}", ack_type ), 4 );
		}
	}
	catch (System::Exception^ e)
	{
		this->logger->Write( "Problema procesando mon_status_callback", 4 );
	}
}

/*
				mon_order_ack & type = 'R' range of subscriber
key				app_handle	mon_handle	reg_mon		sds_mon		cc_mon	type	begin_ssi	end_ssi
char			int			int			int			int			int		char	int			int
2 				4 			4 			1			1			1		1		4 			4
reg_mon	0-no service	1-no initial briefing	2-briefing for registered subscribers	3-briefing for all subscribers
sds_mon	0-no service	1-sds monitoring		2-sds monitoring without sds-TL reports
cc_mon	0-no service	1-monitoring of circuit mode calls

		mon_orderclose / mon_close_ack
mon_close_ack	app_handle	mon_handle	result
char			int			int			short
2 				4 			4 	 		2 

result(mon_orderclose)													result(mon_close_ack)
	0: Monitoring Close Request from an ACAPI Application					0: closeAccepted
	1: Monitoring of requested subscriber is not permitted					1: parameterError						
	2: Subscriber, part of subscriber block or fleet is not valid			2: unknownMonitoringHandle	
	3: Requested monitoring service is not available
	4: Unknown subscriber or fleet
	6: Entity already monitored by another gateway
	8: Reason unspecified
	9: requestedService was empty
	10: endSSI < beginAddress.ssi, or endSSI > maxSSI
	11: Maximum number of monitoring orders exceeded (TAP configuration)
	12: Rejected because parts of the order could not be fulfilled
	13: Rejected due to lack of resources
	14: Received monitoring PDU containing an invalid monHandle,
		or a monHandle not belonging to the serviceHandle given
	15: Target address must also contain country and network code (MCC/MNC)
	16: The monitored target is ambiguous
	17: The vpnNumber in the request message was invalid
	18: The monitoring service requested is not provided by this TAP
	19: The TAP has no connection to the SwMI
	20: The request is rejected due to the load situation in the infrastructure

	la dll no está respondiendo correctamente:
		reg_mon, debería ser 3 pero devuelve 0
		end_ssi, debería ser 2009 pero responde con 0
*/
void Consumidor::mon_order_status_callback( short tipo, System::String^ _contenido )
{
	System::Int32 app_handle, monHandle, resultado;

	try
	{		
		app_handle = System::Convert::ToInt32( _contenido->Substring( 2, 4 ), 16 );
		monHandle = System::Convert::ToInt32( _contenido->Substring( 6, 4 ), 16 );
		resultado	= System::Convert::ToInt32( _contenido->Substring( 10, 2 ), 16 );

		System::Int32 index_linea = find_index_by_handle( app_handle, MON_APP );
		System::Int32 index_mon;

		switch (tipo)
		{
			case 59:	// mon_orderclose	// monitorizacion de terminales acabada por el servidor acapi
				if ( monHandle == this->mon_reg_handle && this->lineas[this->index_first_disp]->mon->handle == app_handle )			// monitorizacion general
				{
					this->mon_reg_handle = -1;
					this->logger->Write( System::String::Format( "MON_ORDERCLOSE- monitorizacion con handle: {0} cerrado por motivo {1}", monHandle, resultado ), 2 );
				}
				else
				{
					index_mon = find_mon_index_by_handle( index_linea, monHandle );
					if ( index_mon >= 0 )
					{
						this->logger->Write( System::String::Format( "MON_ORDERCLOSE- monitorizacion cerrada para el despachador {0} con handle:{1} cerrado por motivo {2}",
							this->lineas[index_linea]->ssi_linea, monHandle, resultado ), 2 );
						this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}:{4}{5}",
								TRAMA_INI, AC_MOD_MON_SSI_ACK, this->lineas[index_linea]->ssi_linea, this->lineas[index_linea]->monitorizaciones[index_mon]->ssi_mon, 0, TRAMA_END ) );
						this->lineas[index_linea]->monitorizaciones->RemoveAt( index_mon );
					}
					else
					{
						this->logger->Write( System::String::Format( "MON_ORDERCLOSE- INCOHERENTE para el despachador {0} handle: {1} cerrado por motivo {2}",
							this->lineas[index_linea]->ssi_linea, monHandle, resultado ), 4 );
					}
					
				}
				break;
			case 60:	// mon_orderclose_ack
				if ( monHandle == this->mon_reg_handle && this->lineas[this->index_first_disp]->mon->handle == app_handle && resultado == 0 )
				{
					this->logger->Write( System::String::Format( "¡¡¡¡¡¡¡MON_ORDERCLOSE_ACK (quien ha cerrado??)- Confirmación desregistro con handle: {0}", monHandle ), 2 );
					this->mon_reg_handle = -1;
				}
				else
				{
					index_mon = find_mon_index_by_handle( index_linea, monHandle );
					if ( index_mon >= 0 )
					{
						if ( resultado == 0  )
						{
							this->logger->Write( System::String::Format( "MON_ORDERCLOSE_ACK- monitorizacion cerrada para el despachador {0} con handle:{1}",
								this->lineas[index_linea]->ssi_linea, monHandle ), 0 );
							this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}:{4}{5}",
								TRAMA_INI, AC_MOD_MON_SSI_ACK, this->lineas[index_linea]->ssi_linea, this->lineas[index_linea]->monitorizaciones[index_mon]->ssi_mon, 0, TRAMA_END ) );
							
							this->lineas[index_linea]->monitorizaciones->RemoveAt( index_mon );
						}
						else
						{
							this->logger->Write( System::String::Format( "MON_ORDERCLOSE_ACK- monitorizacion NO CERRADA cerrada para el despachador {0} con handle:{1} motivo {2}",
								this->lineas[index_linea]->ssi_linea, monHandle, resultado ), 4 );
							this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}:{4}{5}",
								TRAMA_INI, AC_MOD_MON_SSI_ACK, this->lineas[index_linea]->ssi_linea, this->lineas[index_linea]->monitorizaciones[index_mon]->ssi_mon, 1, TRAMA_END ) );
						}
					}
					else
					{
						this->logger->Write( System::String::Format( "MON_ORDERCLOSE_ACK- INCOHERENTE para el despachador {0} handle: {1} cerrado por motivo {2}",
							this->lineas[index_linea]->ssi_linea, monHandle, resultado ), 4 );
					}
				}
				break;
		}
	}
	catch (System::Exception^ e)
	{
		this->logger->Write( "Problema procesando mon_status_callback", 4 );
	}
}

/*
	Para evitar saturar al AcCore y a la base de datos con el initial briefing se avisa según "permisoAvisoMon" modificado en check_mon_reg()
*/
void Consumidor::logica_aviso_mod_mon( System::String^ msg, System::Int32 monHandle, System::Int32 t_ssi, System::DateTime^ lastUpdate )
{
	try
	{
		if ( this->permisoAvisoMon )				// si no se ha registrado el servicio hace poco => habrá muchos mensajes seguidos
		{
			this->manager->avisaAcCore(System::String::Format( "{0}{1}:{2}:{3}{4}", TRAMA_INI, FRT_AVISA_CAMBIOS, T_TERMINALES, t_ssi, TRAMA_END ) );
		}
		else
		{
			this->logger->Write( "mon_register_callback no anunciado al AcCore, initial briefing", -1 );
		}
		this->logger->Write( System::String::Format( "{0}. monHandle:{1} ssi: {2}  time: {3}", msg, monHandle, t_ssi, lastUpdate->ToString( "yyyy-MM-dd HH:mm:ss" ) ), 0 );
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "error en logica_aviso_mod_mon", 4 );
	}
}

/*
mon_location_update	app_handle	mon_handle	timeStamp	monSSI	locUpdate	appendLocArea	locArea
char				int			int			String		String	char		char			String
2					4			4			14			8		1			1				12

mon_location_detach	app_handle	mon_handle	timeStamp	monSSI	locArea
char				int			int			String		String	String
2					4			4			14			8		12

Errata Documentacion, en monSSI ponia que era de tipo char con una longitud de 1 byte
*/
void Consumidor::mon_register_callback( short tipo, System::String^ _contenido )
{
	System::Int32 ssi;
	System::Int32 app_handle, monHandle;
	System::DateTime^ lastUpdate;
	wchar_t locUpdate;
	wchar_t appendLocArea;
	System::String^ locArea;
	System::String^ msg;
	bool registrado;
	try
	{
		ssi				= System::Convert::ToInt32( _contenido->Substring( 24, 8 ), 16 );
		app_handle		= System::Convert::ToInt32( _contenido->Substring( 2, 4 ), 16 );
		monHandle		= System::Convert::ToInt32( _contenido->Substring( 6, 4 ), 16 );
		lastUpdate		= this->procesa_timeStamp ( _contenido->Substring( 10, 14 ) );

		// No se debería recibir ningun mon_location_update o mon_location_detach de un servicio de monitorizacion incorrecto!
		// Debería de haber un único servicio de monitorización de registro desregistro (manejado por el primer despachador)
		//		Debido al timing del acapi server es posible emitir dos o más ordenes Mon_order_req, en el caso de que los ACK tarden mucho en llegar
		//		habrá que cerrar los servicios de monitorizacion antiguos.
		if ( app_handle != this->lineas[this->index_first_disp]->mon->handle || monHandle != this->mon_reg_handle )
		{
			this->logger->Write( System::String::Format( "Se recibio un mon_register_callback de un servicio incorrecto! "
				"Deseado: app_h {0} mon_h {1} - Recibido: app_h {2} mon_h {3}",
				this->lineas[this->index_first_disp]->mon->handle, this->mon_reg_handle, app_handle, monHandle), 4 );
			// Se cierra el servicio de monitorización que no corresponde
			this->logger->Write( System::String::Format( "Se intenta cerrar el servicio de MON sobrante. app_h:{0} mon_h:{1}", app_handle, monHandle ), 2 );
			this->tetra->monitoring_close_order( app_handle, monHandle );
		}
		else
		{
			switch ( tipo )
			{
				case 12:	// mon_location_update
					locUpdate		= _contenido[32];
					appendLocArea	= _contenido[33];
					locArea			= _contenido->Substring( 34, 12 );
					msg = "LOCATION_UPDATE";
					registrado = true;
					break;
				case 13:	// mon_location_detach
					locArea			= _contenido->Substring( 32, 12 );
					msg = "LOCATION_DETACH";
					registrado = false;
					break;
			}
			if ( this->db->update_status ( ssi, lastUpdate, registrado, locArea ) )
			{
				this->logica_aviso_mod_mon( msg, monHandle, ssi, lastUpdate );
			}
			else
			{
				throw gcnew System::Exception();
			}
		}
	}
	catch (System::Exception^ e)
	{
		this->logger->Write( "ocurrió algún error procesando register_update", 4 );
	}
}

/*
mon_cc_information	app_handle	mon_handle	timeStamp	target_ssi	source_ssi	callID	callState	hookSignalling	duplexMode
char				int			int			String		String		String		int		char		char			char
2					4			4			14			8			8			4		1			1				1
*/
void Consumidor::mon_cc_info_callback ( System::String^ _contenido )
{
	try
	{
		System::Int32	app_handle, mon_handle, target_ssi, source_ssi, call_id, call_state;
		bool hook_signalling, duplex_call;
		System::DateTime^ time_stamp;

		app_handle			= System::Convert::ToInt32( _contenido->Substring( 2, 4 ), 16 );
		mon_handle			= System::Convert::ToInt32( _contenido->Substring( 6, 4 ), 16 );
		time_stamp			= this->procesa_timeStamp( _contenido->Substring( 10, 14 ) );
		target_ssi			= System::Convert::ToInt32( _contenido->Substring( 24, 8 ),  16 );
		source_ssi			= System::Convert::ToInt32( _contenido->Substring( 32, 8 ), 16 );
		call_id				= System::Convert::ToInt32( _contenido->Substring( 40, 4 ), 16 );
		call_state			= System::Convert::ToInt32( _contenido->Substring( 44, 1 ), 16 );
		hook_signalling		= ( System::Convert::ToInt32( _contenido->Substring( 45, 1 ), 16 ) == 1 ) ? true : false;
		duplex_call			= ( System::Convert::ToInt32( _contenido->Substring( 46, 1 ), 16 ) == 1 ) ? true : false;

		System::Int32 index_linea = find_index_by_handle( app_handle, MON_APP );
		System::Int32 index_mon = find_mon_index_by_handle( index_linea, mon_handle );

		this->logger->Write( System::String::Format( ":_:_: MON_CC_INFO [[{0}]] monHandle:{1} target:{2} src:{3} callId:{4} state:{5} hook?{6} dup?{7} time:{8}",
					this->lineas[index_linea]->ssi_linea, mon_handle, target_ssi, source_ssi, call_id, call_state, hook_signalling, duplex_call, time_stamp ), 0 );

		if ( index_mon >= 0 )
		{
			this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->ssi_destino	= target_ssi;
			this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->ssi_origen	= source_ssi;
			this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->id			= call_id;
			this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->estado		= call_state;
			this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->tipo		= duplex_call ? CC_TIPO_DUPLEX : CC_TIPO_NO_DUPLEX;

			print_estado_mon_ssi( index_linea, index_mon );

			if ( call_state == MON_CALL_CONNECT )
			{
				this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}:{4}:{5}:{6}{7}",
								TRAMA_INI, AC_MON_CALL_CONN, this->lineas[index_linea]->ssi_linea, this->lineas[index_linea]->monitorizaciones[index_mon]->ssi_mon,
								duplex_call ? 1:0, source_ssi, target_ssi, TRAMA_END ) );
			}
		}
		else
		{
			this->logger->Write( System::String::Format( ":_:_: MON_CC_INFO [[{0}]] monHandle:{1} no ENCONTRADO!",
					this->lineas[index_linea]->ssi_linea, mon_handle ), 2 );
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en mon_cc_info_callback", 4 );
	}
}

/*
mon_disconnect	app_handle	mon_handle	timeStamp	disconnectedPartySSI	callID	disconnectCause
char			int			int			String		String					int		short
2				4			4			14			8						4		2
*/
void Consumidor::mon_disconnect_callback ( System::String^ _contenido )
{
	try
	{
		System::Int32	app_handle, mon_handle, disconnected_ssi, call_id, disconnect_cause;
		System::DateTime^ time_stamp;


		app_handle			= System::Convert::ToInt32( _contenido->Substring( 2, 4 ), 16 );
		mon_handle			= System::Convert::ToInt32( _contenido->Substring( 6, 4 ), 16 );
		time_stamp			= this->procesa_timeStamp( _contenido->Substring( 10, 14 ) );
		disconnected_ssi	= System::Convert::ToInt32( _contenido->Substring( 24, 8 ),  16 );
		call_id				= System::Convert::ToInt32( _contenido->Substring( 32, 4 ), 16 );
		disconnect_cause	= System::Convert::ToInt32( _contenido->Substring( 36, 2 ), 16 );

		System::Int32 index_linea = find_index_by_handle( app_handle, MON_APP );
		System::Int32 index_mon = find_mon_index_by_handle( index_linea, mon_handle );

		this->logger->Write( System::String::Format( ":_:_: MON_DISCONNECT [[{0}]] monHandle:{1} disconnected:{2} callId:{3} CAUSE:{4}  time:{5}",
					this->lineas[index_linea]->ssi_linea, mon_handle, disconnected_ssi, call_id, disconnect_cause, time_stamp ), 0 );

		if( index_mon >= 0 )
		{
			this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion = gcnew Comunicacion_tetra( -1, -1, -1 );
			this->lineas[index_linea]->monitorizaciones[index_mon]->interviniendo = false;
			
			this->print_estado_mon_ssi( index_linea, index_mon );

			this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}{4}",
								TRAMA_INI, AC_MON_CALL_DISCONN, this->lineas[index_linea]->ssi_linea, this->lineas[index_linea]->monitorizaciones[index_mon]->ssi_mon, TRAMA_END ) );
		}
		else
		{
			this->logger->Write( System::String::Format( ":_:_: MON_DISCONNECT INCOHERENTE [[{0}]] monHandle:{1}",
					index_linea, mon_handle ), 2 );
		}

	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en mon_disconnect_callback", 4 );
	}
}

/*
mon_disconnect	app_handle	mon_handle	call_id		result
char			int			int			int			short
2				4			4			4			1
result:
0 = forcedCallEndOK
1 = failedUnspecified
2 = deniedNoPermission
3 = deniedWrongCallID
4 = currentlyNotAvailable
*/
void Consumidor::mon_forced_call_end_ack_callback ( System::String^ _contenido )
{
	try
	{
		System::Int32	app_handle, mon_handle, call_id, result;


		app_handle			= System::Convert::ToInt32( _contenido->Substring( 2, 4 ), 16 );
		mon_handle			= System::Convert::ToInt32( _contenido->Substring( 6, 4 ), 16 );
		call_id				= System::Convert::ToInt32( _contenido->Substring( 10, 4 ), 16 );
		result				= System::Convert::ToInt32( _contenido->Substring( 14, 1 ), 16 );

		// No se hace ningun check, se recibirá un mon_cc_disconnect si la orden fue correcta
		System::Int32 index_linea = find_index_by_handle( app_handle, MON_APP );
		System::Int32 index_mon = find_mon_index_by_handle( index_linea, mon_handle );

		if( index_mon >= 0 )
		{
			if ( result == 0 )
			{
				this->logger->Write( System::String::Format( ":_:_: mon_forced_call_end_ack_callback OK en [[{0}]] monHandle:{1} callId:{2}",
					this->lineas[index_linea]->ssi_linea, mon_handle, call_id ), 1 );
			}
			else
			{
				this->logger->Write( System::String::Format( ":_:_: mon_forced_call_end_ack_callback INCORRECTO en [[{0}]] monHandle:{1} callId:{2} resultado:{3}",
					this->lineas[index_linea]->ssi_linea, mon_handle, call_id, result ), 4 );
			}
		}
		else
		{
			this->logger->Write( System::String::Format( ":_:_: mon_forced_call_end_ack_callback INCOHERENTE [[{0}]] monHandle:{1}",
					index_linea, mon_handle ), 2 );
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en mon_disconnect_callback", 4 );
	}
}

/*
	YYYYMMDDHHmmss
*/
System::DateTime^ Consumidor::procesa_timeStamp( System::String^ timeStamp )
{
	return gcnew System::DateTime
		(
		System::Convert::ToInt32( timeStamp->Substring( 0, 4 ) ),		// YYYY
		System::Convert::ToInt32( timeStamp->Substring( 4, 2 ) ),		// MM
		System::Convert::ToInt32( timeStamp->Substring( 6, 2 ) ),		// DD
		System::Convert::ToInt32( timeStamp->Substring( 8, 2 ) ),		// HH
		System::Convert::ToInt32( timeStamp->Substring( 10, 2 ) ),		// mm
		System::Convert::ToInt32( timeStamp->Substring( 12, 2 ) )		// ss
		);
}

/*
	Al hacer una asignación de grupo se pueden añadir decenas de terminales a un grupo, por cada modificación de pertenencias se avisaría
	a los operadores que acceden a la base de datos para cargar todas las pertenencias de cada grupo. Previsiblemente inmediatamente después
	de la asignación habrá muchas modificaciones en	las pertenencias.
	Se sigue una lógica por la cual se espera un cierto número de ciclos de timer para notificar cualquier cambio en las pertenencias del último
	grupo que se modifico.
	this->permisoAvisoDgna se actualiza en el timer, en "check_logica_aviso_dgna"
*/
void Consumidor::logica_aviso_mod_dgna( System::Int32 group_ssi )
{
	try
	{
		if ( this->permisoAvisoDgna )
		{
			this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}{4}", TRAMA_INI, FRT_AVISA_CAMBIOS, T_PERTENENCIAS, group_ssi, TRAMA_END ) );
			this->last_dgna_group = group_ssi;
		}
		else if ( ( group_ssi != this->last_dgna_group ) && this->last_dgna_group != -1 )				// Si se quiere modificar un grupo distinto antes que acabe el periodo del anterior
		{
			// aviso al ultimo grupo dgna, reinicio la cuenta de ciclos timer tras la cual se avisará al nuevo grupo dgna
			this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}{4}", TRAMA_INI, FRT_AVISA_CAMBIOS, T_PERTENENCIAS, this->last_dgna_group, TRAMA_END ) );
			this->last_dgna_group = group_ssi;
			this->dgna_espera = 0;
		}
		else if ( this->last_dgna_group == -1 )
		{
			this->last_dgna_group = group_ssi;
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "Ocurrio un fallo en logica_aviso_mod_dgna", 4 );
	}
}

/*
Callback recibido con el progreso/resultado de un add o del de un ssi a un grupo dinámico
	La base de datos solo se actualizará si se ha podido llevar a cabo la acción (del o add)

7: dyn_group_add_ack. RESULT:
		assignmentRejectedForAnyReason						0
		assignmentAccepted									1
		assignmentNotAcceptedForSecurityReasons				2
		assignmentRejectedForCapacityExceededReasons		3
		assignmentRejectedSSNotSupportedByMS				4
		assignmentRejectedDgnaActionNotSupportedByMS		5
		assignmentRejectedNoAnswerFromMS					6
		assignmentRejectedMSIsBusy							7
		assignmentRejectedMSNotAllowedToRecvAssignments		8
		assignmentRejectedGroupNotAllowedToBeAssigned		9
		assignmentProceeding								10
		assignmentRejectedGeneralError						11
		assignmentRejectedUnknownSSITSI						12
		assignmentRejectedMSLSIsNotRegistered				13
		assignmentRejectedRoutingProblemInInfrastructure	14
		assignmentRejectedMessageConsistencyError			15

8: dyn_group_del_ack. RESULT:
		groupDetached										0
		groupDetachedAndDeassigned							1
		deassignmentRejectedSSNotSupportedByMS				4
		deassignmentRejectedDgnaActionNotSupportedByMS		5
		deassignmentRejectedNoAnswerFromMS					6
		deassignmentRejectedMSBusy							7
		deassignmentRejectedMSNotAllowedToRecvAssignments	8
		deassignmentRejectedGroupNotAllowedToBeDeassigned	9
		deassignmentProceeding								10
		deassignmentRejectedGeneralError					11
		deassignmentRejectedUnknownSSITSI					12
		deassignmentRejectedMSLSIsNotRegistered				13
		deassignmentRejectedRoutingProblemInInfrastructure	14
		deassignmentRejectedMessageConsistencyError			15

-------------------trama---------------------------
X_ack	app_handle	group_ssi	call_party_ssi	result
char	int			String		String			char
2 		4			8			8				1
*/
void Consumidor::dyn_group_callback( short tipo, System::String^ _contenido )
{
	System::Int32 app_handle;
	System::Int32 individual_ssi;
	System::Int32 group_ssi;
	System::Int32 result;
	Pertenencia^ pertenencia_previa;
	System::Int32 estado_previo;

	System::Int32 index_linea;

	try
	{
		app_handle		= System::Convert::ToInt32( _contenido->Substring( 2, 4 ), 16 );

		if ( app_handle != this->lineas[this->index_first_disp]->ss->handle )
		{
			this->logger->Write( "Recibido group callback desde un app handle incorrecto!", 4 );
			throw gcnew System::Exception();
		}

		individual_ssi	= System::Convert::ToInt32( _contenido->Substring( 14, 8 ), 16 );
		group_ssi		= System::Convert::ToInt32( _contenido->Substring( 6, 8 ), 16 );
		result			= System::Convert::ToInt32( _contenido->Substring( 22, 1 ), 16 );
		
		pertenencia_previa = this->db->read_pertenencia( group_ssi, individual_ssi );
		estado_previo = pertenencia_previa->estado;

		switch ( tipo )
		{
			case 7:		// dyn_group_add_ack
				
				if ( tipo = 7 && result == 1 )		// si ( dyn_group_add_ack and assignmentAccepted )
				{
					this->logger->Write( System::String::Format( "DYN_GROUP_ADD_ACK, assignmentAccepted.group: {0}, individual: {1}",
						group_ssi, individual_ssi ), 0 );
					switch ( estado_previo )
					{
						case PERTENENCIA_ERROR:
							throw gcnew System::Exception();
							break;

						case PERTENENCIA_NO_EXISTE:
							if ( ! this->db->update_pertenencias( group_ssi, individual_ssi, PERTENENCIA_OK, DB_INSERT ) )
								throw gcnew System::Exception();
							this->logica_aviso_mod_dgna( group_ssi );
							break;

						case PERTENENCIA_PENDIENTE_AÑADIR:
						case PERTENENCIA_AÑADIR_FALLO:
						case PERTENENCIA_PENDIENTE_BORRAR:
						case PERTENENCIA_BORRAR_FALLO:
							if ( ! this->db->update_pertenencias( group_ssi, individual_ssi, PERTENENCIA_OK, DB_UPDATE ) )
								throw gcnew System::Exception();
							this->logica_aviso_mod_dgna( group_ssi );
							break;

						case PERTENENCIA_OK:
							// no hago nada
							this->logger->Write( System::String::Format( "DYN_GROUP_ADD_ACK, assignmentAccepted cuando ya había una PERTENENCIA OK. group: {0}, individual: {1}",
								group_ssi, individual_ssi ), 2 );
							break;

						default:
							this->logger->Write( System::String::Format( "DYN_GROUP_ADD_ACK, estado_previo_desconocido!! - {0}", estado_previo ), 4 );
							break;
					}
				}
				else
				{
					this->logger->Write( System::String::Format( "DYN_GROUP_ADD_ACK, group: {0}, individual: {1} result: {2}",
						group_ssi, individual_ssi, result ), 0 );
				}
				break;
			case 8:		// dyn_group_del_ack
				if ( tipo = 8 && ( result == 1 ) )		// result = 1 (Detached) NO implica que el terminal deje de pertenecer al grupo!!
				{
					this->logger->Write( System::String::Format( "DYN_GROUP_DEL_ACK, groupDetached.group: {0}, individual: {1}",
						group_ssi, individual_ssi ), 0 );
					switch ( estado_previo )
					{
						case PERTENENCIA_ERROR:
							throw gcnew System::Exception();
							break;

						case PERTENENCIA_NO_EXISTE:
							// no hago nada
							this->logger->Write( System::String::Format( "DYN_GROUP_DEL_ACK, callback detached cuando no habia pertenencia!!!! group: {0}, individual: {1}",
								group_ssi, individual_ssi ), 2 );
							break;

						case PERTENENCIA_PENDIENTE_AÑADIR:
						case PERTENENCIA_AÑADIR_FALLO:
						case PERTENENCIA_PENDIENTE_BORRAR:
						case PERTENENCIA_BORRAR_FALLO:
						case PERTENENCIA_OK:
							if ( ( estado_previo !=  PERTENENCIA_PENDIENTE_BORRAR ) )			// caso incoherente deberia de pasar primero por el estado pendiente_borrar
								this->logger->Write( System::String::Format( "DYN_GROUP_DEL_ACK, callback detached con estado previo !=PENDIENTE_BORRAR !!¡¡¡ group: {0}, individual: {1} estado {2}",
								group_ssi, individual_ssi, estado_previo ), 2 );
							if ( ! this->db->update_pertenencias( group_ssi, individual_ssi, PERTENENCIA_PENDIENTE_BORRAR, DB_DELETE ) )
								throw gcnew System::Exception();
							//if ( estado_previo != PERTENENCIA_PENDIENTE_BORRAR )		// los casos pendientes de borrar no se muestran en los operadores!! => no hace falta que los operadores sepan que estan realmente borrados
							// aviso en cualquier caso mejor
							this->logica_aviso_mod_dgna( group_ssi );
							break;

						default:
							this->logger->Write( System::String::Format( "DYN_GROUP_DEL_ACK, estado_previo_desconocido!! - {0}", estado_previo ), 4 );
							break;
					}

					// comprobar si la pertenencia borrada envuelve a alguna linea attached, si es así cambiar el grupo asociado al defecto (si no existe se cambiará a vacío)
					index_linea = find_index_by_ssi( individual_ssi );
					if ( index_linea >= 0 )
					{
						if ( this->lineas[index_linea]->ssi_grupo_asociado == group_ssi )
						{
							this->logger->Write( System::String::Format( "Pertenencia BORRADA afecta a linea {0}, se intenta ATTACH al grupo por defecto {1}",
								index_linea, this->lineas[index_linea]->ssi_grupo_defecto ), 1 );

							this->lineas[index_linea]->ssi_grupo_asociado = -1;			// Si no se consigue cambiar al grupo por defecto la linea se quedará vacía!
							this->lineas[index_linea]->attached = false;
							this->cambia_grupo_asociado( System::String::Format( "{0}:{1}", index_linea, this->lineas[index_linea]->ssi_grupo_defecto ) );
						}
					}
				}
				else
				{
					this->logger->Write( System::String::Format( "DYN_GROUP_DEL_ACK, group: {0}, individual: {1} result: {2}", group_ssi, individual_ssi, result ), 1 );
				}
				break;
		}
	}
	catch (System::Exception^ e)
	{
		this->logger->Write( "ocurrió algún error procesando DYN_group_callback", 4 );
	}
}

/*-------------------trama DOCUMENTACION---------------------------
X_ack	app_handle	group_ssi	call_party_ssi	result
char	int			String		String			char
2 		4			8			8				1

-------------------trama OBSERVADA!!!!!---------------------------
X_ack	app_handle	call_party_ssi	group_ssi		result
char	int			String			String			char
2 		4			8				8				1

36: dyn_group_attach_ack. RESULT: |0 succesful or already attached|1 denied|2 denied because of ssi invalid|3 duplicate group attachment|
37: dyn_group_detach_ack. RESULT: |0 succesful or not previously attached|1 denied|
*/
void Consumidor::group_callback( short tipo, System::String^ _contenido )
{
	System::Int32 app_handle;
	System::Int32 individual_ssi;
	System::Int32 group_ssi;
	System::Int32 result;
	System::Int32 index_linea;

	try
	{
		app_handle		= System::Convert::ToInt32( _contenido->Substring( 2, 4 ), 16 );

		individual_ssi	= System::Convert::ToInt32( _contenido->Substring( 6, 8 ), 16 );
		group_ssi		= System::Convert::ToInt32( _contenido->Substring( 14, 8 ), 16 );
		result			= System::Convert::ToInt32( _contenido->Substring( 22, 1 ), 16 );

		index_linea = find_index_by_handle( app_handle, SS_APP );

		switch ( tipo )
		{
			case 36:	// group_attach_ack
				// si (dyn_group_attach_ack and succesful and already attached o duplicate group attachment)
				if ( tipo = 36 && ( result == 0 || result == 3 ) )
				{
					this->logger->Write( System::String::Format( "GROUP_ATTACH_ACK, assignmentAccepted linea {0} group {1} individual {2}", index_linea, group_ssi, individual_ssi ), 0 );
					
					if ( result == 3 )
						this->logger->Write( "GROUP_ADD_ACK, result: 3 ===>>> duplicate group attachment", 1 );
					
					if ( ! this->db->update_attached ( group_ssi, individual_ssi, PER_ATTACHED ) )		// Si la pertenencia no existe! (NO DEBERIA OCURRIR)
						this->logger->Write( System::String::Format( "....GROUP_ATTACH_ACK OK a una pertenencia que NO EXISTE!! group: {0}, individual: {1} result: {2}", group_ssi, individual_ssi, result ), 2 );

					if ( this->lineas[index_linea]->ssi_grupo_asociado == group_ssi )
					{
						this->lineas[index_linea]->attached = true;
						this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}:{4}{5}",
								TRAMA_INI, CAMBIA_GRUPO_LINEA_ACK, index_linea,this->lineas[index_linea]->ssi_grupo_asociado,
								this->lineas[index_linea]->attached ? 1 : 0, TRAMA_END ) );
					}
					else
					{
						this->logger->Write( System::String::Format( "linea {0} attached a un grupo {1} distinto del asociado {2}", index_linea, group_ssi, this->lineas[index_linea]->ssi_grupo_asociado ), 2 );
					}
				}
				else
				{
					this->logger->Write( System::String::Format( "[[[[]]]]  GROUP_ATTACH_ACK, group: {0}, individual: {1} result: {2}", group_ssi, individual_ssi, result ), 0 );
				}
				break;
			case 37:	// dyn_group_detach_ack
				if ( tipo = 37 && result == 0 )
				{
					this->logger->Write( System::String::Format( "GROUP_DETACH_ACK, groupDetached linea {0} group {1} individual {2}", index_linea, group_ssi, individual_ssi ), 0 );
					if ( ! this->db->update_attached ( group_ssi, individual_ssi, PER_DETACHED ) )		// Si la pertenencia no existe! (NO DEBERIA OCURRIR)
						this->logger->Write( System::String::Format( "....GROUP_DETACH_ACK OK a una pertenencia que NO EXISTE!! group: {0}, individual: {1} result: {2}", group_ssi, individual_ssi, result ), 2 );
					
					if ( this->lineas[index_linea]->ssi_grupo_asociado == group_ssi )
					{
						this->lineas[index_linea]->attached = false;
						this->lineas[index_linea]->ssi_grupo_asociado = -1;

						this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}:{4}{5}",
								TRAMA_INI, CAMBIA_GRUPO_LINEA_ACK, index_linea, this->lineas[index_linea]->ssi_grupo_asociado,
								this->lineas[index_linea]->attached ? 1 : 0, TRAMA_END ) );
					}
					else
					{
						this->logger->Write( System::String::Format( "linea {0} detached de un grupo {1} distinto del asociado {2}", index_linea, group_ssi, this->lineas[index_linea]->ssi_grupo_asociado ), 2 );
					}
				}
				else
				{
					this->logger->Write( System::String::Format( "[[[[]]]]  GROUP_DETACH_ACK, group: {0}, individual: {1} result: {2}", group_ssi, individual_ssi, result ), 0 );
				}
				break;
		}
	}
	catch (System::Exception^ e)
	{
		this->logger->Write( "ocurrió algún error procesando group_callback", 4 );
	}
}

/*
	Para cada terminal que se quiere añadir/borrar de un grupo se chequea si ya existe una pertenencia y su estado, y se actúa en consecuencia
*/
void Consumidor::modifica_grupo_pertenencias( bool estatico, bool add, System::Int32 group_ssi, System::Int32 individual_ssi )
{
	int app_handle;
	Pertenencia^ pertenencia_previa;
	System::Int32 estado_previo;
	try
	{
		if ( estatico && ( individual_ssi >= this->lineas[0]->ssi_linea  || individual_ssi <= this->lineas[lineas->Length - 1]->ssi_linea ) )
			throw gcnew System::Exception();			// intento añadir un ssi que no es ni de operadores ni de despachadores a un grupo estático

		pertenencia_previa = this->db->read_pertenencia( group_ssi, individual_ssi );
		estado_previo = pertenencia_previa->estado;

		// todas las ordenes ss se mandan desde la linea del primer despachador
		app_handle = this->lineas[this->index_first_disp]->ss->handle;

		switch( estado_previo )
		{
			case PERTENENCIA_ERROR:
				throw gcnew System::Exception();
				break;

			case PERTENENCIA_NO_EXISTE:
				if ( add )
				{
					if ( this->db->update_pertenencias( group_ssi, individual_ssi, PERTENENCIA_PENDIENTE_AÑADIR, DB_INSERT ) )
					{
						/*if ( ! tetra->modifica_pertenencias( app_handle, estatico, add, group_ssi, individual_ssi ) )
							throw gcnew System::Exception();*/
						
						this->permisoAvisoDgna = false;
						this->logica_aviso_mod_dgna( group_ssi );
					}
				}
				else
				{
					// no hacer nada, quitar una pertenencia que no existe!!!
					this->logger->Write( System::String::Format( "Se ha intentado quitar una pertenencia que no existe. grupo: {0} individuo: {1}", group_ssi, individual_ssi ), 2 );
				}
				break;

			case PERTENENCIA_OK:
				if ( add )
				{
					// no hacer nada, añadir una pertenencia que ya existe!!!
					this->logger->Write( System::String::Format( "Se ha intentado añadir una pertenencia que ya existe. grupo: {0} individuo: {1}", group_ssi, individual_ssi ), 2 );
				}
				else
				{
					if ( this->db->update_pertenencias( group_ssi, individual_ssi, PERTENENCIA_PENDIENTE_BORRAR, DB_UPDATE ) )
					{
						/*if ( ! tetra->modifica_pertenencias( app_handle, estatico, add, group_ssi, individual_ssi ) )
							throw gcnew System::Exception();*/

						this->permisoAvisoDgna = false;
						this->logica_aviso_mod_dgna( group_ssi );
					}
				}
				break;

			case PERTENENCIA_PENDIENTE_AÑADIR:
				if ( add )
				{
					// no hacer nada, añadir una pertenencia que esta pendiente de añadir!!!
					this->logger->Write( System::String::Format( "Se ha intentado añadir una pertenencia que ya estaba pendiente de añadir. grupo: {0} individuo: {1}", group_ssi, individual_ssi ), 2 );
				}
				else
				{
					// Si la pertenencia esta pendiente_añadir y quiero borrarla, basta con eliminar la entrada de la base de datos, ya que el ssi individual no pertenece al ssi grupo
					if ( ! this->db->update_pertenencias( group_ssi, individual_ssi, PERTENENCIA_PENDIENTE_BORRAR, DB_DELETE ) )
						throw gcnew System::Exception();
					else
					{
						this->permisoAvisoDgna = false;
						this->logica_aviso_mod_dgna( group_ssi );
					}
				}
				break;

			case PERTENENCIA_PENDIENTE_BORRAR:
			case PERTENENCIA_BORRAR_FALLO:
				if ( add )
				{
					// una pertenencia que esta pendiente ser borrada (el ssi aun pertenece al grupo) se quiere añadir de nuevo => se deja en PERTENENCIA_OK
					if ( ! this->db->update_pertenencias( group_ssi, individual_ssi, PERTENENCIA_OK, DB_UPDATE ) )
						throw gcnew System::Exception();
					else
					{
						this->permisoAvisoDgna = false;
						this->logica_aviso_mod_dgna( group_ssi );
					}
				}
				else
				{
					// no hacer nada, borrar una pertenencia que esta pendiente de ser borrada!!!
					this->logger->Write( System::String::Format( "Se ha intentado borrar una pertenencia que ya estaba pendiente de ser borrada. grupo: {0} individuo: {1}", group_ssi, individual_ssi ), 2 );
				}
				break;

			case PERTENENCIA_AÑADIR_FALLO:
				if ( add )
				{
					// una pertenencia que no se pudo añadir (el ssi no pertenece al grupo) se quiere añadir de nuevo => se volvera a intentar el numero de intentos correspondiente
					if ( ! this->db->update_pertenencias( group_ssi, individual_ssi, PERTENENCIA_PENDIENTE_AÑADIR, DB_UPDATE ) )
						throw gcnew System::Exception();
					else
					{
						this->permisoAvisoDgna = false;
						this->logica_aviso_mod_dgna( group_ssi );
					}
				}
				else
				{
					// Si la pertenencia no se pudo añadir y quiero borrarla, basta con eliminar la entrada de la base de datos, ya que el ssi individual no pertenece al ssi grupo
					if ( ! this->db->update_pertenencias( group_ssi, individual_ssi, PERTENENCIA_PENDIENTE_BORRAR, DB_DELETE ) )
						throw gcnew System::Exception();
					else
					{
						this->permisoAvisoDgna = false;
						this->logica_aviso_mod_dgna( group_ssi );
					}
				}
				break;
			case PERTENENCIA_ESTATICA:
				this->logger->Write( "Se ha intentado modificar una pertenencia ESTATICA!!! se ignora", 4 );
				break;

			default:
				this->logger->Write( System::String::Format( "Se ha intentado modificar una pertenencia con un estado previo desconocido!!! - {0}", estado_previo ), 4 );
				break;
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema añadiendo ssi particular en modifica_grupo_pertenencias()", 4 );
	}
}

/*
Orden de un operador de agregar o eliminar ssis a un grupo dinámico.

	[A-add, D-delete]:[num affected ssi]:[S-static, D-dynamic]:[group ssi]:[ssi 1]: ... :[ssi z]

	caso particular 1=>(add && num affected ssi == -1), para añadir un grupo entero ( todos los miembros del grupo [ssi 1] al [group ssi] )
	caso particular 1=>(delete && num affected ssi == -1), para borrar todos los miembros del [group ssi] ( [ssi 1] = -1 )

	*campo [S-static, D-dynamic] ignorado, siempre considerado dynamic
*/
void Consumidor::modifica_grupo( System::String^ _contenido )
{
	try
	{
		array<System::String^>^ campos = nullptr;
		array<wchar_t>^ separador = {':'};
		campos = _contenido->Split( separador, System::StringSplitOptions::RemoveEmptyEntries );

		bool add, estatico;
		// add?
		if ( campos[0]->Equals( "A" ) )
			add = true;
		else if ( campos[0]->Equals( "D" ) )
			add = false;
		else
			throw gcnew System::Exception();

		/*
		// static?
		if ( campos[2]->Equals( "S" ) )
			estatico = true;
		else if ( campos[2]->Equals( "D" ) )
			estatico = false;
		else
			throw gcnew System::Exception();
		*/
		estatico = false;			// Forzado a false, los attach y detach serán únicamente para los ssi aplicación del 105. check_attached!!

		System::Int32 num_ssis = System::Convert::ToInt32( campos[1] );
		System::Int32 group_ssi = System::Convert::ToInt32( campos[3] );

		System::Int32 individual_ssi;

		// Se quiere añadir un grupo !!
		if ( num_ssis == -1 )
		{
			ArrayList^ terminales;
			
			if( add )
				terminales = this->db->read_all_pertenencias_grupo( System::Convert::ToInt32( campos[ 4 ] ) );		// campos[4] contendrá el ssi del grupo a añadir
			else
				terminales = this->db->read_all_pertenencias_grupo( System::Convert::ToInt32( campos[ 3 ] ) );		// campos[3] contendrá el ssi del propio grupo a vaciar

			if ( terminales != nullptr )
			{
				IEnumerator^ e = terminales->GetEnumerator();
				while( e->MoveNext() )
				{
					individual_ssi = safe_cast<System::Int32>( e->Current );
					this->modifica_grupo_pertenencias( estatico, add, group_ssi, individual_ssi );
				}
			}
		}
		// Si efectivamente hay tantos ssis como se especifica
		else if ( num_ssis <= ( campos->Length - 4 ) && num_ssis > 0 )
		{
			for ( int i = 0; i < num_ssis; i++ )
			{
				individual_ssi = System::Convert::ToInt32( campos[ 4 + i ] );
				this->modifica_grupo_pertenencias( estatico, add, group_ssi, individual_ssi );
			}
		}
		else
			throw gcnew System::Exception();
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema general en modifica_grupo()", 4 );
	}
}

/*
	Imprime el estado completo de una linea para poder monitorizar los cambios en CC
*/
void Consumidor::print_estado_linea( int indice )
{
	try
	{
		Comunicacion_tetra^ llamada = this->lineas[indice]->comunicacion;
		System::String ^tipo, ^estado;
		switch ( llamada->tipo )
		{
		case -1:
				tipo = "UNDEF";
				break;
			case CC_TIPO_POINTTOPOINT:
				tipo = "POINTtoPOINT";
				break;
			case CC_TIPO_POINTTOMULTIPOINT:
				tipo = "POINTtoMULTIPOINT";
				break;
			case CC_TIPO_POINTTOMULTIPOINTACK:
				tipo = "POINTtoMULTIPOINTack";
				break;
			case CC_TIPO_BROADCAST:
				tipo = "BROADCAST";
				break;
			case CC_TIPO_DUPLEX:
				tipo = "DUPLEX";
				break;
			case CC_AMBIENCE_LISTEN:
				tipo = "AMBIENCE_LISTEN";
				break;
		}
		switch ( llamada->estado )
		{
			case CC_EMPTY:
				estado = "EMPTY";
				break;
			case CC_PROGRESSING:
				estado = "PROGRESSING";
				break;
			case CC_QUEUED:
				estado = "QUEUED";
				break;
			case CC_SUBS_PAGED:
				estado = "SUBS_PAGED";
				break;
			case CC_RINGING:
				estado = "RINGING";
				break;
			case CC_CONTINUE:
				estado = "CONTINUE";
				break;
			case CC_HANG_EXPIRED:
				estado = "HANG_EXPIRED";
				break;
			case CC_ORDER_SENT:
				estado = "ORDER_SENT";
				break;
			case CC_ACKED:
				estado = "ACKED";
				break;
			case CC_ESTABLISHED:
				estado = "ESTABLISHED";
				break;
			case CC_RECEIVED:
				estado = "RECEIVED";
				break;
			case CC_ACCEPTING:
				estado = "ACCEPTING";
				break;
			case CC_HANGING_UP:
				estado = "HANGING_UP";
				break;
		}
		this->logger->Write( System::String::Format ( "|[{0}]|||{1}||{2}||pinchada:{3}||prior:{4}||ssi:{5}||id:{6}||orig:{7}||dst:{8}||ip:{9}||port:{10}||allowtx:{11}||tx?{12}||exist_tx:{13}->{14}",
			indice, tipo, estado, this->lineas[indice]->pinchada, llamada->priority, this->lineas[indice]->ssi_linea, llamada->id, llamada->ssi_origen, llamada->ssi_destino,
			llamada->acapi_ip, llamada->acapi_rtp_port, llamada->tx_req_permission, llamada->tx_granted, llamada->exist_tx, llamada->tx_ssi ), 3 );
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( System::String::Format ( "problema en print_estado_linea, para la linea {0}", indice ), 4 );
	}
}

/*
	Imprime el estado completo de un ssi monitorizado
*/
void Consumidor::print_estado_mon_ssi( System::Int32 index_linea, System::Int32 index_mon )
{
	try
	{
		Monitorizacion_item^ mon_item = this->lineas[index_linea]->monitorizaciones[index_mon];
		Comunicacion_tetra^ llamada = mon_item->comunicacion;

		System::Int32 warning_level;

		System::String ^tipo, ^estado;

		switch ( llamada->tipo )
		{
			case -1:
				tipo = "UNDEF";
				break;
			case CC_TIPO_DUPLEX:
				tipo = "DUPLEX";
				break;
			case CC_TIPO_NO_DUPLEX:
				tipo = "NO_DUPLEX";
				break;
			default:
				tipo = "ERROR!";
				break;
		}
		switch ( llamada->estado )
		{
			case MON_EMPTY:
				estado = "EMPTY";
				break;
			case MON_CALL_SETUP:
				estado = "CALL_SETUP";
				break;
			case MON_CALL_SETUP_REJECT:
				estado = "CALL_SETUP_REJECT";
				break;
			case MON_ALERT_FROM_CALLED_PARTY:
				estado = "ALERT_FROM_CALLED_PARTY";
				break;
			case MON_ACCEPT_FROM_CALLED_PARTY:
				estado = "ACCEPT_FROM_CALLED_PARTY";
				break;
			case MON_CALL_CONNECT:
				estado = "CALL_CONNECT";
				break;
			case MON_INCLUDE_CALL_SETUP:
				estado = "INCLUDE_CALL_SETUP";
				break;
			case MON_INCLUDE_CALL_SETUP_REJECT:
				estado = "INCLUDE_CALL_SETUP_REJECT";
				break;
			case MON_INCLUDE_ACCEPT_FROM_CALLED_PARTY:
				estado = "INCLUDE_ACCEPT_FROM_CALLED_PARTY";
				break;
			case MON_INCLUDE_CALL_CONNECT:
				estado = "INCLUDE_CALL_CONNECT";
				break;
			default:
				estado = "ERROR!";
				break;
		}
		warning_level = 3;
		if ( this->lineas[index_linea]->mon_item_interviniendo == mon_item )
			warning_level = 6;

		this->logger->Write( System::String::Format (	"MON|[{0}-{1}]||..{2}..||ESCUCHO?{3}||Stop?{4}||{5}||{6}||id:{7}||orig:{8}||dst:{9}||"
														"ip1:{10}||port1:{11}||ip2:{12}||port2:{13}||allowtx:{14}||tx?{15}||exist_tx:{16}->{17}",
			index_linea, this->lineas[index_linea]->ssi_linea, mon_item->ssi_mon, mon_item->interviniendo, mon_item->stop_intervention, tipo, estado, llamada->id, llamada->ssi_origen, llamada->ssi_destino,
			llamada->acapi_ip, llamada->acapi_rtp_port, llamada->acapi_ip_mon, llamada->acapi_rtp_port_mon,
			llamada->tx_req_permission, llamada->tx_granted, llamada->exist_tx, llamada->tx_ssi ), warning_level );
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( System::String::Format ( "problema en print_estado_mon_ssi, para linea {0}", index_linea ), 4 );
	}
}

/*
Se comprueba si hay un msg_number recibido anteriormente igual al recibido, se devuelve true si es cierto.

Si es la primera vez que se llama a la función se inicializan las variables que intervienen
*/
bool Consumidor::check_duplicated_sds( System::Int32 msg_number )
{
	try
	{
		if ( this->last_sds_msg_numbers == nullptr )	// primera vez que se llama ( que se recibe un sds correcto )
		{
			this->last_sds_msg_numbers = gcnew array<System::Int32>( NUMBER_SDS_MSG_NUMBER_TRACK );
			this->last_sds_number_pointer = 0;
			for ( int i=0; i < this->last_sds_msg_numbers->Length; i++ )
				this->last_sds_msg_numbers[i] = -1;

			this->logger->Write( "INCIALIZADA ESTRUCTURA de TRACKING SDS duplicados", 1 );

			return false;
		}
		else
		{
			for ( int i=0; i<this->last_sds_msg_numbers->Length; i++ )
			{
				if ( this->last_sds_msg_numbers[i] == msg_number )
					return true;
			}
			return false;
		}

	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en check_duplicated_sds", 4 );
		return false;
	}
}

/*
El array "last_sds_msg_numbers" contendrá los últimos msg_number recibidos. Se actualizan de manera cíclica

"last_sds_number_pointer" indica la posición del último msg_number que se inserto
*/
void Consumidor::insert_sds_msg_number( System::Int32 msg_number )
{
	try
	{
		// Primero se actualiza "last_sds_number_pointer"
		this->last_sds_number_pointer ++;
		if ( this->last_sds_number_pointer == this->last_sds_msg_numbers->Length )
			this->last_sds_number_pointer = 0;

		this->last_sds_msg_numbers[this->last_sds_number_pointer] = msg_number;

		this->logger->Write( System::String::Format( "SDS TRACK duplicados insertado msg_number {0} en la posicion {1}", msg_number, this->last_sds_number_pointer ), 1 );

	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en insert_sds_msg_number", 4 );
	}
}

/*
cc_setup_ack	app_handle	call_part	callingPart	callID	enc_mode	rtpPort		rtpIP
char			int			int			int			int		char		int			string
2				4			8			8			4		1			4			variable?

Respuesta de acapi a una orden de iniciar llamada por parte del 105
*/
void Consumidor::cc_setup_ack_callback( System::String^ _contenido )
{
	try
	{
		System::Int32	app_handle, called_party, calling_party, call_id, enc_mode, rtp_port;

		array<System::Byte>^ ipaddr_b = gcnew array<System::Byte>(4);

		app_handle		= System::Convert::ToInt32( _contenido->Substring( 2, 4 ), 16 );
		called_party	= System::Convert::ToInt32( _contenido->Substring( 6, 8 ), 16 );
		calling_party	= System::Convert::ToInt32( _contenido->Substring( 14, 8 ), 16 );
		call_id			= System::Convert::ToInt32( _contenido->Substring( 22, 4 ), 16 );
		enc_mode		= System::Convert::ToInt32( _contenido->Substring( 26, 1 ), 16 );
		rtp_port		= System::Convert::ToInt32( _contenido->Substring( 27, 4 ), 16 );

		//observados bytes vacios
		ipaddr_b[0] = _contenido[ _contenido->Length - 4 ];
		ipaddr_b[1] = _contenido[ _contenido->Length - 3 ];
		ipaddr_b[2] = _contenido[ _contenido->Length - 2 ];
		ipaddr_b[3] = _contenido[ _contenido->Length - 1 ];

		IPAddress^ ipaddr = gcnew IPAddress( ipaddr_b );

		int index_linea = find_index_by_handle( app_handle, CC_APP );
		if ( this->lineas[index_linea]->comunicacion->id == -1 )
		{
			this->lineas[index_linea]->comunicacion->estado = CC_ACKED;
			this->lineas[index_linea]->comunicacion->id = call_id;
			this->lineas[index_linea]->comunicacion->ssi_destino = called_party;
			this->lineas[index_linea]->comunicacion->ssi_origen = calling_party;
			this->lineas[index_linea]->comunicacion->acapi_ip = ipaddr;
			this->lineas[index_linea]->comunicacion->acapi_rtp_port = rtp_port;
			this->lineas[index_linea]->comunicacion->tx_granted = false;

			this->logger->Write( System::String::Format( "cc_setup_ack ssi: {0}, dst: {1} callid: {2}, ipdst: {3}, portdst: {4}",
					this->lineas[index_linea]->ssi_linea, called_party, call_id, ipaddr, rtp_port ), 0 );
			this->print_estado_linea( index_linea );
		}
		else
		{
			this->logger->Write( System::String::Format( "cc_setup_ack recibido en una linea que ya tenia una llamada!!!!!!! RECIBIDO: ssi: {0}, dst: {1} callid: {2}, ipdst: {3}, portdst: {4}",
					this->lineas[index_linea]->ssi_linea, called_party, call_id, ipaddr, rtp_port ), 2 );
			this->print_estado_linea( index_linea );
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en cc_setup_ack_callback", 4 );
	}
}

/*
cc_information	app_handle	call_part	callID	call_state	duplex	commtype
char			int			int			int		char		char	char
2				4			8			4		1			1		1
callState
	0 Progressing: start of call setup phase
	1 CallQueued: call is in queue
	2 SubscriberPaged (*not implemented)
	3 Ringing: call signalled at remote station
	4 CallContinue: interrupted call will be continued
	5 HangTimeExpired: hangover time has elapsed
Duplex 1, Simplex 0
CommType
	0 = pointToPoint -- normal call
	1 = pointToMultipoint -- group call
	2 = pointToMultipointAck -- acknowledged group call, not implemented
	3 = broadcast
*/
void Consumidor::cc_information_callback( System::String^ _contenido )
{
	try
	{
		System::Int32	app_handle, called_party, call_id, call_state, comm_type;
		bool duplex;

		app_handle		= System::Convert::ToInt32( _contenido->Substring( 2, 4 ), 16 );
		called_party	= System::Convert::ToInt32( _contenido->Substring( 6, 8 ), 16 );
		call_id			= System::Convert::ToInt32( _contenido->Substring( 14, 4 ), 16 );
		call_state		= System::Convert::ToInt32( _contenido->Substring( 18, 1 ), 16 );
		duplex			= ( System::Convert::ToInt32( _contenido->Substring( 19, 1 ), 16 ) == 0 ) ? true : false;
		comm_type		= System::Convert::ToInt32( _contenido->Substring( 20, 1 ), 16 );

		int index_linea = find_index_by_handle( app_handle, CC_APP );

		// debería tratarse de la misma llamada que tiene activa la linea, un ack recibido anteriormente. ¿¿caso de varias llamadas por linea??
		if ( this->lineas[index_linea]->comunicacion->id == call_id )
		{
			this->lineas[index_linea]->comunicacion->estado = call_state;

			/*		En llamadas de grupo no se comunica fiablemente el tipo de llamada en cc_information.
					Se rellena manualmente cuando se emite la llamada ( call_ssi() )
			if ( duplex )
				this->lineas[index_linea]->comunicacion->tipo = CC_TIPO_DUPLEX;		// una llamada duplex solo puede ser punto a punto
			else if ( comm_type >= 0 && comm_type <= 3 )
				this->lineas[index_linea]->comunicacion->tipo = comm_type;
			else
				throw gcnew System::Exception();
			*/
			
			this->logger->Write( System::String::Format( "cc_information ssi: {0}, dst: {1} callid: {2}, duplex?:{3}  estado: {4} ",
				this->lineas[index_linea]->ssi_linea, called_party, call_id, duplex, this->lineas[index_linea]->comunicacion->tipo ), 0 );
			this->print_estado_linea( index_linea );
		}
		else
		{
			this->logger->Write( System::String::Format( "cc_sinformation,callId incoeherente. Esperado:{0}, Recibido ssi: {1}, dst: {2} callid: {3}, estado: {4}",
				this->lineas[index_linea]->comunicacion->id, this->lineas[index_linea]->ssi_linea, called_party, call_id, this->lineas[index_linea]->comunicacion->tipo ), 2 );
			this->print_estado_linea( index_linea );
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en cc_information_callback", 4 );
	}
}

/*
cc_connect	app_handle	call_part	callID	hookSign	duplex	commtype	txGrant	lineState	encryption
char		int			int			int		char		char	char		char	char		char
2			4			8			4		1			1		1			1		1			1
Hook Signaling: 0=No hook signalling 1=Hook on/Hook off signalling
Duplex 1, Simplex 0
CommType
	0 = pointToPoint -- normal call
	1 = pointToMultipoint -- group call
	2 = pointToMultipointAck -- acknowledged group call, not implemented
	3 = broadcast
TxGrant
	0 = txGranted
	1 = txNotGranted
	2 = txRequestQueued
	3 = txGrantedToAnotherUser
Line State
	0 = lineConnected
	1 = lineNotConnected
	2 = parameterError
	3 = unknownLineResource
	4 = inactiveLineResource
	5 = lineResourceUsedInOtherCall
Encryption-Mode:
	0: Clear Mode
	1: TETRA End-to-End encryption
*/
void Consumidor::cc_connect_callback( System::String^ _contenido )
{
	try
	{
		System::Int32	app_handle, called_party, call_id, comm_type, tx_grant, line_state;
		bool hook_signalling, duplex, line_encryption;

		app_handle		= System::Convert::ToInt32( _contenido->Substring( 2, 4 ), 16 );
		called_party	= System::Convert::ToInt32( _contenido->Substring( 6, 8 ), 16 );
		call_id			= System::Convert::ToInt32( _contenido->Substring( 14, 4 ), 16 );
		hook_signalling	= ( System::Convert::ToInt32( _contenido->Substring( 18, 1 ), 16 ) == 1 ) ? true : false;
		duplex			= ( System::Convert::ToInt32( _contenido->Substring( 19, 1 ), 16 ) == 0 ) ? true : false;
		comm_type		= System::Convert::ToInt32( _contenido->Substring( 20, 1 ), 16 );
		tx_grant		= System::Convert::ToInt32( _contenido->Substring( 21, 1 ), 16 );
		line_state		= System::Convert::ToInt32( _contenido->Substring( 22, 1 ), 16 );
		line_encryption	= ( System::Convert::ToInt32( _contenido->Substring( 23, 1 ), 16 ) == 1 ) ? true : false;

		int index_linea = find_index_by_handle( app_handle, CC_APP );

		// debería tratarse de la misma llamada que tiene activa la linea, un ack recibido anteriormente. ¿¿caso de varias llamadas por linea??
		if ( this->lineas[index_linea]->comunicacion->id == call_id )
		{
			if ( line_state == 0 )
			{
				//this->lineas[index_linea]->comunicacion->tipo = comm_type;
				this->lineas[index_linea]->comunicacion->estado = CC_ESTABLISHED;
				this->lineas[index_linea]->comunicacion->tx_granted = ( tx_grant == 0 );		// 0 = txGranted
				if ( ( tx_grant == 0 ) || ( tx_grant == 3 ) )									// 1 = txNotGranted, 2 = txRequestQueued
					this->lineas[index_linea]->comunicacion->exist_tx = true;

				this->logger->Write( System::String::Format( "cc_connect establecida ssi: {0}, dst_ssi: {1} callid: {2}, commtype: {3}  txgranted?: {4}",
						this->lineas[index_linea]->ssi_linea, called_party, call_id, comm_type, this->lineas[index_linea]->comunicacion->tx_granted ), 0 );

				this->manager->avisaAcCore(System::String::Format( "{0}{1}:{2}:{3}:{4}:{5}:{6}:{7}{8}",
					TRAMA_INI, CC_LLAMADA_ESTABLECIDA, index_linea, this->lineas[index_linea]->comunicacion->tipo,
					this->lineas[index_linea]->comunicacion->ssi_destino, this->lineas[index_linea]->comunicacion->priority,
					this->lineas[index_linea]->comunicacion->acapi_ip, this->lineas[index_linea]->comunicacion->acapi_rtp_port, TRAMA_END ) );

				this->proceso_escucha_rtp( true, index_linea );


				/*
					Las llamadas emitidas por ssi applicación ( cc_connect_callback => CC_ESTABLISHED ) reciben la información inicial de PTT
					en el propio callback ( CC_CONNEC_IND ) ya sean individual o de grupo. Sin embargo las llamadas que son recibidas por los
					ssi aplicación ( cc_setup_callback => CC_ESTABLISHED ) reciben la información del PTT en el propio callback (llamadas individuales)
					o en un tx_grant emitido justo despues (llamadas de grupo)
				*/
				if ( ( this->lineas[index_linea]->comunicacion->tipo == CC_TIPO_POINTTOPOINT || this->lineas[index_linea]->comunicacion->tipo == CC_TIPO_POINTTOMULTIPOINT )
					&& this->lineas[index_linea]->comunicacion->tx_granted )
				{
					this->lineas[index_linea]->comunicacion->tx_ssi = this->lineas[index_linea]->ssi_linea;
				}
				else if ( this->lineas[index_linea]->comunicacion->tipo == CC_TIPO_POINTTOPOINT || this->lineas[index_linea]->comunicacion->tipo == CC_TIPO_POINTTOMULTIPOINT )
				{
					this->lineas[index_linea]->comunicacion->tx_ssi = this->lineas[index_linea]->comunicacion->ssi_destino;
				}
				// duplex no informo de PTT
				if ( this->lineas[index_linea]->comunicacion->tipo == CC_TIPO_POINTTOPOINT || this->lineas[index_linea]->comunicacion->tipo == CC_TIPO_POINTTOMULTIPOINT )
				{		//avisa info de tx "CC_PTT"
					this->manager->avisaAcCore(System::String::Format( "{0}{1}:{2}:{3}:{4}:{5}:{6}{7}",
									TRAMA_INI, CC_PTT, index_linea, this->lineas[index_linea]->comunicacion->tx_req_permission ? 1:0,
									this->lineas[index_linea]->comunicacion->tx_granted ? 1:0, this->lineas[index_linea]->comunicacion->exist_tx ? 1:0,
									this->lineas[index_linea]->comunicacion->tx_ssi, TRAMA_END ) );
				}

				this->print_estado_linea( index_linea );
			}
			else
			{
				this->logger->Write( System::String::Format( "cc_connect no se pudo establecer ssi: {0}, callid: {1}, callback line_state: {2}",
						this->lineas[index_linea]->ssi_linea, call_id, line_state ), 2 );
				this->print_estado_linea( index_linea );
			}
		}
		else
		{
			this->logger->Write( System::String::Format( "cc_connect,callId incoeherente. Esperado:{0}, Recibido ssi: {1}, dst: {2} callid: {3}, estado: {4}",
				this->lineas[index_linea]->comunicacion->id, this->lineas[index_linea]->ssi_linea, called_party, call_id, this->lineas[index_linea]->comunicacion->tipo ), 2 );
			this->print_estado_linea( index_linea );
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en cc_connect_callback", 4 );
	}
}

/*		ambas cc_disconnect y cc_disconnect_ack tienen la misma estructura

cc_disconnect/_ack	app_handle	call_part	callID	disconCause
char				int			int			int		short
2					4			8			4		2

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
25 icLeavingCallNoCallEnd
23 unknownTetraIdentity
24 ssSpecificDisconnection
25 unknownExternalSubscriberIdentity
26 callRestorationOfOtherUserFailed
27 wrongCommunicationType
*/
void Consumidor::cc_disconnect_callback( System::String^ _contenido )
{
	try
	{
		System::Int32	app_handle, called_party, call_id, cause;


		app_handle		= System::Convert::ToInt32( _contenido->Substring( 2, 4 ), 16 );
		called_party	= System::Convert::ToInt32( _contenido->Substring( 6, 8 ), 16 );
		call_id			= System::Convert::ToInt32( _contenido->Substring( 14, 4 ), 16 );
		cause			= System::Convert::ToInt32( _contenido->Substring( 18, 2 ), 16 );

		int index_linea = find_index_by_handle( app_handle, CC_APP );

		if ( this->lineas[index_linea]->comunicacion->id == call_id )
		{
			this->lineas[index_linea]->comunicacion = gcnew Comunicacion_tetra( -1, -1, -1 );
			this->logger->Write( System::String::Format( "cc_disconect ssi: {0}, ssi_dst:{1} callid: {2}, cause: {3}", this->lineas[index_linea]->ssi_linea, called_party, call_id, cause ), 0 );
			this->lineas[index_linea]->comunicacion->tx_ssi = -1;
			this->print_estado_linea( index_linea );

			this->manager->avisaAcCore(System::String::Format( "{0}{1}:{2}{3}",
				TRAMA_INI, CC_LLAMADA_ACABADA, index_linea, TRAMA_END ) );

			this->proceso_escucha_rtp( false,  index_linea );
		}
		else
		{
			this->logger->Write( System::String::Format( "cc_disconect UNA LLAMADA NO REGISTRADA EN LA LINEA ssi: {0}, con callid: {1}, Recibido: callID:{2}, ssi_dst:{3}, cause: {4}",
				this->lineas[index_linea]->ssi_linea, this->lineas[index_linea]->comunicacion->id, call_id, called_party, cause ), 2 );
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en cc_disconnect_callback", 4 );
	}
}

/*
cc_setup	app_handle	target_part	callID	hookSign	duplex	commtype	txGrant	call_prior	src_address	encryption
char		int			int			int		char		char	char		char	short		int			char
2			4			8			4		1			1		1			1		2			8			1
Hook Signaling: 0=No hook signalling 1=Hook on/Hook off signalling
Duplex 1, Simplex 0
CommType
	0 = pointToPoint -- normal call
	1 = pointToMultipoint -- group call
	2 = pointToMultipointAck -- acknowledged group call, not implemented
	3 = broadcast
TxGrant
	0 = txGranted
	1 = txNotGranted
	2 = txRequestQueued
	3 = txGrantedToAnotherUser
callPriority :
	0 = priorityNotDefined
	1 = priority1 Lowest Priority
	2 = priority2
	3 = priority3
	4 = priority4
	5 = priority5
	6 = priority6
	7 = priority7
	8 = priority8
	9 = priority9
	10= priority10
	11= priority11
	12= preEmptivePrio1
	13= preEmptivePrio2
	14= preEmptivePrio3
	15= preEmptivePrio4 Emergency
Encryption-Mode:
	0: Clear Mode
	1: TETRA End-to-End encryption

	Callback emitido por la acapi.DLL cuando se recibe una llamada entrante.
	Lógica necesaria para colgar automáticamente. Se ha de evitar más de un flujo de audio por linea:
		Llamada recibida cuando existia una llamada existente
		Llamada recibida para un grupo distinto al asociado o para un ssi distinto al particular de la linea
		Llamada recibida cuando se estaba interviniendo en una llamada monitorizada
*/
void Consumidor::cc_setup_callback( System::String^ _contenido )
{
	try
	{
		System::Int32	app_handle, target_party, call_id, comm_type, tx_grant, call_priority, src_party;
		bool hook_signalling, duplex, line_encryption;

		bool permiso_aceptar = false;

		app_handle		= System::Convert::ToInt32( _contenido->Substring( 2, 4 ), 16 );
		target_party	= System::Convert::ToInt32( _contenido->Substring( 6, 8 ), 16 );
		call_id			= System::Convert::ToInt32( _contenido->Substring( 14, 4 ), 16 );
		hook_signalling	= ( System::Convert::ToInt32( _contenido->Substring( 18, 1 ), 16 ) == CC_HOOK_SIGNALLING_TRUE ) ? true : false;
		duplex			= ( System::Convert::ToInt32( _contenido->Substring( 19, 1 ), 16 ) == 0 ) ? true : false;
		comm_type		= System::Convert::ToInt32( _contenido->Substring( 20, 1 ), 16 );
		tx_grant		= System::Convert::ToInt32( _contenido->Substring( 21, 1 ), 16 );
		call_priority	= System::Convert::ToInt32( _contenido->Substring( 22, 2 ), 16 );
		src_party		= System::Convert::ToInt32( _contenido->Substring( 24, 8 ), 16 );
		line_encryption	= ( System::Convert::ToInt32( _contenido->Substring( 32, 1 ), 16 ) == 1 ) ? true : false;

		int index_linea = find_index_by_handle( app_handle, CC_APP );

		permiso_aceptar =	( (
							target_party == this->lineas[index_linea]->ssi_grupo_asociado ||	// llamada dirigida al grupo asociado
							target_party == this->lineas[index_linea]->ssi_linea				// llamada dirigida al ssi particular de la linea
								)	&&
							this->lineas[index_linea]->mon_item_interviniendo == nullptr		// no existe ninguna intervencion de monitorizacion
							);

		if ( this->lineas[index_linea]->mon_item_interviniendo != nullptr )
			this->logger->Write( System::String::Format( "LA LINEA {0} estaba interviniendo al mon_item {1}",
					index_linea, this->lineas[index_linea]->mon_item_interviniendo->ssi_mon ), 6 );

		// debería tratarse de la misma llamada que tiene activa la linea, un ack recibido anteriormente. ¿¿caso de varias llamadas por linea??
		if ( this->lineas[index_linea]->comunicacion->id == -1 && permiso_aceptar )
		{
			this->lineas[index_linea]->comunicacion->estado			= CC_RECEIVED;
			this->lineas[index_linea]->comunicacion->id				= call_id;
			this->lineas[index_linea]->comunicacion->ssi_origen		= src_party;
			this->lineas[index_linea]->comunicacion->ssi_destino	= target_party;
			this->lineas[index_linea]->comunicacion->priority		= call_priority;
			this->lineas[index_linea]->comunicacion->tx_granted		= ( tx_grant == 0 );
			this->lineas[index_linea]->comunicacion->exist_tx		= false;		// aun no se ha aceptado

			if ( duplex )
				this->lineas[index_linea]->comunicacion->tipo = CC_TIPO_DUPLEX;		// una llamada duplex solo puede ser punto a punto
			else if ( comm_type >= 0 && comm_type <= 3 )
				this->lineas[index_linea]->comunicacion->tipo = comm_type;
			else
				throw gcnew System::Exception();

			this->logger->Write( System::String::Format( "cc_setup desde target_ssi:{0}, src_ssi:{1}, callid:{2}, tipo:{3} txgranted?:{4}",
					target_party, src_party, call_id, this->lineas[index_linea]->comunicacion->tipo, tx_grant ), 0 );
			this->print_estado_linea( index_linea );
			
			this->manager->avisaAcCore(System::String::Format( "{0}{1}:{2}:{3}:{4}:{5}{6}",
					TRAMA_INI, CC_LLAMADA_ENTRANTE, index_linea, this->lineas[index_linea]->comunicacion->tipo,
					this->lineas[index_linea]->comunicacion->ssi_origen, call_priority, TRAMA_END ) );
			
			// una llamada se cogerá automáticamente si es destinada al grupo asociado o ssi_linea, si no tiene hook signalling o si esta pinchada
			if ( ( this->lineas[index_linea]->pinchada || ( ! hook_signalling ) ) && permiso_aceptar )
			{
				this->logger->Write( System::String::Format( "Se acepta la llamada al ssi:{0} con id:{1}", target_party, call_id ), 1 );
				this->acepta_llamada( index_linea );
			}
		}
		else
		{
			if ( ! permiso_aceptar )
			{
				this->logger->Write( System::String::Format( "cc_setup SIN PERMISO!! destino target_ssi:{0}, ssi _linea:{1} ssi_grupo_asociado:{2}",
					target_party, this->lineas[index_linea]->ssi_linea, this->lineas[index_linea]->ssi_grupo_asociado ), 2 );
			}
			else
			{
				this->logger->Write( System::String::Format( "cc_setup, cuando existía una llamada en la linea. Anteriorssi:{0}, AnteriorId{1} Recibido ssi: {2}, dst: {3} callid: {4}",
					this->lineas[index_linea]->ssi_linea, this->lineas[index_linea]->comunicacion->id, src_party, target_party, call_id ), 2 );
			}

			this->logger->Write( System::String::Format( "Se intenta desconectar la llamada al ssi:{0} con id:{1}", target_party, call_id ), 1 );
			if ( ! this->tetra->cuelga_llamada( app_handle, call_id, 2 ) )			// disconnectCause 2->calledPartyBusy
				this->logger->Write( System::String::Format( "TetraIterface, no se pudo emitir la orden de desconexion!!!!apphandle:{0} con callid:{1}", app_handle, call_id ), 4 );
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en cc_setup_callback", 4 );
	}
}

/*
cc_accepted		app_handle	called_part	callID	tx_grant	line_state	rtpPort		rtpPayloadType	rtpIP
char			int			int			int		char		char		int			int				string
2				4			8			4		1			1			4			4				variable?

Respuesta de acapi a una orden de aceptar llamada entrante al 105
Line State : Response to lineInformation, from server to appl.
	0 = lineConnected
	1 = lineNotConnected
	2 = parameterError
	3 = unknownLineResource
	4 = inactiveLineResource
	5 = lineResourceUsedInOtherCall
*/
void Consumidor::cc_accepted_callback( System::String^ _contenido )
{
	try
	{
		System::Int32	app_handle, called_party, call_id, tx_grant, line_state, rtp_port, rtp_payload;

		array<System::Byte>^ ipaddr_b = gcnew array<System::Byte>(4);

		app_handle		= System::Convert::ToInt32( _contenido->Substring(  2, 4 ), 16 );
		called_party	= System::Convert::ToInt32( _contenido->Substring(  6, 8 ), 16 );
		call_id			= System::Convert::ToInt32( _contenido->Substring( 14, 4 ), 16 );
		tx_grant		= System::Convert::ToInt32( _contenido->Substring( 18, 1 ), 16 );
		line_state		= System::Convert::ToInt32( _contenido->Substring( 19, 1 ), 16 );

		int index_linea = find_index_by_handle( app_handle, CC_APP );

		if ( this->lineas[index_linea]->comunicacion->id == call_id && this->lineas[index_linea]->comunicacion->estado == CC_ACCEPTING )
		{
			if ( line_state == 0 )		// 0 = lineConnected
			{
				rtp_port		= System::Convert::ToInt32( _contenido->Substring( 20, 4 ), 16 );
				rtp_payload		= System::Convert::ToInt32( _contenido->Substring( 24, 4 ), 16 );
		
				ipaddr_b[0] = _contenido[ _contenido->Length - 4 ];
				ipaddr_b[1] = _contenido[ _contenido->Length - 3 ];
				ipaddr_b[2] = _contenido[ _contenido->Length - 2 ];
				ipaddr_b[3] = _contenido[ _contenido->Length - 1 ];

				IPAddress^ ipaddr = gcnew IPAddress( ipaddr_b );

				this->lineas[index_linea]->comunicacion->estado = CC_ESTABLISHED;
				this->lineas[index_linea]->comunicacion->acapi_ip = ipaddr;
				this->lineas[index_linea]->comunicacion->acapi_rtp_port = rtp_port;
				this->lineas[index_linea]->comunicacion->tx_granted = ( tx_grant == 0 );
				if ( ( tx_grant == 0 ) || ( tx_grant == 3 ) )
					this->lineas[index_linea]->comunicacion->exist_tx = true;

				this->logger->Write( System::String::Format( "cc_accepted ssi: {0}, dst: {1} callid: {2}, ipdst: {3}, portdst: {4}",
						this->lineas[index_linea]->ssi_linea, called_party, call_id, ipaddr, rtp_port ), 0 );

				// modificar ssi que se comunica, ORIGEN O DESTINO segun si llamada de grupo o individual !!!!!!!!!!!!!!
				this->manager->avisaAcCore(System::String::Format( "{0}{1}:{2}:{3}:{4}:{5}:{6}:{7}{8}",
					TRAMA_INI, CC_LLAMADA_ESTABLECIDA, index_linea, this->lineas[index_linea]->comunicacion->tipo, this->lineas[index_linea]->comunicacion->ssi_destino,
					this->lineas[index_linea]->comunicacion->priority, ipaddr, rtp_port, TRAMA_END ) );

				this->proceso_escucha_rtp( true, index_linea );

				/*
					Las llamadas emitidas por ssi applicación ( cc_connect_callback => CC_ESTABLISHED ) reciben la información inicial de PTT
					en el propio callback ( CC_CONNEC_IND ) ya sean individual o de grupo. Sin embargo las llamadas que son recibidas por los
					ssi aplicación ( cc_setup_callback => CC_ESTABLISHED ) reciben la información del PTT en el propio callback (llamadas individuales)
					o en un tx_grant emitido justo despues (llamadas de grupo)
				*/
				if ( this->lineas[index_linea]->comunicacion->tipo == CC_TIPO_POINTTOPOINT && this->lineas[index_linea]->comunicacion->tx_granted )
				{
					this->lineas[index_linea]->comunicacion->tx_ssi = this->lineas[index_linea]->ssi_linea;
				}
				else if ( this->lineas[index_linea]->comunicacion->tipo == CC_TIPO_POINTTOPOINT )
				{
					this->lineas[index_linea]->comunicacion->tx_ssi = this->lineas[index_linea]->comunicacion->ssi_origen;
				}
				// EN el caso de CC_TIPO_POINTTOMULTIPOINT habrá un txgrant sucesivo que informara del PTT, así evito duplicados
				if ( this->lineas[index_linea]->comunicacion->tipo == CC_TIPO_POINTTOPOINT )
				{		//avisa info de tx "CC_PTT"
					this->manager->avisaAcCore(System::String::Format( "{0}{1}:{2}:{3}:{4}:{5}:{6}{7}",
									TRAMA_INI, CC_PTT, index_linea, this->lineas[index_linea]->comunicacion->tx_req_permission ? 1:0,
									this->lineas[index_linea]->comunicacion->tx_granted ? 1:0, this->lineas[index_linea]->comunicacion->exist_tx ? 1:0,
									this->lineas[index_linea]->comunicacion->tx_ssi, TRAMA_END ) );
				}

				this->print_estado_linea( index_linea );
			}
			else
			{
				this->logger->Write( System::String::Format( "AAAAAAAAAAAAAAAAAAAAA cc_accepted con line_state distinto a lineConnected, recibido:{0}!!!", line_state ), 2 );
				this->print_estado_linea( index_linea );
				this->logger->Write( System::String::Format( "AAAAAAAAAAAAAAAAAAAAAAA Se intenta desconectar la llamada al ssi:{0} con id:{1}", called_party, call_id ), 1 );
				if ( ! this->tetra->cuelga_llamada( app_handle, call_id, 2 ) )			// disconnectCause 1->calledPartyBusy
					this->logger->Write( System::String::Format( "TetraIterface, no se pudo emitir la orden de desconexion!!!!apphandle:{0} con callid:{1}", app_handle, call_id ), 4 );

			}
		}
		else
		{
			this->logger->Write( System::String::Format( "cc_accepted incoherente!! callId rcv:{0} expect:{1} estado_previo:{2} distinto a CC_ACEPTING=10",
				call_id, this->lineas[index_linea]->comunicacion->id, this->lineas[index_linea]->comunicacion->estado ), 2 );
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en cc_accepted_callback", 4 );
	}
}

/*
cc_ceased		app_handle	called_part	callID	txReqPermission
char			int			int			int		char
2				4			8			4		1

txReqPermission :0 true, 1 false;		// OBSERVADO AL REVES!!!! 0 false, 1 True!!!!!!!!!!!!!
Significa que nadie está hablando!
*/
void Consumidor::cc_ceased_callback( System::String^ _contenido )
{
	try
	{
		System::Int32	app_handle, called_party, call_id;
		bool tx_req_permission;

		array<System::Byte>^ ipaddr_b = gcnew array<System::Byte>(4);

		app_handle			= System::Convert::ToInt32( _contenido->Substring( 2, 4 ), 16 );
		called_party		= System::Convert::ToInt32( _contenido->Substring( 6, 8 ), 16 );
		call_id				= System::Convert::ToInt32( _contenido->Substring( 14, 4 ), 16 );
		tx_req_permission	= ( System::Convert::ToInt32( _contenido->Substring( 18, 1 ), 16 ) == 1 ) ? true : false;

		int index_linea = find_index_by_handle( app_handle, CC_APP );

		if ( this->lineas[index_linea]->comunicacion->id == call_id )
		{
			/*
				en llamadas de grupo existirán cc_ceased duplicados cuando sea el ssi applicacion el que despulse el PTT
				un ceased tendrá 'called_party = ssi_linea' y otro 'called_party = ssi_grupo'
				
				ESTA DUPLICIDAD OCURRE SI LA LLAMADA DE GRUPO NO ES INICIADA POR NINGUNA LINEA DEL 105. En la situacion donde alguna linea del 105
				sea la iniciadora de la llamada de grupo solo llegan cc_ceased con el dst_ssi particular ( == linea iniciadora )

				=> se necesita logica para no avisar al AcCore dos veces, cc_ceased solo informa que no hay nadie transmitiendo
				!!!!( fijarse en comunicacion->exist_tx )!!!!! => ignoro si en la llamada ya se ha recibido un cc_ceased antes!
			*/
			if ( this->lineas[index_linea]->comunicacion->estado == CC_ESTABLISHED && this->lineas[index_linea]->comunicacion->exist_tx == true )
			{
				this->lineas[index_linea]->comunicacion->tx_granted = false;
				this->lineas[index_linea]->comunicacion->exist_tx = false;
				this->lineas[index_linea]->comunicacion->tx_req_permission = tx_req_permission;
				this->lineas[index_linea]->comunicacion->tx_ssi = -1;

				this->logger->Write( System::String::Format( "cc_ceased ssi: {0}, dst: {1} callid: {2}, txReqPermission: {3}",
						this->lineas[index_linea]->ssi_linea, called_party, call_id, tx_req_permission ), 0 );
				this->print_estado_linea( index_linea );
				this->manager->avisaAcCore(System::String::Format( "{0}{1}:{2}:{3}:{4}:{5}:{6}{7}",
						TRAMA_INI, CC_PTT, index_linea, this->lineas[index_linea]->comunicacion->tx_req_permission ? 1:0,
						this->lineas[index_linea]->comunicacion->tx_granted ? 1:0, this->lineas[index_linea]->comunicacion->exist_tx ? 1:0,
						this->lineas[index_linea]->comunicacion->tx_ssi, TRAMA_END ) );
			}
			else
			{
				this->logger->Write( System::String::Format( "cc_ceased IGNORADO ssi: {0}, dst: {1} callid: {2}, txReqPermission: {3}",
						this->lineas[index_linea]->ssi_linea, called_party, call_id, tx_req_permission ), 2 );
			}
		}
		else
		{
			this->logger->Write( "cc_ceased incoherente!! con callId distinto", 2 );
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en cc_accepted_callback", 4 );
	}
}

/*
cc_grant		app_handle	called_part	tx_party	callID	txGrant	txReqPermission
char			int			int			int			int		char	char
2				4			8			8			4		1		1

txReqPermission: 0 true, 1 false;		// OBSERVADO AL REVES!!!! 0 false, 1 True!!!!!!!!!!!!!
Significa que nadie está hablando!
*/
void Consumidor::cc_grant_callback( System::String^ _contenido )
{
	try
	{
		System::Int32	app_handle, called_party, tx_party, call_id, tx_grant;
		bool tx_req_permission;

		app_handle			= System::Convert::ToInt32( _contenido->Substring( 2, 4 ), 16 );
		called_party		= System::Convert::ToInt32( _contenido->Substring( 6, 8 ), 16 );
		tx_party			= System::Convert::ToInt32( _contenido->Substring( 14, 8 ), 16 );
		call_id				= System::Convert::ToInt32( _contenido->Substring( 22, 4 ), 16 );
		tx_grant			= ( System::Convert::ToInt32( _contenido->Substring( 26, 1 ), 16 ) );
		tx_req_permission	= ( System::Convert::ToInt32( _contenido->Substring( 27, 1 ), 16 ) == 1 ) ? true : false;

		int index_linea = find_index_by_handle( app_handle, CC_APP );

		if ( this->lineas[index_linea]->comunicacion->id == call_id )
		{
			// en una llamada de grupo, para un txdemand hecho por la applicacion se recibirán dos txgrant, uno dirigido al grupo y otro al propio ssi particular
			// habrá que ignorar aquellos mensajes destinados al grupo (todos los participantes de la llamada), donde se comunica que transmite el ssi de linea
			// ESTA DUPLICIDAD OCURRE SI LA LLAMADA DE GRUPO NO ES INICIADA POR NINGUNA LINEA DEL 105
			if (
				( this->lineas[index_linea]->comunicacion->estado != CC_ESTABLISHED )
				||
				(
				( this->lineas[index_linea]->comunicacion->tipo == CC_TIPO_POINTTOMULTIPOINT )
				&& ( called_party != this->lineas[index_linea]->ssi_linea )
				&& ( tx_party == this->lineas[index_linea]->ssi_linea )
				)
				)
			{
				this->logger->Write( System::String::Format( "cc_grant IGNORADO ssi: {0}, dst_ssi:{1}, tx_ssi: {2}, callid: {3}, tx_grant: {4} txReqPermission: {5}",
						this->lineas[index_linea]->ssi_linea, called_party, tx_party, call_id, tx_grant, tx_req_permission ), 2 );
			}
			else
			{
				this->lineas[index_linea]->comunicacion->tx_granted = ( tx_grant == 0 );
				if ( ( tx_grant == 0 ) || ( tx_grant == 3 ) )
				{
					this->lineas[index_linea]->comunicacion->exist_tx = true;
					this->lineas[index_linea]->comunicacion->tx_ssi = tx_party;
				}
				this->lineas[index_linea]->comunicacion->tx_req_permission = tx_req_permission;

				this->logger->Write( System::String::Format( "cc_grant ssi: {0}, dst_ssi:{1}, tx_ssi: {2}, callid: {3}, tx_grant: {4} txReqPermission: {5}",
						this->lineas[index_linea]->ssi_linea, called_party, tx_party, call_id, tx_grant, tx_req_permission ), 0 );
				this->print_estado_linea( index_linea );

				this->manager->avisaAcCore(System::String::Format( "{0}{1}:{2}:{3}:{4}:{5}:{6}{7}",
						TRAMA_INI, CC_PTT, index_linea, this->lineas[index_linea]->comunicacion->tx_req_permission ? 1:0,
						this->lineas[index_linea]->comunicacion->tx_granted ? 1:0, this->lineas[index_linea]->comunicacion->exist_tx ? 1:0,
						this->lineas[index_linea]->comunicacion->tx_ssi, TRAMA_END ) );
			}
		}
		else
		{
			this->logger->Write( "cc_grant incoherente!! con callId distinto", 2 );
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en cc_accepted_callback", 4 );
	}
}

/*
Callback recibido tras emitir una orden "Mon_Intercept_Req". La información de ips y puertos son los propios de la RED TETRA a donde dirigir los streams de audio
	en caso de intervenir en la llamada monitorizada interceptada. El resultado nos dirá si se ha podido conectar las lineas y por tanto el audio será transmitido
	correctamente

mon_intercept_ack_rtp	app_handle	mon_handle	intercept_ssi1	intercept_ssi2	call_id	num_of_rtp_data	result1	rtpPort1	rtpPayload Type1	rtpIpAddress1
char					int			int			string			string			int		int				int		int			int					string(variable)
2						4			4			8				8				4		1				2		4			2					2

En el caso de num_of_rtp_data = 2

	separator	result2	rtpPort2	rtpPayload Type2	rtpIpAddress2
				int		int			int					string
	#			2		4			2					2


intercept ssi1: source_address
intercept ssi2: target_address
result* :
0 = lineConnected
1 = lineNotConnected
2 = parameterError
3 = unknownLineResource
4 = inactiveLineResource
5 = lineResourceUsedInOtherCall

*/
void Consumidor::mon_intercept_ack_callback( System::String^ _contenido )
{
	try
	{
		System::Int32	app_handle, mon_handle, source_ssi, target_ssi, call_id, num_rtp, result_1, result_2, rtp_port_1, rtp_port_2, rtp_payload_1, rtp_payload_2;

		array<System::Byte>^ ipaddr_b = gcnew array<System::Byte>(4);
		IPAddress ^ipaddr_1, ^ipaddr_2;

		app_handle			= System::Convert::ToInt32( _contenido->Substring( 2, 4 ), 16 );
		mon_handle			= System::Convert::ToInt32( _contenido->Substring( 6, 4 ), 16 );
		source_ssi			= System::Convert::ToInt32( _contenido->Substring( 10, 8 ), 16 );
		target_ssi			= System::Convert::ToInt32( _contenido->Substring( 18, 8 ), 16 );
		call_id				= System::Convert::ToInt32( _contenido->Substring( 26, 4 ), 16 );
		num_rtp				= System::Convert::ToInt32( _contenido->Substring( 30, 1 ), 16 );
		result_1			= System::Convert::ToInt32( _contenido->Substring( 31, 2 ), 16 );
		rtp_port_1			= System::Convert::ToInt32( _contenido->Substring( 33, 4 ), 16 );
		rtp_payload_1		= System::Convert::ToInt32( _contenido->Substring( 37, 2 ), 16 );

		ipaddr_b[0] = _contenido[ _contenido->Length - 4 ];
		ipaddr_b[1] = _contenido[ _contenido->Length - 3 ];
		ipaddr_b[2] = _contenido[ _contenido->Length - 2 ];
		ipaddr_b[3] = _contenido[ _contenido->Length - 1 ];

		ipaddr_1 = gcnew IPAddress( ipaddr_b );

		if ( num_rtp == 2 )
		{
			System::Int32 separator_index = _contenido->IndexOf( '#' );
			if ( separator_index < 0 )
			{
				this->logger->Write( "mon_intercept_ack_rtp incorrecto, separador no encontrado", 2 );
				throw gcnew System::Exception();
			}
			result_2			= System::Convert::ToInt32( _contenido->Substring( separator_index + 1, 2 ), 16 );
			rtp_port_2			= System::Convert::ToInt32( _contenido->Substring( separator_index + 3, 4 ), 16 );
			rtp_payload_2		= System::Convert::ToInt32( _contenido->Substring( separator_index + 7, 2 ), 16 );

			ipaddr_b[0] = _contenido[ separator_index - 4 ];
			ipaddr_b[1] = _contenido[ separator_index - 3 ];
			ipaddr_b[2] = _contenido[ separator_index - 2 ];
			ipaddr_b[3] = _contenido[ separator_index - 1 ];

			ipaddr_1 = gcnew IPAddress( ipaddr_b );					// Sobreescribo la ipaddr_1

			ipaddr_b[0] = _contenido[ _contenido->Length - 4 ];
			ipaddr_b[1] = _contenido[ _contenido->Length - 3 ];
			ipaddr_b[2] = _contenido[ _contenido->Length - 2 ];
			ipaddr_b[3] = _contenido[ _contenido->Length - 1 ];

			ipaddr_2 = gcnew IPAddress( ipaddr_b );
		}

		System::Int32 index_linea	= find_index_by_handle( app_handle, MON_APP );
		System::Int32 index_mon		= find_mon_index_by_handle( index_linea, mon_handle );

		this->logger->Write( System::String::Format( "[INTERCEPT ACK-{0}] src {1} tgt {2} id {3} rtps {4} res1 {5} port1 {6} payld1 {7} ip1 {8} res2 {9} port2 {10} payld2 {11} ip2 {12}",
						 index_linea, source_ssi, target_ssi, call_id, num_rtp, result_1, rtp_port_1, rtp_payload_1, ipaddr_1, result_2, rtp_port_2, rtp_payload_2, ipaddr_2 ), 0 );

		// las linea/s tienen que estar conectadas para oir audio, en caso contrario habrá que desconectar la escucha
		if ( ( num_rtp == 1 && result_1 == 0 ) || ( num_rtp == 2 && result_1 == 0 && result_2 == 0 ) )
		{
			if ( call_id != this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->id )
			{
				this->logger->Write( System::String::Format( "mon_intercept_ack_callback__ CALL ID Distinto del esperado!!, se escucha En linea {0} para el ssi monitorizado {1}, call_id recibido {2}, call_id esperado {3}",
						this->lineas[index_linea]->ssi_linea, this->lineas[index_linea]->monitorizaciones[index_mon]->ssi_mon, call_id, this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->id ), 2 );
				throw gcnew System::Exception();
			}

			this->lineas[index_linea]->monitorizaciones[index_mon]->interviniendo = true;
			this->lineas[index_linea]->monitorizaciones[index_mon]->stop_intervention = false;

			this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->acapi_ip			= ipaddr_1;
			this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->acapi_ip_mon		= ipaddr_2;
			this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->acapi_rtp_port	= rtp_port_1;
			this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->acapi_rtp_port_mon= rtp_port_2;

			this->logger->Write( System::String::Format( "mon_intercept_ack_callback CORRECTO, se escucha En linea {0} para el ssi monitorizado {1}",
						this->lineas[index_linea]->ssi_linea, this->lineas[index_linea]->monitorizaciones[index_mon]->ssi_mon ), 1 );

			if ( this->lineas[index_linea]->monitorizaciones[index_mon] != this->lineas[index_linea]->mon_item_interviniendo )		// Si no se trata de dos referencias al mismo objeto es un error! no puede haber  dos mon_items intervenidos
			{
				this->logger->Write( "mon_intercept_ack_callback INCORRECTo DOS MON_ITEMS estan siendo intervenidos!", 5 );
			}

			this->print_estado_mon_ssi( index_linea, index_mon );
		}
		else
		{
			this->logger->Write( System::String::Format( "mon_intercept_ack_callback incorrecto, resultado/s distinto de lineConnected En linea {0} para el ssi monitorizado {1}",
						this->lineas[index_linea]->ssi_linea, this->lineas[index_linea]->monitorizaciones[index_mon]->ssi_mon ), 2 );
			// EMITIR mon_intercept_disconnect!!
		}

	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en mon_intercept_ack_rtp", 4 );
	}
}

/*
Callback recibido tras emitir una orden "Mon_Intercept_Req". La información de ips y puertos son LOS INDICADOS POR LA APLICACION enviados previamente al servidor acapi
con la orden "Mon_Intercept_Req" a donde la red Tetra dirigirá los streams de audio de la llamada interceptada.

mon_intercept_connect	app_handle	mon_handle	call_id	num_of_rtp_data	result1	rtpPort1	rtpPayload Type1	rtpIpAddress1
char					int			int			int		int				int		int			int					string(variable)
2						4			4			4		1				2		4			2					2

En el caso de num_of_rtp_data = 2

	separator	result2	rtpPort2	rtpPayload Type2	rtpIpAddress2
				int		int			int					string
	#			2		4			2					2

result* :
0 = lineConnected
1 = lineNotConnected
2 = parameterError
3 = unknownLineResource
4 = inactiveLineResource
5 = lineResourceUsedInOtherCall

*/
void Consumidor::mon_intercept_connect_callback( System::String^ _contenido )
{
	try
	{
		System::Int32	app_handle, mon_handle, call_id, num_rtp, result_1, result_2, rtp_port_1, rtp_port_2, rtp_payload_1, rtp_payload_2;

		array<System::Byte>^ ipaddr_b = gcnew array<System::Byte>(4);
		IPAddress ^ipaddr_1, ^ipaddr_2;

		app_handle			= System::Convert::ToInt32( _contenido->Substring( 2, 4 ), 16 );
		mon_handle			= System::Convert::ToInt32( _contenido->Substring( 6, 4 ), 16 );
		call_id				= System::Convert::ToInt32( _contenido->Substring( 10, 4 ), 16 );
		num_rtp				= System::Convert::ToInt32( _contenido->Substring( 14, 1 ), 16 );
		result_1			= System::Convert::ToInt32( _contenido->Substring( 15, 2 ), 16 );
		rtp_port_1			= System::Convert::ToInt32( _contenido->Substring( 17, 4 ), 16 );
		rtp_payload_1		= System::Convert::ToInt32( _contenido->Substring( 21, 2 ), 16 );

		ipaddr_b[0] = _contenido[ _contenido->Length - 4 ];
		ipaddr_b[1] = _contenido[ _contenido->Length - 3 ];
		ipaddr_b[2] = _contenido[ _contenido->Length - 2 ];
		ipaddr_b[3] = _contenido[ _contenido->Length - 1 ];

		ipaddr_1 = gcnew IPAddress( ipaddr_b );

		if ( num_rtp == 2 )
		{
			System::Int32 separator_index = _contenido->IndexOf( '#' );
			if ( separator_index < 0 )
			{
				this->logger->Write( "mon_intercept_connect incorrecto, separador no encontrado", 2 );
				throw gcnew System::Exception();
			}
			result_2			= System::Convert::ToInt32( _contenido->Substring( separator_index + 1, 2 ), 16 );
			rtp_port_2			= System::Convert::ToInt32( _contenido->Substring( separator_index + 3, 4 ), 16 );
			rtp_payload_2		= System::Convert::ToInt32( _contenido->Substring( separator_index + 7, 2 ), 16 );

			ipaddr_b[0] = _contenido[ separator_index - 4 ];
			ipaddr_b[1] = _contenido[ separator_index - 3 ];
			ipaddr_b[2] = _contenido[ separator_index - 2 ];
			ipaddr_b[3] = _contenido[ separator_index - 1 ];

			ipaddr_1 = gcnew IPAddress( ipaddr_b );					// Sobreescribo la ipaddr_1

			ipaddr_b[0] = _contenido[ _contenido->Length - 4 ];
			ipaddr_b[1] = _contenido[ _contenido->Length - 3 ];
			ipaddr_b[2] = _contenido[ _contenido->Length - 2 ];
			ipaddr_b[3] = _contenido[ _contenido->Length - 1 ];

			ipaddr_2 = gcnew IPAddress( ipaddr_b );
		}
		
		System::Int32 index_linea	= find_index_by_handle( app_handle, MON_APP );
		System::Int32 index_mon		= find_mon_index_by_handle( index_linea, mon_handle );

		this->logger->Write( System::String::Format( "[INTERCEPT CONN-{0}] id {1} rtps {2} res1 {3} port1 {4} payld1 {5} ip1 {6} res2 {7} port2 {8} payld2 {9} ip2 {10}",
					index_linea, call_id, num_rtp, result_1, rtp_port_1, rtp_payload_1, ipaddr_1, result_2, rtp_port_2, rtp_payload_2, ipaddr_2 ), 0 );

		// las linea/s tienen que estar conectadas para oir audio, en caso contrario habrá que desconectar la escucha
		if ( ( num_rtp == 1 && result_1 == 0 ) || ( num_rtp == 2 && result_1 == 0 && result_2 == 0 ) )
		{
			if ( call_id != this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->id )
			{
				this->logger->Write( System::String::Format( "mon_intercept_connect CALL ID Distinto del esperado!!, se escucha En linea {0} para el ssi monitorizado {1}, call_id recibido {2}, call_id esperado {3}",
						this->lineas[index_linea]->ssi_linea, this->lineas[index_linea]->monitorizaciones[index_mon]->ssi_mon, call_id, this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->id ), 2 );
				throw gcnew System::Exception();
			}

			this->lineas[index_linea]->monitorizaciones[index_mon]->interviniendo = true;
			this->lineas[index_linea]->monitorizaciones[index_mon]->stop_intervention = false;
			this->logger->Write( System::String::Format( "mon_intercept_connect CORRECTO, se escucha En linea {0} para el ssi monitorizado {1}",
						this->lineas[index_linea]->ssi_linea, this->lineas[index_linea]->monitorizaciones[index_mon]->ssi_mon ), 1 );

			if ( this->lineas[index_linea]->monitorizaciones[index_mon] != this->lineas[index_linea]->mon_item_interviniendo )		// Si no se trata de dos referencias al mismo objeto es un error! no puede haber  dos mon_items intervenidos
			{
				this->logger->Write( "mon_intercept_connect INCORRECTO DOS MON_ITEMS estan siendo intervenidos!", 5 );
			}



			this->print_estado_mon_ssi( index_linea, index_mon );

			this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}:{4}:{5}:{6}:{7}:{8}{9}",
								TRAMA_INI,
								AC_LISTEN_MON_ACK,
								this->lineas[index_linea]->monitorizaciones[index_mon]->interviniendo ? 1:0,
								this->lineas[index_linea]->ssi_linea,
								this->lineas[index_linea]->monitorizaciones[index_mon]->ssi_mon,
								this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->acapi_ip,
								this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->acapi_rtp_port,
								this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->acapi_ip_mon,
								this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->acapi_rtp_port_mon,
								TRAMA_END ) );
		}
		else
		{
			this->logger->Write( System::String::Format( "mon_intercept_connect incorrecto, resultado/s distinto de lineConnected En linea {0} para el ssi monitorizado {1}",
						this->lineas[index_linea]->ssi_linea, this->lineas[index_linea]->monitorizaciones[index_mon]->ssi_mon ), 2 );
			// EMITIR mon_intercept_disconnect!!
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en mon_intercept_connect", 4 );
	}
}

/*
mon_intercept_disconnect/_ack	app_handle	mon_handle	disconnected_ssi	call_id	disconnect_cause
char							int			int			int					int		short
2								4			4			8					4		2

disconnect cause:
	0 = causeUnknown
	1 = userRequestedDisconnect
	2 = calledPartyBusy
	3 = calledPartyNotReachable
	4 = calledPartyDoesNotSupportEncryption
	5 = congestionInInfrastructure
	6 = notAllowedTrafficCase
	7= incompatibleTrafficCase
	8 = requestedServiceNotAvailable
	9 = preEmptiveUseOfResource
	10 = invalidCallIdentifier
	11 = callRejectedByCalledParty
	12 = noIdleCcEntity
	13 = expiryOfTimer
	14 = dmxRequestedDisconnect
	15 = acknowledgedServiceNotCompleted
	16 = resourceFailed
	17 = unknownResource
	18 = inactiveResource
	19 = resourceUsedInOtherCall
	20 = cellReSelection
	21 = callSetupRepetition
	22 = icLeavingCallNoCallEnd Include Call: The issuer leaves the call, but the call shall not be ended
	23 = unknownTetraIdentity
	24 = ssSpecificDisconnection
	25 = unknownExternalSubscriberIdentity
	26 = callRestorationOfOtherUserFailed
	27 = wrongCommunicationType
	28 = callTransferSubscriberError
	29 = unknownVirtualDest
	30 = routingFailureToVirtualDest
	31 = offlineCircuitModeMonitoringGateway
	32 = unknownCallIdentifier
	33 = monToSameVirtualDestAlreadyActive
	34 = unknownMonitoringHandle Connection refused because the monitoring handle is unknown
	35 = callInWrongState Monitoring of call refused because call is in the wrong state

*/
void Consumidor::mon_intercept_disconnect_callback( short tipo, System::String^ _contenido )
{
	try
	{
		System::String^ callback;

		if ( tipo == 45 )
			callback = "MON_INTERCEPT_DISCONNECT";
		if ( tipo == 46 )
			callback = "MON_INTERCEPT_DISCONNECT_ACK";

		System::Int32	app_handle, mon_handle, disconnected_ssi, call_id, cause;

		app_handle			= System::Convert::ToInt32( _contenido->Substring( 2, 4 ), 16 );
		mon_handle			= System::Convert::ToInt32( _contenido->Substring( 6, 4 ), 16 );
		disconnected_ssi	= System::Convert::ToInt32( _contenido->Substring( 10, 8 ), 16 );
		call_id				= System::Convert::ToInt32( _contenido->Substring( 18, 4 ), 16 );
		cause				= System::Convert::ToInt32( _contenido->Substring( 22, 2 ), 16 );
		
		System::Int32 index_linea	= find_index_by_handle( app_handle, MON_APP );
		System::Int32 index_mon		= find_mon_index_by_handle( index_linea, mon_handle );

		if ( call_id != this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->id )
		{
			this->logger->Write( System::String::Format( "{0} CALL ID Distinto del esperado!!, se escuchaba En linea {1}  el ssi monitorizado {2}, call_id recibido {3}, call_id esperado {4}",
						callback, this->lineas[index_linea]->ssi_linea, this->lineas[index_linea]->monitorizaciones[index_mon]->ssi_mon, call_id, this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->id ), 2 );
		}

		this->lineas[index_linea]->monitorizaciones[index_mon]->interviniendo = false;
		this->lineas[index_linea]->monitorizaciones[index_mon]->stop_intervention = false;
		this->logger->Write( System::String::Format( "{0} se deja de escuchar En linea {1} al ssi monitorizado {2}",
						callback, this->lineas[index_linea]->ssi_linea, this->lineas[index_linea]->monitorizaciones[index_mon]->ssi_mon ), 1 );

		if ( this->lineas[index_linea]->monitorizaciones[index_mon] != this->lineas[index_linea]->mon_item_interviniendo )		// Si no se trata de dos referencias al mismo objeto es un error! no puede haber  dos mon_items intervenidos
		{
			this->logger->Write( "disconnect_callback INCORRECTO DOS MON_ITEMS estan siendo intervenidos!", 5 );
		}

		this->lineas[index_linea]->mon_item_interviniendo = nullptr;		// Aqui se borra la segunda referencia del mon_item intervenido

		this->print_estado_mon_ssi( index_linea, index_mon );

		// La informacion de ip y puertos no tiene sentido ahora
		//this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}:{4}:-1:-1:-1:-1{5}",
		//			TRAMA_INI,
		//			AC_LISTEN_MON_ACK,
		//			this->lineas[index_linea]->monitorizaciones[index_mon]->interviniendo ? 1:0,
		//			this->lineas[index_linea]->ssi_linea,
		//			this->lineas[index_linea]->monitorizaciones[index_mon]->ssi_mon,
		//			TRAMA_END ) );
		this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}:{4}:{5}:{6}:{7}:{8}{9}",
					TRAMA_INI,
					AC_LISTEN_MON_ACK,
					this->lineas[index_linea]->monitorizaciones[index_mon]->interviniendo ? 1:0,
					this->lineas[index_linea]->ssi_linea,
					this->lineas[index_linea]->monitorizaciones[index_mon]->ssi_mon,
					this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->acapi_ip,
					this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->acapi_rtp_port,
					this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->acapi_ip_mon,
					this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->acapi_rtp_port_mon,
					TRAMA_END ) );
		
		// Se simula un final de llamada para efectos de lógica de lineas en Core y Oper
		this->manager->avisaAcCore(System::String::Format( "{0}{1}:{2}{3}",
				TRAMA_INI, CC_LLAMADA_ACABADA, index_linea, TRAMA_END ) );


		this->proceso_escucha_rtp( false,  index_linea );
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en disconnect_callback", 4 );
	}
}

/*
mon_tx_demand/ceased	app_handle	mon_handle	timeStamp	involved_ssi	call_id
char					int			int			string		int				int	
2						4			4			14			8				4

mon_tx_demand/ceased	app_handle	mon_handle	timeStamp	callParty_ssi	transmitting_ssi	call_id
char					int			int			string		int				int					int	
2						4			4			14			8				8					4

*/
void Consumidor::mon_tx_callback( short tipo, System::String^ _contenido )
{
	try
	{
		System::String^ callback;

		if ( tipo == 17 )
			callback = "MON_TX_DEMAND";
		if ( tipo == 18 )
			callback = "MON_TX_CEASED";
		if ( tipo == 19 )
			callback = "MON_TX_GRANT";

		System::Int32		app_handle, mon_handle, involved_ssi, call_id;

		app_handle			= System::Convert::ToInt32( _contenido->Substring( 2, 4 ), 16 );
		mon_handle			= System::Convert::ToInt32( _contenido->Substring( 6, 4 ), 16 );
		involved_ssi		= System::Convert::ToInt32( _contenido->Substring( 24, 8 ), 16 );
		if ( tipo == 17 || tipo == 18 )		// MON_TX_DEMAND || MON_TX_CEASED
		{
			involved_ssi		= System::Convert::ToInt32( _contenido->Substring( 24, 8 ), 16 );
			call_id				= System::Convert::ToInt32( _contenido->Substring( 32, 4 ), 16 );
		}
		if ( tipo == 19 )					// MON_TX_GRANT
		{
			System::Int32 call_party_ssi	=  System::Convert::ToInt32( _contenido->Substring( 24, 8 ), 16 );
			involved_ssi		= System::Convert::ToInt32( _contenido->Substring( 32, 8 ), 16 );		// transmitting_ssi
			call_id				= System::Convert::ToInt32( _contenido->Substring( 40, 4 ), 16 );
		}
		
		
		System::DateTime^	time_stamp = this->procesa_timeStamp( _contenido->Substring( 10, 14 ) );

		System::Int32 index_linea	= find_index_by_handle( app_handle, MON_APP );
		System::Int32 index_mon		= find_mon_index_by_handle( index_linea, mon_handle );

		if ( call_id != this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->id )
		{
			this->logger->Write( System::String::Format( "{0} CALL ID Distinto del esperado!!, se escuchaba En linea {1}  el ssi monitorizado {2}, call_id recibido {3}, call_id esperado {4}",
						callback, this->lineas[index_linea]->ssi_linea, this->lineas[index_linea]->monitorizaciones[index_mon]->ssi_mon, call_id, this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->id ), 2 );
			throw gcnew System::Exception();
		}
		switch ( tipo )
		{
		case 17:	// MON_TX_DEMAND
			//	No hago nada con el demand por ahora
			break;
		case 18:	// MON_TX_CEASED
			this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->exist_tx = false;
			this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->tx_granted = false;
			this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->tx_ssi = -1;
			break;
		case 19:	// MON_TX_GRANT
			this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->exist_tx = true;
			this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->tx_granted = ( involved_ssi == this->lineas[index_linea]->ssi_linea );
			this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->tx_ssi = involved_ssi;
			break;
		}
		this->logger->Write( System::String::Format( "{0} En linea {1} ssi monitorizado {2}, call_id {3}, involved_ssi {4}",
						callback, this->lineas[index_linea]->ssi_linea, this->lineas[index_linea]->monitorizaciones[index_mon]->ssi_mon, call_id, involved_ssi ), 1 );

		this->print_estado_mon_ssi( index_linea, index_mon );

		this->manager->avisaAcCore(System::String::Format( "{0}{1}:{2}:{3}:{4}:{5}:{6}{7}",
						TRAMA_INI,
						CC_PTT,
						index_linea,
						this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->tx_req_permission ? 1:0,
						this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->tx_granted ? 1:0,
						this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->exist_tx ? 1:0,
						this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->tx_ssi,
						TRAMA_END ) );
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en mon_tx_callback", 4 );
	}
}

/*
	Orden procedente del manager de llamar a un ssi particular o de grupo, en simplex o duplex

tipo
	110-Simplex, 111-Duplex, 112-Grupo, 144-Ambiente (*)

_contenido
	[index_linea]:[ssi_dst]:[prioridad]

	Si ya existe un recurso de audio ocupado (llamada o monitorizacion intervenida) para la linea requerida 
	habrá que liberarlo (colgar la llamada o mon intercept disconnect). Pra evitar casos conflictivos donde la 
	llamada no se ha podido colgar o puedan existir más de un flujo de audio se vuelve a encolar el evento con
	un tiempo de expiración. Si tras el tiempo de expiración no se ha podido liberar el recurso de audio no habrá sido
	posible realizar la llamada
	
	(*) Las llamadas ambiente son una funcionalidad propia de los despachadores, pero son manejadas por el servicio CC
			=> todas las lineas podrían realmente hacer ambience calls (se ace una comprobación de que son despachadoras)
			=> funcionalmente se deberían hacer sobre ssis monitorizados, pero esto se controla desde el operador
					(NO	SE HACE NINGUNA COMPROBACION DE QUE EL SSI DESTINO ESTA SIENDO MONITORIZADO)
*/
void Consumidor::call_ssi ( short tipo, System::String^ _contenido, System::DateTime^ expiracion )
{
	try
	{
		bool reencola_evento;

		array<System::String^>^ campos = nullptr;
		array<wchar_t>^ separador = {':'};
		campos = _contenido->Split( separador, System::StringSplitOptions::RemoveEmptyEntries );

		System::Int32 index_linea	= System::Convert::ToInt32( campos[0] );
		System::Int32 ssi_dst		= System::Convert::ToInt32( campos[1] );
		System::Int32 prioridad		= System::Convert::ToInt32( campos[2] );

		this->logger->Write( System::String::Format( "intento llamar tipo:{0} linea:{1}, dest:{2}, prioridad:{3}",
			tipo, index_linea, ssi_dst, prioridad ), 1 );

		if ( this->lineas[index_linea]->pinchada )
		{
			// El evento se encolará y desencolará numerosas veces hasta que la linea no tenga ningún recurso de audio ocupado
			this->logger->Write( System::String::Format( "intento llamar linea {0}", index_linea ), -5 );

			/*
				Si la linea tiene un recurso de audio ocupado se intenta liberar y se vuelvo a encolar el evento
				exactamente igual pero con tiempo de expiracion (para evitar bucle indefinido)
			*/
			if ( this->lineas[index_linea]->comunicacion->estado != CC_EMPTY )
			{
				reencola_evento = true;

				if( this->lineas[index_linea]->comunicacion->estado != CC_HANGING_UP )
				{	// Cuelgo solo una vez
					this->cuelga_llamada( index_linea, 1 );
				}
			}
			else if ( this->lineas[index_linea]->mon_item_interviniendo != nullptr )
			{
				reencola_evento = true;

				if ( ! this->lineas[index_linea]->mon_item_interviniendo->stop_intervention )
				{	// mon_intercept_disconnect solo una vez
					this->stop_escucha( index_linea );
				}
			}
			if ( reencola_evento )
			{
				this->logger->Write( "reencolo evento llamada", -5 );
				EventoCola^ eventoIgual = gcnew EventoCola();
				eventoIgual->tipo = tipo;
				eventoIgual->contenido = _contenido;
				
				System::DateTime^ evento_expira;
				
				if( expiracion == nullptr )
				{
					evento_expira = System::DateTime::Now;
					evento_expira = evento_expira->AddSeconds( EVENTO_EXPIRA_SEC );
				}
				else
				{
					evento_expira = expiracion;
				}

				eventoIgual->expira = evento_expira;

				this->cola->Enqueue(eventoIgual);

				if( this->permiso_continua )	// Solo continuaré consumiendo si hay permiso
					this->continua->Set();

				return;
			}

			char src_ip[5];
			memset( &src_ip, 0, sizeof( src_ip ) );

			array<System::Byte>^ ip_bytes = this->asterisk_ip->GetAddressBytes();

			for( int i=0; i < ip_bytes->Length; i++)
			{
				src_ip[i] = ip_bytes[i];
			}

			System::IntPtr p = Marshal::StringToHGlobalAnsi( this->asterisk_ip->ToString() );
			//char *aux_char = static_cast<char*>( p.ToPointer() );
			//strcat( src_ip, aux_char );
				
			Marshal::FreeHGlobal(p);

			System::Int32 app_handle = this->lineas[index_linea]->cc->handle;
			System::Int32 asterisk_port = this->lineas[index_linea]->puerto_asterisk;
		
			switch ( tipo )
			{
				case AC_CALL_DUPLEX:
					if ( this->tetra->call_duplex( app_handle, ssi_dst, src_ip, asterisk_port, CC_RTP_PAYLOAD_TYPE, CC_HOOK_SIGNALLING, CC_PRIORITY_TYPE, prioridad, CC_ENCRYPTION ) )
					{
						this->logger->Write( "call_RtpDuplex emitido correctamente", 1 );
						this->lineas[index_linea]->comunicacion->tipo = CC_TIPO_DUPLEX;
						this->lineas[index_linea]->comunicacion->estado = CC_ORDER_SENT;
						this->lineas[index_linea]->comunicacion->priority = prioridad;
						this->print_estado_linea( index_linea );
					}
					else
					{
						this->logger->Write( "call_RtpDuplex NO EMITIDO!!!!!", 4 );
					}
					break;
				case AC_CALL_SIMPLEX:
					if ( this->tetra->call_simplex( app_handle, ssi_dst, src_ip, asterisk_port, CC_RTP_PAYLOAD_TYPE, CC_HOOK_SIGNALLING, CC_PRIORITY_TYPE, prioridad, CC_ENCRYPTION ) )
					{
						this->logger->Write( "call_RtpSimplex emitido correctamente", 1 );
						this->lineas[index_linea]->comunicacion->tipo = CC_TIPO_POINTTOPOINT;
						this->lineas[index_linea]->comunicacion->estado = CC_ORDER_SENT;
						this->lineas[index_linea]->comunicacion->priority = prioridad;
						this->print_estado_linea( index_linea );
					}
					else
					{
						this->logger->Write( "call_RtpSimplex NO EMITIDO!!!!!", 4 );
					}
					break;
				case AC_CALL_GROUP:
					if ( this->tetra->call_group( app_handle, ssi_dst, src_ip, asterisk_port, CC_RTP_PAYLOAD_TYPE, CC_HOOK_SIGNALLING, CC_PRIORITY_TYPE, prioridad, CC_ENCRYPTION ) )
					{
						this->logger->Write( "call_RtpGroup emitido correctamente", 1 );
						this->lineas[index_linea]->comunicacion->tipo = CC_TIPO_POINTTOMULTIPOINT;
						this->lineas[index_linea]->comunicacion->estado = CC_ORDER_SENT;
						this->lineas[index_linea]->comunicacion->priority = prioridad;
						this->print_estado_linea( index_linea );
					}
					else
					{
						this->logger->Write( "call_RtpGroup NO EMITIDO!!!!!", 4 );
					}
					break;

				case AC_CALL_LISTEN_AMBIENCE:
					if ( ! this->lineas[index_linea]->despachador )
					{
						this->logger->Write( System::String::Format( "¡¡¡¡call_AmbienceListen en linea [{0}] no despachadora!!!!", index_linea ), 2 );
						throw gcnew System::Exception( );
					}
					if ( this->tetra->call_listen_ambience( app_handle, ssi_dst, src_ip, asterisk_port, CC_RTP_PAYLOAD_TYPE, CC_HOOK_SIGNALLING, CC_PRIORITY_TYPE, prioridad, 1, CC_ENCRYPTION ) )
					{
						this->logger->Write( "call_AmbienceListen emitido correctamente", 1 );
						this->lineas[index_linea]->comunicacion->tipo = CC_AMBIENCE_LISTEN;
						this->lineas[index_linea]->comunicacion->estado = CC_ORDER_SENT;
						this->lineas[index_linea]->comunicacion->priority = prioridad;
						this->print_estado_linea( index_linea );
					}
					else
					{
						this->logger->Write( "call_AmbienceListen NO EMITIDO!!!!!", 4 );
					}
					break;
			}
		}
		else
		{
			this->logger->Write( System::String::Format( "¡¡¡¡se intento llamar por la linea {0} NO pinchada!!!!", index_linea ), 2 );
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en Consumidor::call_duplex", 4 );
	}
}

/*
	Orden procedente del manager de des/pinchar una linea.
	
	Las líneas que estén pinchadas cogerán llamadas que estén a la espera en ese momento, y automáticamente
	aceptarán llamadas entrantes, independientemente de si son con HOOK SIGNALLING o NO.

	Al despinchar una línea se colgará cualquier llamada en curso, además si se cambió de canal mientras estaba pinchada,
	se intentará volver al canal por defecto si se indica así en la configuración ( "change_group_to_default" )

	[index_linea]:[pinchada?]
*/
void Consumidor::pincha_linea ( System::String^ _contenido )
{
	try
	{
		bool cuelgo, stop_escucha, cambio_canal;

		array<System::String^>^ campos = nullptr;
		array<wchar_t>^ separador = {':'};
		campos = _contenido->Split( separador, System::StringSplitOptions::RemoveEmptyEntries );

		System::Int32 index_linea	= System::Convert::ToInt32( campos[0] );
		bool pinchada				= System::Convert::ToInt32( campos[1] ) != 0;

		System::Int32 call_id = this->lineas[index_linea]->comunicacion->id;
		this->lineas[index_linea]->pinchada = pinchada;

		if ( ! this->lineas[index_linea]->activa )
		{
			this->logger->Write( System::String::Format( "Se ignora DES/PINCHA en linea {0} no activa!", index_linea ), 4 );
			return;
		}

		if ( pinchada )			// PINCHA linea y llamada existente en la linea
		{
			this->logger->Write( System::String::Format( "---------PINCHA linea:{0} ", index_linea ), 0 );

			if ( call_id != -1 )
			{
				if ( this->lineas[index_linea]->comunicacion->estado != CC_ESTABLISHED )
				{	// Si la llamada ya estaba establecida no la intento aceptar de nuevo
					this->acepta_llamada( index_linea );
				}
				else
				{
					// Se manda por necesesidades del AcCore con la modelapi, si un operador se sale de una llamada de grupo, esta llamada
					// sigue establecida, si se vuelve a pinchar la misma linea, la modelApi necesita informacion de nuevo sobre la ip y puerto de la comunicacion
					this->manager->avisaAcCore(System::String::Format( "{0}{1}:{2}:{3}:{4}:{5}:{6}:{7}{8}",
							TRAMA_INI, CC_LLAMADA_ESTABLECIDA, index_linea, this->lineas[index_linea]->comunicacion->tipo,
							this->lineas[index_linea]->comunicacion->ssi_destino, this->lineas[index_linea]->comunicacion->priority,
							this->lineas[index_linea]->comunicacion->acapi_ip, this->lineas[index_linea]->comunicacion->acapi_rtp_port, TRAMA_END ) );
				}
			}
		}
		else if( ! pinchada )		// DESPINCHA linea
		{
			/*
				Si el cambio de canal se debe hacer por defecto ("change_group_to_default" TRUE) se cambiará de canal al despinchar si la linea
					esta en un canal asociado distinto al defecto. En este caso también se colgará la llamada en curso.
				Si no hay que cambiar de canal, las llamadas de grupo en curso que coincidan con el canal asociado se dejarán sin colgar para poder 
				unirse a ella a posteriori. ESTO ES DEBIDO A QUE SE HA OBSERVADO QUE AL ABANDONAR UNA LLAMADA DE GRUPO, NO SE PUEDE VOLVER A ELLA HASTA
				QUE SE ESTABLEZCA OTRA DISTINTA

				ES NECESARIO AL MENOS SOLTAR EL PTT para que no se quede la linea transmitiendo hasta el final de la llamada
			*/

			// Si hay llamada activa el caso por defecto es colgar
			if ( call_id != -1 )
				cuelgo = true;

			if ( this->change_group_to_default )
			{
				if ( this->lineas[index_linea]->ssi_grupo_asociado == this->lineas[index_linea]->ssi_grupo_defecto )
				{
					cambio_canal = false;
					if ( ( call_id != -1 ) && ( this->lineas[index_linea]->comunicacion->ssi_destino == this->lineas[index_linea]->ssi_grupo_asociado ) )
						cuelgo = false;
				}
				else
				{
					cambio_canal = true;
				}
			}
			else
			{
				cambio_canal = false;
				if ( ( call_id != -1 ) && ( this->lineas[index_linea]->comunicacion->ssi_destino == this->lineas[index_linea]->ssi_grupo_asociado ) )
					cuelgo = false;
			}

			if ( this->lineas[index_linea]->mon_item_interviniendo != nullptr )		// Hay una escucha activa o iniciandose/parandose ( casos muy improbables, la intento parar de todas maneras )
			{
				stop_escucha = true;
				this->stop_escucha( index_linea );
			}


			this->logger->Write( System::String::Format( "---------DESPINCHA. Cuelgo? {0} stopEscucha? {1} Cambio de Canal? {2}", cuelgo, stop_escucha, cambio_canal ), 0 );

			if ( stop_escucha && cuelgo )		// NO DEBERIA DE OCURRIR NUNCA, dos recursos de audio para la misma linea!
				this->logger->Write( "////////////// ERROR LOGICO, una linea con llamada y escucha en CURSO!!!!", 5 );

			if ( cuelgo )
			{	// Si es una llamada de grupo dejo la llamada abierta!! para poder unirme más tarde (simplemente abrir canal asterix-operador)
				this->cuelga_llamada( index_linea, 1 );
			}
			else if ( ( call_id != -1 ) && this->lineas[index_linea]->comunicacion->exist_tx )	// Despincha con linea activa pero NO CUELGO!! => hay que soltar el PTT si se está transmitiendo
			{
				this->logger->Write( "No se cuelga la llamada!! Se intenta soltar el PTT", 1 );
				this->ptt( System::String::Format( "{0}:0:0", index_linea ) );					// Suelta '0' el ptt en la linea 'index_linea'
			}

			if ( cambio_canal )
			{
				this->logger->Write( System::String::Format( "¡¡¡¡change_group_to_default!!! Se intenta cambiar linea {0} a su grupo por defecto {1}",
											index_linea, this->lineas[index_linea]->ssi_grupo_defecto ), 0 );
				this->lineas[index_linea]->attached = false;
				this->cambia_grupo_asociado( System::String::Format( "{0}:{1}", index_linea, this->lineas[index_linea]->ssi_grupo_defecto ) );
			}
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en Consumidor::pincha_linea", 4 );
	}
}

/*
	Acepta una llamada entrante, lógica para reconocer si es de grupo o individual
*/
void Consumidor::acepta_llamada ( System::Int32 index_linea )
{
	try
	{
		System::Int32 group_ssi;

		//char src_ip[20];
		//	memset( &src_ip, 0, sizeof( src_ip ) );

		char src_ip[5];
		memset( &src_ip, 0, sizeof( src_ip ) );

		array<System::Byte>^ ip_bytes = this->asterisk_ip->GetAddressBytes();

		for( int i=0; i < ip_bytes->Length; i++)
		{
			src_ip[i] = ip_bytes[i];
		}

		System::IntPtr p = Marshal::StringToHGlobalAnsi( this->asterisk_ip->ToString() );
		//char *aux_char = static_cast<char*>( p.ToPointer() );
		//strcat( src_ip, aux_char );

		Marshal::FreeHGlobal(p);

		System::Int32 app_handle	= this->lineas[index_linea]->cc->handle;
		System::Int32 call_id		= this->lineas[index_linea]->comunicacion->id;
		System::Int32 asterisk_port = this->lineas[index_linea]->puerto_asterisk;
		switch ( this->lineas[index_linea]->comunicacion->tipo )
		{
			case CC_TIPO_POINTTOPOINT:
			case CC_TIPO_DUPLEX:
				if ( this->tetra->acepta_llamada( app_handle, call_id, src_ip, asterisk_port, CC_RTP_PAYLOAD_TYPE, CC_ENCRYPTION ) )
				{
					this->logger->Write( "callRtpAccept emitido correctamente", 0 );

					this->lineas[index_linea]->comunicacion->estado = CC_ACCEPTING;
					this->print_estado_linea( index_linea );
				}
				else
				{
					this->logger->Write( "callRtpAccept NO EMITIDO!!!!!", 4 );
				}
				break;
			case CC_TIPO_POINTTOMULTIPOINT:
				group_ssi = this->lineas[index_linea]->comunicacion->ssi_destino;
				if ( this->tetra->acepta_llamada_grupo( app_handle, call_id, group_ssi, src_ip, asterisk_port, CC_RTP_PAYLOAD_TYPE, CC_ENCRYPTION ) )
				{
					this->logger->Write( "callRtpAcceptGroup emitido correctamente", 0 );

					this->lineas[index_linea]->comunicacion->estado = CC_ACCEPTING;
					this->print_estado_linea( index_linea );
				}
				else
				{
					this->logger->Write( "callRtpAcceptGroup NO EMITIDO!!!!!", 4 );
				}
				break;
			default:
				this->logger->Write( "acepta_llamada de un tipo no manejado!!!!!", 2 );
				break;
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en Consumidor::acepta_llamada", 4 );
	}
}

/*
	Cuelga la linea especificada, se emite un CC_DISCONNECT con la "causa" especificada

	0 - causeUnknown
	1 - userRequestedDisconnect
	2 - calledPartyBusy
	...
*/
void Consumidor::cuelga_llamada ( System::Int32 index_linea, System::Int32 causa )
{
	try
	{
		System::Int32 app_handle	= this->lineas[index_linea]->cc->handle;
		System::Int32 call_id		= this->lineas[index_linea]->comunicacion->id;

		if ( this->lineas[index_linea]->comunicacion->id != -1 )
		{
			if ( this->tetra->cuelga_llamada( app_handle, call_id, causa ) )
			{
				this->logger->Write( "callDisconnect emitido correctamente", 1 );

				this->lineas[index_linea]->comunicacion->estado = CC_HANGING_UP;
				this->print_estado_linea( index_linea );
			}
			else
			{
				this->logger->Write( "callDisconnect NO EMITIDO!!!!!", 4 );
			}
		}
		else
		{
			this->logger->Write( "Se ha intentado colgar una linea vacia", 2 );
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en Consumidor::cuelga_llamada", 4 );
	}
}

/*
	Orden de pulsar el PTT o despulsarlo
	Se comprueba el recurso de audio ocupado y se mandan ordenes distintas dependiendo de si es una llamada convencional
		o una llamada monitorizada interceptada

	Si hay una linea libre y asciada a un grupo, se iniciará una llamada automaticamente al grupo asociado

	AcapiDocumentation, txPriorityType] prioridad = 2 

	[index_linea]:[pulsado?]:[prioridad]
*/
void Consumidor::ptt ( System::String^ _contenido )
{
	try
	{
		array<System::String^>^ campos = nullptr;
		array<wchar_t>^ separador = {':'};
		campos = _contenido->Split( separador, System::StringSplitOptions::RemoveEmptyEntries );

		System::Int32 index_linea	= System::Convert::ToInt32( campos[0] );
		System::Int32 pulsado		= System::Convert::ToInt32( campos[1] );
		System::Int32 prioridad		= System::Convert::ToInt32( campos[2] );

		if ( ! ( pulsado == 0 || pulsado == 1 ) || prioridad < 0 || prioridad > 5 )
		{
			this->logger->Write( "Consumidor::ptt, parametros incorrectos", 4 );
			throw gcnew System::Exception();
		}

		System::Int32 app_handle, mon_handle, call_id;
		
		if ( this->lineas[index_linea]->mon_item_interviniendo != nullptr )		// intervenir en la llamada interceptada por el despachador
		{
			app_handle	= this->lineas[index_linea]->mon->handle;
			mon_handle	= this->lineas[index_linea]->mon_item_interviniendo->mon_handle;
			call_id		= this->lineas[index_linea]->mon_item_interviniendo->comunicacion->id;
			
			if ( this->tetra->mon_ptt ( app_handle, mon_handle, this->lineas[index_linea]->ssi_linea, call_id, pulsado, prioridad ) )
			{
				this->logger->Write( "-- MON ptt emitido correctamente", 0 );
			}
			else
			{
				this->logger->Write( "-- MON ptt NO EMITIDO!!!!!", 4 );
			}
		}
		else
		{
			if ( pulsado == 1 && this->lineas[index_linea]->comunicacion->id == -1  && this->lineas[index_linea]->attached )
			{	// llamada no establecida
				this->call_ssi( AC_CALL_GROUP, System::String::Format( "{0}:{1}:7", index_linea, this->lineas[index_linea]->ssi_grupo_asociado ), nullptr );
			}
			else
			{	// ptt convencional
				app_handle	= this->lineas[index_linea]->cc->handle;
				call_id		= this->lineas[index_linea]->comunicacion->id;

				if ( this->tetra->ptt( app_handle, call_id, pulsado, prioridad ) )
				{
					this->logger->Write( "ptt emitido correctamente", 0 );
				}
				else
				{
					this->logger->Write( "ptt NO EMITIDO!!!!!", 4 );
				}
			}
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en Consumidor::ptt", 4 );
	}
}

/*
	grupo_asociado_deseado == -1 => detach del grupo asociado únicamente, dejar la línea desasociada

	[index_linea]:[grupo_asociado_deseado]
*/
void Consumidor::cambia_grupo_asociado ( System::String^ _contenido )
{
	try
	{
		array<System::String^>^ campos = nullptr;
		array<wchar_t>^ separador = {':'};
		campos = _contenido->Split( separador, System::StringSplitOptions::RemoveEmptyEntries );

		System::Int32 index_linea	= System::Convert::ToInt32( campos[0] );
		System::Int32 grupo_asociado_deseado = System::Convert::ToInt32( campos[1] );

		System::Int32 ssi_linea		= this->lineas[index_linea]->ssi_linea;

		this->logger->Write( System::String::Format( "CAMBIA_GRUPO_ASOCIADO linea {0}, grupo deseado{1}", index_linea, grupo_asociado_deseado ), 0 );

		if ( this->lineas[index_linea]->comunicacion->id != -1 )
		{
			this->cuelga_llamada( index_linea, 1 );
			this->logger->Write( System::String::Format( ".-.-.-.cuelgo llamada por cambio de canal en linea {0}", index_linea ), 1 );
		}

		// 
		if ( grupo_asociado_deseado == -1 )
		{
			if ( ! this->db->update_attached( this->lineas[index_linea]->ssi_grupo_asociado, ssi_linea, PER_DETACH_PENDING ) )
			{
				this->logger->Write( "problema en db haciendo detach grupo previo cambia_grupo_asociado", 4 );
			}
			else
			{
				this->lineas[index_linea]->attached = false;
			}

		}
		else if ( this->lineas[index_linea]->ssi_grupo_asociado == grupo_asociado_deseado )
		{	// El grupo a cambiar es el mismo, se intenta hacer un attach de nuevo. Si no existe la pertenencia se pone en vacío el grupo asociado!!
			if ( this->db->update_attached( grupo_asociado_deseado, ssi_linea, PER_ATTACH_PENDING ) )
			{	// Existe la pertenencia
				this->lineas[index_linea]->attached = false;
			}
			else
			{
				this->logger->Write( System::String::Format( "INCOHERENCIA Se intento cambiar al mismo grupo {0}, y no existe en pertenencias!!!!", grupo_asociado_deseado ), 2 );
				this->lineas[index_linea]->attached = false;
				this->lineas[index_linea]->ssi_grupo_asociado = -1;
			}
		}
		// si el grupo es distinto, se comprueba la pertenencia. Si existe se intenta detach del grupo anterior y se actualizan los campos necesarios
		else if ( this->db->update_attached( grupo_asociado_deseado, ssi_linea, PER_ATTACH_PENDING ) )
		{	// Existe la pertenencia
			if ( ! this->db->update_attached( this->lineas[index_linea]->ssi_grupo_asociado, ssi_linea, PER_DETACH_PENDING ) )
				this->logger->Write( "problema en db haciendo detach grupo previo cambia_grupo_asociado", 4 );

			this->lineas[index_linea]->ssi_grupo_asociado = grupo_asociado_deseado;
			this->lineas[index_linea]->attached = false;
		}
		// NO existe la pertenencia, sigo attached al grupo en el que estaba. NO HAGO NADA. LO COMUNICO
		this->manager->avisaAcCore( System::String::Format( "{0}{1}:{2}:{3}:{4}{5}",
							TRAMA_INI, CAMBIA_GRUPO_LINEA_ACK, index_linea, this->lineas[index_linea]->ssi_grupo_asociado,
							this->lineas[index_linea]->attached ? 1 : 0, TRAMA_END ) );
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en cambia_grupo_asociado", 4 );
	}
}

/*
	mon_ssi == -1 => elimina todas las monitorizaciones del despachador especificado.

	[A-D]:[ssi_despachador]:[mon_ssi]
*/
void Consumidor::modifica_item_mon ( System::String^ _contenido )
{
	try
	{
		array<System::String^>^ campos = nullptr;
		array<wchar_t>^ separador = {':'};
		campos = _contenido->Split( separador, System::StringSplitOptions::RemoveEmptyEntries );

		if ( campos[0] != "A" && campos[0] != "D" )
		{
			this->logger->Write( "!!!modifica_item_mon mal formateado", 4 );
			throw gcnew System::Exception();
		}

		bool agrega						= ( campos[0] == "A" ) ? true : false;
		System::Int32 ssi_despachador	= System::Convert::ToInt32( campos[1] );
		System::Int32 ssi_mon			= System::Convert::ToInt32( campos[2] );

		System::Int32 index_linea		= this->find_index_by_ssi( ssi_despachador );

		if ( ! this->lineas[index_linea]->despachador )
		{
			this->logger->Write( System::String::Format( "!!!modifica_item_mon linea NO despachadora {0}", ssi_despachador ), 2 );
			throw gcnew System::Exception();
		}

		System::Int32 index_mon;

		if ( agrega )
		{		// check si el mismo ssi ya esta monitorizado. Añade nuevo mon_item en caso contrario
			if ( this->lineas[index_linea]->monitorizaciones->Count < NO_MAX_MON_ITEMS_LINEA )
			{
				index_mon = find_mon_index_by_ssi( index_linea, ssi_mon );
			
				if ( index_mon >= 0 )
				{
					this->logger->Write( System::String::Format( "!!!modifica_item_mon ADD ssi_mon {0} YA ESTABA MONITORIZADO por {1}. INTENTO ELIMINAR EL ANTERIOR HANDLE", ssi_mon, ssi_despachador ), 2 );
					if ( ! this->tetra->monitoring_close_order( this->lineas[index_linea]->mon->handle, this->lineas[index_linea]->monitorizaciones[index_mon]->mon_handle ) )
						this->logger->Write( "------- fallo mandando orden de CERRAR monitorizacion SINGLE al servidor acapi", 4 );
					this->lineas[index_linea]->monitorizaciones->RemoveAt( index_mon );
				}

				// añade mon_item. En este punto solo se establece el ssi_linea, que en monitorizacion designará al ssi monitorizado
				Monitorizacion_item^ mon_item = gcnew Monitorizacion_item( ssi_mon );

				this->lineas[index_linea]->monitorizaciones->Add( mon_item );
				index_mon = find_mon_index_by_ssi( index_linea, ssi_mon );
			
				this->logger->Write( System::String::Format( "____orden monreq a EMITIR linea {0}, MON_HANDLE {1} ssi_mon {2}", index_linea, this->lineas[index_linea]->mon->handle, ssi_mon ), 6 );
				// emite orden tetra!
				if ( ! this->tetra->monitoring_orderreq_single( this->lineas[index_linea]->mon->handle, 0, 0, 1, ssi_mon ) )
				{
					this->logger->Write( "------- fallo mandando orden de monitorizacion SINGLE al servidor acapi", 4 );
					this->lineas[index_linea]->monitorizaciones->RemoveAt( index_mon );										// No se ha podido mandar la orden, elimino el item
				}
				else
					print_estado_mon_ssi( index_linea, index_mon );
			}
			else
			{
				this->logger->Write( System::String::Format( "!!!modifica_item_mon ADD despachador {0} tiene demasiados items monitorizados", ssi_despachador ), 2 );
			}
		}
		else
		{		// encuentra el ssi monitorizado y elimina la monitorizacion
			if ( ssi_mon == -1 )
			{	// borrar todas las monitorizaciones del despachador
				this->logger->Write( System::String::Format( "!!!modifica_item_mon Se intenta borrar todas las monitorizaciones del despachador [{0}-{1}]", index_linea, ssi_despachador ), 1 );
				for ( int i=0; i<this->lineas[index_linea]->monitorizaciones->Count; i++ )
				{
					this->tetra->monitoring_close_order( this->lineas[index_linea]->mon->handle, this->lineas[index_linea]->monitorizaciones[i]->mon_handle );
				}
			}
			else
			{
				index_mon = find_mon_index_by_ssi( index_linea, ssi_mon );

				if ( index_mon == -1 )
				{
					this->logger->Write( System::String::Format( "!!!modifica_item_mon DELETE ssi_mon {0} NO ESTABA MONITORIZADO por {1}", ssi_mon, ssi_despachador ), 2 );
					throw gcnew System::Exception();
				}
				// emite orden tetra!
				if ( ! this->tetra->monitoring_close_order( this->lineas[index_linea]->mon->handle, this->lineas[index_linea]->monitorizaciones[index_mon]->mon_handle ) )
					this->logger->Write( "------- fallo mandando orden de CERRAR monitorizacion SINGLE al servidor acapi", 4 );
			}
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en modifica_item_mon", 4 );
	}
}

/*
_contenido
	[0-1]:[ssi_despachador]:[mon_ssi]

	0 - escucha = true
	1 - escucha = false

	Al recibir una orden de escuchar habrá que comprobar si existe un recurso de audio ocupado, ya sea de monitorizacion o de llamada convencional
	Se intentará liberar el recurso ocupado tras lo que se encolará de nuevo el evento con un tiempo de expiración. Si pasado el tiempo de expiración
	no se ha conseguido liberar el recurso de audio no se habrá podido interceptar la llamada monitorizada.

*/
void Consumidor::escucha_item_mon ( System::String^ _contenido, System::DateTime^ expiracion )
{
	try
	{
		bool reencola_evento;

		array<System::String^>^ campos = nullptr;
		array<wchar_t>^ separador = {':'};
		campos = _contenido->Split( separador, System::StringSplitOptions::RemoveEmptyEntries );

		if ( campos[0] != "0" && campos[0] != "1" )
		{
			this->logger->Write( "!!!escucha_item_mon mal formateado", 4 );
			throw gcnew System::Exception();
		}

		bool escucha					= ( System::Convert::ToInt32( campos[0] ) == 1 ) ? true : false;
		System::Int32 ssi_despachador	= System::Convert::ToInt32( campos[1] );
		System::Int32 ssi_mon			= System::Convert::ToInt32( campos[2] );

		System::Int32 index_linea		= this->find_index_by_ssi( ssi_despachador );

		if ( ! this->lineas[index_linea]->despachador )
		{
			this->logger->Write( System::String::Format( "!!!escucha_item_mon linea NO despachadora {0}", ssi_despachador ), 2 );
			throw gcnew System::Exception();
		}

		if ( ! this->lineas[index_linea]->pinchada)
		{
			this->logger->Write( System::String::Format( "!!!escucha_item_mon en linea {0} no pinchada", index_linea ), 2 );
			throw gcnew System::Exception();
		}

		System::Int32 index_mon = find_mon_index_by_ssi( index_linea, ssi_mon );
		if ( index_mon == -1 )
		{
			this->logger->Write( System::String::Format( "!!!escucha_item_mon ESCUCHA INCOHERENTE un ssi_mon {0} desde el despachador {1}", ssi_mon, ssi_despachador ), 2 );
			throw gcnew System::Exception();
		}

		if ( escucha )
		{

			if ( this->lineas[index_linea]->comunicacion->estado != CC_EMPTY )
			{
				reencola_evento = true;

				if( this->lineas[index_linea]->comunicacion->estado != CC_HANGING_UP )
				{	// Cuelgo solo una vez
					this->cuelga_llamada( index_linea, 1 );
				}
			}
			else if ( this->lineas[index_linea]->mon_item_interviniendo != nullptr )
			{
				reencola_evento = true;

				if ( ! this->lineas[index_linea]->mon_item_interviniendo->stop_intervention )
				{	// mon_intercept_disconnect solo una vez
					this->stop_escucha( index_linea );
				}
			}
			if ( reencola_evento )
			{
				this->logger->Write( "reencolo evento escucha", -5 );
				EventoCola^ eventoIgual = gcnew EventoCola();
				eventoIgual->tipo = AC_LISTEN_MON_SSI;
				eventoIgual->contenido = _contenido;
				
				System::DateTime^ evento_expira;
				
				if( expiracion == nullptr )
				{
					evento_expira = System::DateTime::Now;
					evento_expira = evento_expira->AddSeconds( EVENTO_EXPIRA_SEC );
				}
				else
				{
					evento_expira = expiracion;
				}

				eventoIgual->expira = evento_expira;

				this->cola->Enqueue(eventoIgual);

				if( this->permiso_continua )	// Solo continuaré consumiendo si hay permiso
					this->continua->Set();
				
				return;
			}


			if ( this->lineas[index_linea]->monitorizaciones[index_mon]->comunicacion->estado == MON_CALL_CONNECT )
			{
				Linea_tetra^ linea = this->lineas[index_linea];
				Monitorizacion_item^ mon_item = this->lineas[index_linea]->monitorizaciones[index_mon];

				char src_ip[5];
				memset( &src_ip, 0, sizeof( src_ip ) );
				char src_ip2[5];
				memset( &src_ip2, 0, sizeof( src_ip2 ) );

				array<System::Byte>^ ip_bytes = this->asterisk_ip->GetAddressBytes();

				for( int i=0; i < ip_bytes->Length; i++)
				{
					src_ip[i] = ip_bytes[i];
					src_ip2[i] = ip_bytes[i];
				}

				if ( this->tetra->mon_intercept( linea->mon->handle, mon_item->mon_handle, linea->ssi_linea, mon_item->comunicacion->id, src_ip, linea->puerto_asterisk, CC_RTP_PAYLOAD_TYPE ) )
				{
					this->logger->Write( System::String::Format( "!!!EMITO mon_intercept!! al ssi_mon {0} desde el despachador {1}", ssi_mon, ssi_despachador ), 1 );
					
					// Es aqui cuando creo una copia de la referencia al mon_item intervenido para la linea tetra, será el único item intervenido hasta que se acabe la escucha o se reemplace por otro
					this->lineas[index_linea]->mon_item_interviniendo = mon_item;
					this->lineas[index_linea]->mon_item_interviniendo->interviniendo = false;		// habrá que esperar al ack ( mon_intercept_ack ) para confirmar que se esta interveniendo

					this->proceso_escucha_rtp( true,  index_linea );
				}
				else
				{
					this->logger->Write( System::String::Format( "!!!escucha_item_mon NO SE PUDO EMITIR mon_intercept!! al ssi_mon {0} desde el despachador {1}", ssi_mon, ssi_despachador ), 4 );
				}
			}
			else
			{
				this->logger->Write( System::String::Format( "!!!escucha_item_mon ESCUCHA un ssi_mon {0} con la llamada no establecida desde el despachador {1}", ssi_mon, ssi_despachador ), 2 );
			}
		}
		else
		{
			this->stop_escucha( index_linea );
		}

		this->print_estado_mon_ssi( index_linea, index_mon );
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en escucha_item_mon", 4 );
	}
}

/*
	Método que intenta dejar de escuchar una llamada monitorizada. Para indicar que ya se ha intentado mon_intercept_disconnect se
	deja registro en la variable "stop_intervention" del "mon_item_interviniendo" de la linea tetra
*/
void Consumidor::stop_escucha( System::Int32 index_linea )
{
	try
	{
		Linea_tetra^ linea = this->lineas[index_linea];
		Monitorizacion_item^ mon_item = this->lineas[index_linea]->mon_item_interviniendo;
		
		if ( this->lineas[index_linea]->mon_item_interviniendo != nullptr )
		{
			if ( this->tetra->mon_intercept_disconnect( linea->mon->handle, mon_item->mon_handle, linea->ssi_linea, mon_item->comunicacion->id ) )
			{
				this->logger->Write( System::String::Format( "!!!EMITO mon_intercept_disconnect!! al ssi_mon {0} desde el despachador {1}", mon_item->ssi_mon, linea->ssi_linea ), 1 );
				mon_item->stop_intervention = true;
			}
			else
			{
				this->logger->Write( System::String::Format( "!!!stop_escucha NO SE PUDO EMITIR mon_intercept_disconnect!! al ssi_mon {0} desde el despachador {1}", mon_item->ssi_mon, linea->ssi_linea ), 4 );
			}
		}
		else
		{
			this->logger->Write( System::String::Format( "!!!stop_escucha NO ESTABA INTERVINIENDO!! al ssi_mon {0} desde el despachador {1}", mon_item->ssi_mon, linea->ssi_linea ), 2 );
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en stop_escucha", 4 );
	}
}

/*
	[ssi_despachador]:[mon_ssi]
*/
void Consumidor::force_end_mon_call ( System::String^ _contenido )
{
	try
	{
		array<System::String^>^ campos = nullptr;
		array<wchar_t>^ separador = {':'};
		campos = _contenido->Split( separador, System::StringSplitOptions::RemoveEmptyEntries );

		System::Int32 ssi_despachador	= System::Convert::ToInt32( campos[0] );
		System::Int32 ssi_mon			= System::Convert::ToInt32( campos[1] );

		System::Int32 index_linea		= this->find_index_by_ssi( ssi_despachador );

		if ( ! this->lineas[index_linea]->despachador )
		{
			this->logger->Write( System::String::Format( "!!!force_end_mon_call linea NO despachadora {0}", ssi_despachador ), 2 );
			throw gcnew System::Exception();
		}

		System::Int32 index_mon = find_mon_index_by_ssi( index_linea, ssi_mon );
		if ( index_mon == -1 )
		{
			this->logger->Write( System::String::Format( "!!!force_end_mon_call ESCUCHA un ssi_mon {0} con la llamada no establecida desde el despachador {1}", ssi_mon, ssi_despachador ), 2 );
			throw gcnew System::Exception();
		}

		Monitorizacion_item^ mon_item = this->lineas[index_linea]->monitorizaciones[index_mon];

		if( this->tetra->mon_force_end_call( this->lineas[index_linea]->mon->handle, mon_item->mon_handle, mon_item->comunicacion->id, 1 ) )
		{
			this->logger->Write( System::String::Format( "Emito ForceCallEnd desde el despachador {0} a la llamada id{1} src{2} dst{3}",
								ssi_despachador, mon_item->comunicacion->id, mon_item->comunicacion->ssi_origen, mon_item->comunicacion->ssi_destino ), 1 );
		}
		else
		{
			this->logger->Write( System::String::Format( "NO SE PUDO EMITIR ForceCallEnd desde el despachador {0} a la llamada id:{1} src:{2} dst:{3}",
								ssi_despachador, mon_item->comunicacion->id, mon_item->comunicacion->ssi_origen, mon_item->comunicacion->ssi_destino ), 4 );
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en force_end_mon_call", 4 );
	}
}

/*
	Resetea todos los ssi monitorizados de un despachador
	[ssi_despachador]
*/
void Consumidor::reset_mon_items ( System::String^ _contenido )
{
	try
	{
		System::Int32 ssi_despachador	= System::Convert::ToInt32( _contenido );
		System::Int32 index_linea		= find_index_by_ssi( ssi_despachador );

		if ( ! this->lineas[index_linea]->despachador )
		{
			this->logger->Write( System::String::Format( "!!!reset_mon_items de linea {0} no DESPACHADORA", ssi_despachador ), 4 );
		}

		this->logger->Write( System::String::Format( "-----reset_mon_items de linea {0} ", ssi_despachador ), 1 );
		EventoCola^ evento;
		for ( int i = 0; i < this->lineas[index_linea]->monitorizaciones->Count; i ++ )
		{
			evento = gcnew EventoCola();
			evento->tipo = AC_MOD_MON_SSI;
			evento->contenido = System::String::Format( "A:{0}:{1}", ssi_despachador, this->lineas[index_linea]->monitorizaciones[i]->ssi_mon );
			
			this->cola->Enqueue(evento);

			if( this->permiso_continua )	// Solo continuaré consumiendo si hay permiso
				this->continua->Set();
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en modifica_item_mon", 4 );
	}
}

/*
	Rutinas de simulación de llamadas multiples
*/
void Consumidor::simu_calls ( System::Int32 no_lineas, System::Int32 ssi_destino_ini )
{
	try
	{
		System::Int32 ssi_dest = ssi_destino_ini;
		// pincho todas las lineas
		this->pincha_all( 1 );
		this->cuelga_all();

		for  ( System::Int32 index_linea = 0; index_linea < no_lineas; index_linea += 2 )
		{
			// llamada simplex con prioridad estandar (7)
			this->call_ssi(
				AC_CALL_DUPLEX,
				System::String::Format( "{0}:{1}:7", index_linea, this->lineas[index_linea + 1]->ssi_linea ),
				nullptr
				);
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en simu_calls", 4 );
	}
}

/*
	Se pinchan todas las líneas
*/
void Consumidor::pincha_all ( System::Int32 pincha )
{
	try
	{
		if ( pincha != 0 && pincha != 1 )
			throw gcnew System::Exception();

		for ( System::Int32 index_linea = 0; index_linea < this->lineas->Length; index_linea ++ )
		{
			this->pincha_linea( System::String::Format( "{0}:{1}", index_linea, pincha ) );
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en pincha_all", 4 );
	}
}

/*
	Se pinchan todas las líneas
*/
void Consumidor::cuelga_all (  )
{
	try
	{
		for ( System::Int32 index_linea = 0; index_linea < this->lineas->Length; index_linea ++ )
		{
			/*
				Si la linea tiene un recurso de audio ocupado se intenta liberar y se vuelvo a encolar el evento
				exactamente igual pero con tiempo de expiracion (para evitar bucle indefinido)
			*/
			if ( this->lineas[index_linea]->comunicacion->estado != CC_EMPTY )
			{
				if( this->lineas[index_linea]->comunicacion->estado != CC_HANGING_UP )
				{	// Cuelgo solo una vez
					this->cuelga_llamada( index_linea, 1 );
				}
			}
			else if ( this->lineas[index_linea]->mon_item_interviniendo != nullptr )
			{
				if ( ! this->lineas[index_linea]->mon_item_interviniendo->stop_intervention )
				{	// mon_intercept_disconnect solo una vez
					this->stop_escucha( index_linea );
				}
			}
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "problema en cuelga_all", 4 );
	}
}

