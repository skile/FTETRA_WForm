#include "Database.h"

Database::Database()
{
	this->destino = 0;
	this->ticks = 0;
	this->numInsert = 0;
	this->db_enabled = true;		// se pensó para permitir el uso de FR TETRA sin base de datos.
}

/*
void Database::testSQL()
{
	System::DateTime begin, afterConnect, afterInsert, end;
	long long ticks=0;
	long long meanODBC=0;
	long long meanSQL=0;
    OdbcConnection ^cnODBC = gcnew OdbcConnection();
	
	begin = System::DateTime::Now;

	cnODBC->ConnectionString =  "DSN=SDS_sim;Uid=sa;Pwd=sasql;";

	afterConnect= System::DateTime::Now;
	ticks = afterConnect.Ticks - begin.Ticks;
	System::Console::WriteLine(System::String::Concat("time elapsed connecting: ", System::Convert::ToString(ticks)));

	OdbcCommand^ insertCmdODBC = cnODBC-> CreateCommand();
	OdbcCommand^ deleteLastCmdODBC = gcnew OdbcCommand(System::String::Concat("DELETE FROM simpleTest WHERE id IN (SELECT TOP ", System::Convert::ToString(numRep), " id FROM simpleTest ORDER BY id DESC) "), cnODBC);

	System::String ^aux = gcnew System::String("TEST_");
	
	System::DateTime before, after;

	for(int i=0;i<numRep;i++)
	{
		before = System::DateTime::Now;
		cnODBC->Open();
		insertCmdODBC->CommandText = System::String::Concat("INSERT INTO simpleTest (name) VALUES ('", aux, System::Convert::ToString(i), "')");
		insertCmdODBC->ExecuteNonQuery();
		cnODBC->Close();
		after = System::DateTime::Now;
		ticks = after.Ticks - before.Ticks;
		meanODBC += ticks;
	}
	meanODBC = meanODBC/numRep;
	System::Console::WriteLine(System::String::Concat("mean time insert ODBC: ", System::Convert::ToString(meanODBC)));


	afterInsert = System::DateTime::Now;
	ticks = afterInsert.Ticks - afterConnect.Ticks;
	System::Console::WriteLine(System::String::Concat("time elapsed inserting: ", System::Convert::ToString(ticks)));

	cnODBC->Open();
	deleteLastCmdODBC->ExecuteNonQuery();

	end = System::DateTime::Now;
	ticks = end.Ticks - afterInsert.Ticks;
	System::Console::WriteLine(System::String::Concat("time elapsed deleting: ", System::Convert::ToString(ticks)));

    //Cerrar la conexión con la base de datos
    cnODBC->Close();

    //Pausa
	System::Console::ReadLine();
}
*/
bool Database::connect(System::String^ connectionString)
{
	try
	{
		this->connection = gcnew OdbcConnection(connectionString);
		if ( ! this->db_enabled )
		{
			this->logger->Write( "¡¡¡¡¡¡¡¡Base de datos deshabilitada!!!!", 5 );
			throw gcnew System::Exception();
		}
		this->connection->Open();
		this->connection->Close();
		this->sds_nextid = readTopSdsId();
		if ( this->sds_nextid == -1)
		{
			return false;
		}
		else
		{
			this->sds_nextid++;
			return true;
		}
	}
	catch( System::Exception^ e )
	{
		this->logger->Write( "Problema creando/abriendo conexión ODBC con la base de datos", 5 );
		return false;
	}
}

/*
	Se lee la información de las lineas tetra en la tabla radios.
*/
Generic::List<Linea_tetra^>^ Database::read_lines_info ( )
{
	Generic::List<Linea_tetra^>^ lineas_tetra = nullptr;

	System::String ^queryString;
	OdbcCommand^ command;
	OdbcDataReader^ reader;

	try
	{	
		Monitor::Enter( this->connection );

		if ( ! this->db_enabled )
		{
			this->logger->Write( "¡¡¡¡¡¡¡¡Base de datos deshabilitada!!!!", 5 );
			throw gcnew System::Exception();
		}

		//	Selecciono aquellas lineas radio tetra, (tetra_ssi distinto a NULL)
		queryString = "SELECT activa, tetra_ssi, tetra_despachador, tetra_puerto, canal_defecto FROM SICOM.dbo.radios where tetra_ssi > -1 ORDER BY numero ASC";

		this->connection->Open();
		command = gcnew OdbcCommand(queryString, this->connection);
		reader = command->ExecuteReader();

		lineas_tetra = gcnew Generic::List<Linea_tetra^>();
		Linea_tetra^ linea;
		// Call Read before accessing data
		while ( reader->Read() )
		{
			linea = gcnew Linea_tetra();
			linea->activa = reader->GetChar( 0 ) == DB_RADIO_ACTIVA;
			linea->ssi_linea = reader->GetInt32( 1 );
			linea->despachador = reader->GetBoolean( 2 );
			linea->puerto_asterisk = reader->GetInt32( 3 );
			linea->ssi_grupo_defecto = reader->GetInt32( 4 );

			lineas_tetra->Add( linea );
		}
		// Call Close when done reading.
		reader->Close();
		// si no existe ninguna entrada en la base de datos se devolverá -1
		if ( lineas_tetra->Count == 0 )
		{
			return nullptr;
		}
		else
		{
			return lineas_tetra;
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "Problema en read_lines_info!!!!!!!!!!!!!", 4 );
		return nullptr;
	}
	finally
	{
		this->connection->Close();
		Monitor::Exit( this->connection );
	}
}

/*
	Cuando se reciba o mande un sds status habrá que mirar en la tabla de mensajes predefinidos
	para ver si hay texto asociado al numero de estado
*/
bool Database::read_sds_status_text ( System::Int32 status_number, System::String^ %contenido )
{
	System::String ^queryString;
	OdbcCommand^ command;
	OdbcDataReader^ reader;

	try
	{	
		Monitor::Enter( this->connection );

		if ( ! this->db_enabled )
		{
			this->logger->Write( "¡¡¡¡¡¡¡¡Base de datos deshabilitada!!!!", 5 );
			throw gcnew System::Exception();
		}
		bool definido = false;
		queryString = System::String::Format( "SELECT contenido FROM SICOM.dbo.tetra_mensajes_predefinidos where numero={0}", status_number );

		this->connection->Open();
		command = gcnew OdbcCommand(queryString, this->connection);
		reader = command->ExecuteReader();

		// Call Read before accessing data
		while ( reader->Read() )
		{
			definido = true;
			contenido = reader->GetString( 0 );
		}
		// Call Close when done reading.
		reader->Close();

		return definido;
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "Problema en read_sds_status_text!!!!!!!!!!!!!", 4 );
		return false;
	}
	finally
	{
		this->connection->Close();
		Monitor::Exit( this->connection );
	}
}

/*
void Database::insert()
{
	this->before = System::DateTime::Now;
	System::String ^commandText = gcnew System::String(System::String::Concat("INSERT INTO SDS_sim (origen, destino, autor, despachador, tl, contenido, procesado, consumido) VALUES (2009, ", System::Convert::ToString(this->destino), ", 'PEPE', 0, '3452', 'HELLOHELLOHELLO', 1, 1)"));
	
	this->connection->Open();
	OdbcCommand^ insertODBC = gcnew OdbcCommand(commandText, this->connection);

	insertODBC->ExecuteNonQuery();

	this->connection->Close();
	if(numInsert>2)
	{
		this->after = System::DateTime::Now;
		this->ticks += this->after.Ticks - this->before.Ticks;
	}
	this->numInsert++;
	this->destino ++;
}

System::Int32 Database::read()
{
	System::String ^queryString = gcnew System::String("SELECT TOP 1 id FROM SDS_sim ORDER BY id DESC");

    OdbcCommand^ readLastId = gcnew OdbcCommand(queryString, this->connection);

   this->connection->Open();

    OdbcDataReader^ reader = readLastId->ExecuteReader();

    System::Int32 id;
	// Call Read before accessing data. 
    while (reader->Read())
    {
		id = reader->GetInt32(0);
    }

    // Call Close when done reading.
    reader->Close();

	connection->Close();

	return id;

}
*/

/*
	Función que lee la última entrada [id] en la tabla de mensajes tetra
*/
System::Int32 Database::readTopSdsId()
{
	try
	{

		System::String ^queryString = gcnew System::String("SELECT TOP 1 id FROM SICOM.dbo.tetra_mensajes ORDER BY id DESC");

		Monitor::Enter( this->connection );

		if ( ! this->db_enabled )
		{
			this->logger->Write( "¡¡¡¡¡¡¡¡Base de datos deshabilitada!!!!", 5 );
			throw gcnew System::Exception();
		}

		this->connection->Open();

		OdbcCommand^ readLastId = gcnew OdbcCommand(queryString, this->connection);


		OdbcDataReader^ reader = readLastId->ExecuteReader();

		System::Int32 id;
		// Call Read before accessing data. 
		while (reader->Read())
		{
			id = reader->GetInt32(0);
		}

		// Call Close when done reading.
		reader->Close();

		connection->Close();

		return id;
	}
	catch( System::Exception^ e )
	{
		this->logger->Write( "Problema leyendo SDS top", 4 );
		return -1;
	}
	finally
	{
		this->connection->Close();
		Monitor::Exit( this->connection );
	}
}

/*
	función para registrar un sds nuevo en la base de datos, ya sea recibido o enviado
*/
int Database::insert_SDS ( bool recibido, bool despachador, System::Int32 origen, System::Int32 destino,
							System::String^ autor, int handle, System::String^ tl, System::String^ contenido,
							bool procesado, bool consumido, System::DateTime^ _t_tetra, System::DateTime^ _t_procesado, System::DateTime^ _t_consumido )
{
	int idToReturn = this->sds_nextid;
	try
	{
		Monitor::Enter( this->connection );

		if ( ! this->db_enabled )
		{
			this->logger->Write( "¡¡¡¡¡¡¡¡Base de datos deshabilitada!!!!", 5 );
			throw gcnew System::Exception();
		}

		System::String^ t_tetra = ( _t_tetra == nullptr ) ? "1900-01-01 00:00:00" : _t_tetra->ToString( "yyyy-MM-dd HH:mm:ss" );
		System::String^ t_procesado = ( _t_procesado == nullptr ) ? "1900-01-01 00:00:00" : _t_procesado->ToString( "yyyy-MM-dd HH:mm:ss" );
		System::String^ t_consumido = ( _t_consumido == nullptr ) ? "1900-01-01 00:00:00" : _t_consumido->ToString( "yyyy-MM-dd HH:mm:ss" );

		array<System::String^>^ commandTextArray = gcnew array<System::String^>
		{	"INSERT INTO SICOM.dbo.tetra_mensajes (id, recibido, despachador, origen, destino, autor, handle, tl, contenido, procesado, consumido, t_tetra, t_procesado, t_consumido) VALUES (",
			System::Convert::ToString(this->sds_nextid), ",",
			System::Convert::ToString( recibido ? 1 : 0 ), ",",
			System::Convert::ToString( despachador ? 1 : 0 ), ",",
			System::Convert::ToString( origen ), ",",
			System::Convert::ToString( destino ), ",'",
			autor, "',",
			System::Convert::ToString( handle ), ",'",
			tl, "','",
			contenido, "',",
			System::Convert::ToString( procesado ? 1 : 0 ), ",",
			System::Convert::ToString( consumido ? 1 : 0 ), ",'",
			t_tetra, "','",
			t_procesado, "','",
			t_consumido, "')"
		};


		System::String ^commandText = System::String::Concat( commandTextArray );
		
		this->connection->Open();
		OdbcCommand^ insertODBC = gcnew OdbcCommand( commandText, this->connection );
		insertODBC->ExecuteNonQuery();

		this->sds_nextid++;

		return idToReturn;
	}
	catch(System::Exception^ e)
	{
		this->logger->Write( "error insertando SDS", 4 );
		return -1;
	}
	finally
	{
		this->connection->Close();
		Monitor::Exit( this->connection );
	}
}

bool Database::update_status ( int ssi, System::DateTime^ time, bool registrado, System::String^ zona )
{
	try
	{
		Monitor::Enter( this->connection );

		if ( ! this->db_enabled )
		{
			this->logger->Write( "¡¡¡¡¡¡¡¡Base de datos deshabilitada!!!!", 5 );
			throw gcnew System::Exception();
		}

		System::String^ t_cambio = time->ToString( "yyyy-MM-dd HH:mm:ss" );

		System::String ^commandText = System::String::Format(
			"UPDATE SICOM.dbo.tetra_terminales SET t_cambio='{0}', registrado={1}, zona='{2}' WHERE ssi={3}",
			t_cambio, registrado ? 1 : 0, zona, ssi);
		
		this->connection->Open();
		OdbcCommand^ insertODBC = gcnew OdbcCommand( commandText, this->connection );
		insertODBC->ExecuteNonQuery();
		return true;
	}
	catch(System::Exception^ e)
	{
		this->logger->Write( System::String::Format( "error actualizando estado de terminal {0} en la base de datos", ssi ), 4 );
		return false;
	}
	finally
	{
		this->connection->Close();
		Monitor::Exit( this->connection );
	}
}

/*
	devuelve todos los terminales que pertenecen a un determinado grupo
*/
ArrayList^ Database::read_all_pertenencias_grupo( System::Int32 g_ssi )
{
	ArrayList^ terminales = gcnew ArrayList();

	System::String ^queryString;
	OdbcCommand^ command;
	OdbcDataReader^ reader;
	try
	{
		queryString = System::String::Format( "SELECT t_ssi FROM SICOM.dbo.tetra_pertenencias WHERE g_ssi={0}", g_ssi );

		Monitor::Enter( this->connection );

		if ( ! this->db_enabled )
		{
			this->logger->Write( "¡¡¡¡¡¡¡¡Base de datos deshabilitada!!!!", 5 );
			throw gcnew System::Exception();
		}

		this->connection->Open();
		command = gcnew OdbcCommand(queryString, this->connection);
		reader = command->ExecuteReader();
		// Call Read before accessing data
		while ( reader->Read() )
		{
			terminales->Add( reader->GetInt32( 0 ) );
		}
		// Call Close when done reading.
		reader->Close();
		// si no existe ninguna entrada en la base de datos se devolverá -1
		return terminales;
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( System::String::Format( "Problema leyendo terminales pertenecientes al grupo {0}!!!!!!!!!!!!!", g_ssi ), 4 );
		return nullptr;
	}
	finally
	{
		this->connection->Close();
		Monitor::Exit( this->connection );
	}
}

/*
	Lee el estado de una pareja grupo-individuo en pertenencias. Con esta informacion sabremos si hay que hacer insert, update o delete
	return: -1	no hay entrada, -2 ha habido un problema, 0-1-2 segun el estado de la pertenencia si había entrada
*/
Pertenencia^ Database::read_pertenencia ( int group_ssi, int individual_ssi )
{
	System::String ^queryString;
	OdbcCommand^ command;
	OdbcDataReader^ reader;
	Pertenencia^ pertenencia = nullptr;
	try
	{
		queryString = System::String::Format( "SELECT g_ssi, t_ssi, estado, t_cambio, intentos FROM SICOM.dbo.tetra_pertenencias WHERE g_ssi={0} AND t_ssi={1}", group_ssi, individual_ssi );

		Monitor::Enter( this->connection );

		if ( ! this->db_enabled )
		{
			this->logger->Write( "¡¡¡¡¡¡¡¡Base de datos deshabilitada!!!!", 5 );
			throw gcnew System::Exception();
		}

		this->connection->Open();
		command = gcnew OdbcCommand(queryString, this->connection);
		reader = command->ExecuteReader();

		// Call Read before accessing data
		while (reader->Read())
		{
			pertenencia = gcnew Pertenencia( reader->GetInt32(0), reader->GetInt32(1), reader->GetInt32(2), reader->GetDateTime(3), reader->GetInt32(4) );
		}
		if ( pertenencia == nullptr )
		{
			pertenencia = gcnew Pertenencia ( group_ssi, individual_ssi, PERTENENCIA_NO_EXISTE, nullptr, -1 );
		}
		// Call Close when done reading.
		reader->Close();
		// si no existe ninguna entrada en la base de datos se devolverá -1
		return pertenencia;
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "Problema leyendo estado de pertenencias!!!!!!!!!!!!!", 4 );
		return gcnew Pertenencia ( group_ssi, individual_ssi, PERTENENCIA_ERROR, nullptr, -1 );
	}
	finally
	{
		this->connection->Close();
		Monitor::Exit( this->connection );
	}
}

bool Database::update_pertenencias ( int group_ssi, int individual_ssi, int estado, int operacion )
{
	System::String ^queryString;
	System::DateTime^ now;

	try
	{
		Monitor::Enter( this->connection );

		if ( ! this->db_enabled )
		{
			this->logger->Write( "¡¡¡¡¡¡¡¡Base de datos deshabilitada!!!!", 5 );
			throw gcnew System::Exception();
		}

		now = System::DateTime::Now;
		System::String^ t_cambio = now->ToString( "yyyy-MM-dd HH:mm:ss" );
		switch ( operacion )
		{
			case DB_INSERT:
				queryString = System::String::Format( "INSERT INTO SICOM.dbo.tetra_pertenencias (g_ssi, t_ssi, estado, t_cambio, intentos, attached) VALUES ({0}, {1}, {2}, '{3}', 0, {4})",
					group_ssi, individual_ssi, estado, t_cambio, PER_ATTACH_UNKNOWN );
				break;
			case DB_UPDATE:		// Se cambio de estado y se reinician los intentos
				queryString = System::String::Format( "UPDATE SICOM.dbo.tetra_pertenencias SET estado={2}, t_cambio='{3}', intentos=0 WHERE g_ssi={0} AND t_ssi={1}",
					group_ssi, individual_ssi, estado, t_cambio );
				break;
			case DB_DELETE:
				queryString = System::String::Format( "DELETE FROM SICOM.dbo.tetra_pertenencias WHERE g_ssi={0} AND t_ssi={1}",
					group_ssi, individual_ssi );
				break;
			default:
				this->logger->Write( "numero de operacion incorrecto en update_pertenencias", 2 );
				break;
		}
		
		this->connection->Open();
		OdbcCommand^ sql_query = gcnew OdbcCommand( queryString, this->connection );
		sql_query->ExecuteNonQuery();
		return true;
	}
	catch(System::Exception^ e)
	{
		this->logger->Write( System::String::Format( "error actualizando la tabla de pertenencias. grupo: {0} individuo: {1} estado: {2} ", group_ssi, individual_ssi, estado ), 4 );
		return false;
	}
	finally
	{
		this->connection->Close();
		Monitor::Exit( this->connection );;
	}
}

bool Database::clear_pertenencias ( int individual_ssi )
{
	System::String ^queryString;

	try
	{
		Monitor::Enter( this->connection );

		if ( ! this->db_enabled )
		{
			this->logger->Write( "¡¡¡¡¡¡¡¡Base de datos deshabilitada!!!!", 5 );
			throw gcnew System::Exception();
		}
		queryString = System::String::Format( "DELETE FROM SICOM.dbo.tetra_pertenencias WHERE t_ssi={0}",
			individual_ssi );

		this->connection->Open();
		OdbcCommand^ sql_query = gcnew OdbcCommand( queryString, this->connection );
		sql_query->ExecuteNonQuery();
		return true;
	}
	catch(System::Exception^ e)
	{
		this->logger->Write( System::String::Format( "error borrando las pertencias de la linea {0}", individual_ssi ), 4 );
		return false;
	}
	finally
	{
		this->connection->Close();
		Monitor::Exit( this->connection );;
	}
}

void Database::clear_pertenencias_all ( )
{
	System::String ^queryString;
	OdbcCommand^ sql_query;

	try
	{
		Monitor::Enter( this->connection );

		if ( ! this->db_enabled )
		{
			this->logger->Write( "¡¡¡¡¡¡¡¡Base de datos deshabilitada!!!!", 5 );
			throw gcnew System::Exception();
		}
		queryString = "DELETE FROM SICOM.dbo.tetra_pertenencias";

		this->connection->Open();
		sql_query = gcnew OdbcCommand( queryString, this->connection );
		sql_query->ExecuteNonQuery();
	}
	catch(System::Exception^ e)
	{
		this->logger->Write( "error borrando TODAS las pertencia", 4 );
	}
	finally
	{
		this->connection->Close();
		Monitor::Exit( this->connection );;
	}
}

/*
Actualiza el estado attached de una pertenencia.
	PER_ATTACH_UNKNOWN		no importa su estado attached, el estado attached solo tiene sentido para las lineas activas!!
	PER_ATTACHED			
	PER_ATTACH_PENDING		se intentará attach en la rutina periódica check_attached
	PER_DETACH_PENDING		se intentará detach en la rutina periódica check_attached
	PER_DETACHED		
Return, true en el caso de que la pertenencia exista, false si no existe
*/
bool Database::update_attached ( int group_ssi, int individual_ssi, int attached )
{
	System::String ^queryString;
	OdbcCommand^ sql_query;
	OdbcDataReader^ reader;

	bool existe_pertenencia;

	try
	{
		Monitor::Enter( this->connection );

		if ( ! this->db_enabled )
		{
			this->logger->Write( "¡¡¡¡¡¡¡¡Base de datos deshabilitada!!!!", 5 );
			throw gcnew System::Exception();
		}

		existe_pertenencia = false;

		this->connection->Open();
		queryString = System::String::Format( "SELECT g_ssi, t_ssi, estado FROM SICOM.dbo.tetra_pertenencias WHERE g_ssi={0} AND t_ssi={1}",
							group_ssi, individual_ssi );

		sql_query = gcnew OdbcCommand( queryString, this->connection );
		reader = sql_query->ExecuteReader();

		// Call Read before accessing data
		while (reader->Read())
		{
			existe_pertenencia = true;
		}
		// Call Close when done reading.
		reader->Close();

		if ( existe_pertenencia )
		{
			queryString = System::String::Format( "UPDATE SICOM.dbo.tetra_pertenencias SET attached={2} WHERE g_ssi={0} AND t_ssi={1}",
								group_ssi, individual_ssi, attached );
			sql_query = gcnew OdbcCommand( queryString, this->connection );
			sql_query->ExecuteNonQuery();
		}

		return existe_pertenencia;
	}
	catch(System::Exception^ e)
	{
		this->logger->Write( System::String::Format( "error actualizando estado ATTACHED grupo: {0} individuo: {1} attached: {2} ", group_ssi, individual_ssi, attached ), 4 );
		return false;
	}
	finally
	{
		this->connection->Close();
		Monitor::Exit( this->connection );;
	}
}

/*
	Cada vez que se reinicie la aplicación o se pierda la conexion con el servidor acapi los servicios se reiniciarán.
	Los ssi perderan su estado attached a cualquier grupo, por tanto habrá que volver hacer attach. Como cada linea solo puede 
	aceptar llamadas de un grupo a la vez, se hará attach solo a cada grupo asociado de cada linea
*/
void Database::clear_attached ( )
{
	System::String ^queryString;
	OdbcCommand^ sql_query;
	System::DateTime ^start, ^end;
	System::Int32 duration_ms;

	try
	{
		Monitor::Enter( this->connection );
		start = System::DateTime::Now;

		if ( ! this->db_enabled )
		{
			this->logger->Write( "¡¡¡¡¡¡¡¡Base de datos deshabilitada!!!!", 5 );
			throw gcnew System::Exception();
		}

		this->connection->Open();

		queryString = System::String::Format( "UPDATE SICOM.dbo.tetra_pertenencias SET attached={0}", PER_ATTACH_UNKNOWN );
		sql_query = gcnew OdbcCommand( queryString, this->connection );
		sql_query->ExecuteNonQuery();

		end = System::DateTime::Now;
		duration_ms = ( end->Ticks - start->Ticks ) / 10000;
		this->logger->Write( System::String::Format( "CLEAR ATTACHED en {0} ms", duration_ms ), -1 );
	}
	catch(System::Exception^ e)
	{
		this->logger->Write( "error clearing ATTACHED", 4 );
	}
	finally
	{
		this->connection->Close();
		Monitor::Exit( this->connection );;
	}
}

/*
	Cada vez que se inicie o se caiga la monitorización de registro de usuarios habrá que resetear el estdo
	Esto es debido a que es más óptimo usar un 'initial briefing' únicamente de los usuarios que estén registrados
*/
void Database::reset_register_status ( )
{
	System::String ^queryString;
	OdbcCommand^ sql_query;
	System::DateTime ^start, ^end;
	System::Int32 duration_ms;

	try
	{
		Monitor::Enter( this->connection );
		start = System::DateTime::Now;

		if ( ! this->db_enabled )
		{
			this->logger->Write( "¡¡¡¡¡¡¡¡Base de datos deshabilitada!!!!", 5 );
			throw gcnew System::Exception();
		}

		this->connection->Open();

		queryString = "UPDATE SICOM.dbo.tetra_terminales SET registrado=0, zona='-'";
		sql_query = gcnew OdbcCommand( queryString, this->connection );
		sql_query->ExecuteNonQuery();

		end = System::DateTime::Now;
		duration_ms = ( end->Ticks - start->Ticks ) / 10000;
		this->logger->Write( System::String::Format( "RESET_REGISTER_STATUS en {0} ms", duration_ms ), -1 );
	}
	catch(System::Exception^ e)
	{
		this->logger->Write( "error RESET_REGISTER_STATUS", 4 );
	}
	finally
	{
		this->connection->Close();
		Monitor::Exit( this->connection );;
	}
}

/*
	Se recorrera la tabla de pertenencias, aquellas que estén pendientes de ser borradas o añadidas se intentará de nuevo
*/
void Database::check_pertenencias ( Consumidor^ consumidor, TetraInterface* tetra )
{
	System::String ^queryString;
	OdbcCommand^ command;
	OdbcDataReader^ reader;
	System::Int32 g_ssi;
	System::Int32 t_ssi;
	System::Int32 estado;
	System::Int32 intentos; 
	System::DateTime^ before;
	System::DateTime^ after;

	Generic::List<Pertenencia^>^ pertenencias_pendientes;

	System::Int32 app_handle;
	bool ss_status;

	bool add;
	bool estatico;
	bool operacion_ok;

	bool avisa_core;

	try
	{
		Monitor::Enter( this->connection );

		before = System::DateTime::Now;
		pertenencias_pendientes = gcnew Generic::List<Pertenencia^>();

		if ( ! this->db_enabled )
		{
			this->logger->Write( "¡¡¡¡¡¡¡¡Base de datos deshabilitada!!!!", 5 );
			throw gcnew System::Exception();
		}

		app_handle = consumidor->lineas[consumidor->index_first_disp]->ss->handle;
		ss_status = consumidor->lineas[consumidor->index_first_disp]->ss->estado_ok;
		if ( ! ss_status )
		{
			this->logger->Write( "¡¡¡¡¡¡¡¡Check pertencias, SS no OK!!!!", 4 );
			throw gcnew System::Exception();
		}
		queryString = System::String::Format( "SELECT g_ssi, t_ssi, estado, intentos FROM SICOM.dbo.tetra_pertenencias WHERE estado={0} OR estado={1} OR estado={2}",
			PERTENENCIA_PENDIENTE_AÑADIR, PERTENENCIA_PENDIENTE_BORRAR, PERTENENCIA_BORRAR_FALLO );

		this->connection->Open();
		command = gcnew OdbcCommand(queryString, this->connection);
		reader = command->ExecuteReader();
		// Call Read before accessing data
		while ( reader->Read() )
		{
			g_ssi = reader->GetInt32( 0 );
			t_ssi = reader->GetInt32( 1 );
			estado = reader->GetInt32( 2 );
			intentos = reader->GetInt32( 3 );
			// acumulo todas las pertenencias pendientes
			pertenencias_pendientes->Add( gcnew Pertenencia( g_ssi, t_ssi, estado, nullptr, intentos ) );
		}
		// Call Close when done reading.
		reader->Close();

		/*
			Para todas las pertenencias pendientes de añadir, se actualiza el numero de intentos, se da la orden al tetrainterface
				- Se actualiza el estado a PERTENENCIA_AÑADIR_FALLO si se han superado el numero de intentos establecidos
				- Se actualiza el estado e PERTENENCIA_BORRAR_FALLO si se han superado el numero de intentos establecidos
					Una pertenencia en este estado se intentará borrar con una cadencia menor de lo normal, establecida en "NO_ITER_CHECK_PER_BORRAR_FALLO"
		*/
		for each ( Pertenencia^ pertenencia in pertenencias_pendientes )
		{
			try
			{
				g_ssi = pertenencia->g_ssi;
				t_ssi = pertenencia->t_ssi;
				estado = pertenencia->estado;
				intentos = pertenencia->intentos + 1;

				// Se pasa a "PERTENENCIA_AÑADIR_FALLO" si se ha intentado añadir al usuario demasiadas veces.
				// No se volverá a intentar añadir al usuario
				if ( ( consumidor->dgna_num_max_retries >= 0 )
						&& ( estado == PERTENENCIA_PENDIENTE_AÑADIR )
						&& ( intentos > consumidor->dgna_num_max_retries ) )
				{
					estado = PERTENENCIA_AÑADIR_FALLO;
					operacion_ok = true;
					avisa_core = true;
				}
				// Se pasa a "PERTENENCIA_BORRAR_FALLO" si se ha intentado borrar al usuario demasiadas veces.
				// Se seguirá intentando eliminar al usuario del grupo pero con menos frecuencia ( cada NO_ITER_CHECK_PER_BORRAR_FALLO )
				if ( ( consumidor->dgna_num_max_retries >= 0 )
						&& ( estado == PERTENENCIA_PENDIENTE_BORRAR )
						&& ( intentos > consumidor->dgna_num_max_retries ) )
				{
					estado = PERTENENCIA_BORRAR_FALLO;
					operacion_ok = true;
					avisa_core = true;
				}

				if ( estado == PERTENENCIA_PENDIENTE_AÑADIR )
				{
					estatico = false;			// habrá que añadir lógica en caso de grupos estáticos
					add = true;
					operacion_ok = tetra->modifica_pertenencias( app_handle, estatico, add, g_ssi, t_ssi );
				}

				else if (
					estado == PERTENENCIA_PENDIENTE_BORRAR  ||
					( estado == PERTENENCIA_BORRAR_FALLO && ( consumidor->num_timer_iter % NO_ITER_CHECK_PER_BORRAR_FALLO ) == 0 )
						)
				{
					estatico = false;
					add = false;
					operacion_ok = tetra->modifica_pertenencias( app_handle, estatico, add, g_ssi, t_ssi );
				}

				if ( operacion_ok )		// si se ha realizado algún cambio satisfactoriamente ( cambios de estado o intentos satisfactorios ), si no se ha conseguido mandar la orden tetra no se actualiza
				{
					queryString = System::String::Format( "UPDATE SICOM.dbo.tetra_pertenencias SET estado={0}, intentos={1}  where g_ssi={2} AND t_ssi={3}",
							estado, intentos, g_ssi, t_ssi );
					command = gcnew OdbcCommand( queryString, this->connection );
					command->ExecuteNonQuery();
				}
				if ( avisa_core )		// solo avisaré de cambios de estado cuando estos se produzcan => pertenencia cambia de "pendiente" a "fallo"
				{
					consumidor->logica_aviso_mod_dgna( g_ssi );
				}
			}
			catch ( System::Exception^ e )
			{
				this->logger->Write( System::String::Format("Problema actualizando pertenencia pendiente, g_ssi:{0}, t_ssi:{1}, intentos:{2}",
					pertenencia->g_ssi, pertenencia->t_ssi, pertenencia->intentos ), 4 );
			}
		}

		/*
		// Se habrán reintentado los casos pendientes, incrementar el numero de intentos y poner a error aquellas entradas que superen el numero max de reintentos
		queryString = System::String::Format( "UPDATE SICOM.dbo.tetra_pertenencias SET intentos = (intentos + 1) where estado={0} OR estado={1}",
							PERTENENCIA_PENDIENTE_AÑADIR, PERTENENCIA_PENDIENTE_BORRAR );
		command = gcnew OdbcCommand( queryString, this->connection );
		command->ExecuteNonQuery();

		if ( consumidor->dgna_num_max_retries >= 0 )
		{
			queryString = System::String::Format( "UPDATE SICOM.dbo.tetra_pertenencias SET estado = {0} where (estado={1} AND intentos>{2})",
								PERTENENCIA_AÑADIR_FALLO, PERTENENCIA_PENDIENTE_AÑADIR, consumidor->dgna_num_max_retries );
			command = gcnew OdbcCommand( queryString, this->connection );
			command->ExecuteNonQuery();
		}
		*/
		after = System::DateTime::Now;
		System::Int32 duration_ms = ( after->Ticks - before->Ticks ) / 10000;
		this->logger->Write( System::String::Format( "check_pertenencias en {0} ms", duration_ms ), -1 );
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "Problema chequeando pertenencias!!!!!!!!!!!!!", 4 );
	}
	finally
	{
		this->connection->Close();
		Monitor::Exit( this->connection );
	}
}

/*
	Se recorrera la tabla de pertenencias, aquellas que estén pendientes de ser borradas o añadidas se intentará de nuevo
*/
void Database::check_attached ( Consumidor^ consumidor, TetraInterface* tetra )
{
	System::String ^queryString;
	OdbcCommand^ command;
	OdbcDataReader^ reader;
	System::Int32 g_ssi;
	System::Int32 t_ssi;
	System::Int32 attached; 
	System::DateTime^ before;
	System::DateTime^ after;

	System::Int32 index_linea;
	System::Int32 app_handle;
	bool ss_status;

	bool add;
	bool estatico;
	bool operacion_ok;

	try
	{
		Monitor::Enter( this->connection );

		System::DateTime ^before, ^after, ^t_before, ^t_after;
		System::Int64 duration_ms;

		if ( ! this->db_enabled )
		{
			this->logger->Write( "¡¡¡¡¡¡¡¡Base de datos deshabilitada!!!!", 5 );
			throw gcnew System::Exception();
		}

		before = System::DateTime::Now;

		queryString = System::String::Format( "SELECT g_ssi, t_ssi, attached FROM SICOM.dbo.tetra_pertenencias WHERE estado={0} AND (attached={1} OR attached={2})",
			PERTENENCIA_OK, PER_ATTACH_PENDING, PER_DETACH_PENDING );

		this->connection->Open();
		command = gcnew OdbcCommand(queryString, this->connection);
		reader = command->ExecuteReader();

		after = System::DateTime::Now;
		duration_ms = ( after->Ticks - before->Ticks ) / 10000;
		this->logger->Write( System::String::Format( "check_attached [ExecuteReader] en {0} ms", duration_ms ), -5 );
		before = System::DateTime::Now;

		// Call Read before accessing data
		estatico = true;				// Se hará attach a los grupos desde los ssi applicacion
		while ( reader->Read() )
		{
			try
			{

				after = System::DateTime::Now;
				duration_ms = ( after->Ticks - before->Ticks ) / 10000;
				this->logger->Write( System::String::Format( "check_attached [Read pertenencia] en {0} ms", duration_ms ), -5 );
				before = System::DateTime::Now;

				g_ssi = reader->GetInt32( 0 );
				t_ssi = reader->GetInt32( 1 );
				attached = reader->GetInt32( 2 );

				index_linea = consumidor->find_index_by_ssi( t_ssi );
				app_handle = consumidor->lineas[index_linea]->ss->handle;
				ss_status = consumidor->lineas[index_linea]->ss->estado_ok;

				// Si la linea no esta activa o no tiene el servicio ss abierto no se procederás a intentar attach/detach
				if ( ( ! ss_status ) || ( ! consumidor->lineas[index_linea]->activa ) )
				{
					this->logger->Write( System::String::Format( "¡¡¡¡¡¡¡¡Check ATTACHED, SS no OK en linea {0} !!!!", index_linea ), 4 );
					throw gcnew System::Exception();
				}

				if ( attached == PER_ATTACH_PENDING )
				{
					t_before = System::DateTime::Now;

					add = true;
					operacion_ok = tetra->modifica_pertenencias( app_handle, estatico, add, g_ssi, t_ssi );

					t_after = System::DateTime::Now;
					duration_ms = ( after->Ticks - before->Ticks ) / 10000;
					this->logger->Write( System::String::Format( "check_attached [tetra attach] en {0} ms", duration_ms ), -5 );
				}
				else if ( attached == PER_DETACH_PENDING )
				{
					t_before = System::DateTime::Now;

					add = false;
					operacion_ok = tetra->modifica_pertenencias( app_handle, estatico, add, g_ssi, t_ssi );

					t_after = System::DateTime::Now;
					duration_ms = ( after->Ticks - before->Ticks ) / 10000;
					this->logger->Write( System::String::Format( "check_attached [tetra detach] en {0} ms", duration_ms ), -5 );
				}
			}
			catch ( System::Exception^ e )
			{
				this->logger->Write( System::String::Format("Problema generando de/attach pendiente, g_ssi:{0}, t_ssi:{1}, attached:{2}",
					g_ssi, t_ssi, attached ), 4 );
			}
		}
		// Call Close when done reading.
		reader->Close();
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "Problema chequeando attached!!!!!!!!!!!!!", 4 );
	}
	finally
	{
		this->connection->Close();
		Monitor::Exit( this->connection );
	}
}

/*
	Se carga en memoria todos los subscribers de la base de datos, ya sean grupos o terminales => subscriber_db_list
	Cada subscriber del archivo xml está contenido en "subscriber_updated_list"
	Se compara cada subscriber del xml con cada subscriber de la base de datos. Se pueden dar los siguientes casos:

		subscriber existe y es igual en xml y db				ninguna accion en la base de datos
		subscriber existe en xml y db pero existen diferencias	se borra de la db, se inserta el nuevo subscriber del xml
		subscriber existe en xml pero no en db					insertar subscriber en db
		subscriber existe en db pero no en xml					se borra de la base de datos

	Devuelve false en caso de fallo
*/
bool Database::update_subscribers ( array<Subscriber^>^ subscriber_updated_list )
{
	System::String ^queryString;
	OdbcCommand^ sql_query;
	System::DateTime^ now;
	Generic::List<Subscriber^>^ subscriber_db_list;
	Subscriber^ subscriber_updated;
	Subscriber^ subscriber_db;
	OdbcDataReader^ reader;
	bool found;
	System::DateTime ^before, ^after;
	System::DateTime ^start, ^end;
	System::Int32 duration_ms;
	System::Int32 num_operaciones;
	try
	{
		Monitor::Enter( this->connection );
		this->logger->Write( "¡¡¡¡¡¡¡¡Comienza update_subscribers!!!!", 1 );
		start = System::DateTime::Now;
		before = System::DateTime::Now;
		num_operaciones = 0;

		if ( ! this->db_enabled )
		{
			this->logger->Write( "¡¡¡¡¡¡¡¡Base de datos deshabilitada!!!!", 5 );
			throw gcnew System::Exception();
		}
		this->connection->Open();
		subscriber_db_list = gcnew Generic::List<Subscriber^>();
		// -------------construir lista de subscriptores en la base de datos 
		// ----------------------- TERMINALES
		queryString = "SELECT ssi,nombre,tipo FROM SICOM.dbo.tetra_terminales";
		sql_query = gcnew OdbcCommand( queryString, this->connection );
		reader = sql_query->ExecuteReader();
		num_operaciones++;

		// Call Read before accessing data
		while ( reader->Read() )
		{
			subscriber_db = gcnew Subscriber( false, reader->GetInt32(0), reader->GetString(1), reader->GetChar(2) );
			subscriber_db_list->Add( subscriber_db );
		}
		// Call Close when done reading.
		reader->Close();
		//----------------------- GRUPOS
		queryString = "SELECT ssi,nombre,tipo FROM SICOM.dbo.tetra_grupos";
		sql_query = gcnew OdbcCommand( queryString, this->connection );
		reader = sql_query->ExecuteReader();
		num_operaciones++;

		// Call Read before accessing data
		while ( reader->Read() )
		{
			subscriber_db = gcnew Subscriber( true, reader->GetInt32(0), reader->GetString(1), reader->GetChar(2) );
			subscriber_db_list->Add( subscriber_db );
		}
		// Call Close when done reading.
		reader->Close();

		after = System::DateTime::Now;
		duration_ms = ( after->Ticks - before->Ticks ) / 10000;
		this->logger->Write( System::String::Format( "UPDATE_SUBSCRIBERS leer datos en {0} ms, operaciones realizadas: {1}", duration_ms, num_operaciones ), 1 );
		before = System::DateTime::Now;
		//--------------------------------------------------------------------------------------------
		//Para cada subscriber de "subscriber_updated_list" (xml) hay que ver si hay algun subscriber con el mismo
		//ssi y actuar en consecuencia. Bucle con complejidad O(n*n)!!!!

		//subscriber_db_list->TrimExcess();	//ahorro memoria

		for each ( subscriber_updated in subscriber_updated_list )
		{
			try
			{
				found = false;
				for each ( subscriber_db in subscriber_db_list )
				{
					// si se trata del mismo ssi
					if ( subscriber_db->ssi == subscriber_updated->ssi )
					{
						found = true;		// Se ha encontrado el ssi del xml en la base de datos
						// -----------------------------------------------
						// el subscriber del xml es distinto al de la BD!!	=> Borrar version db (grupos o terminales?) añadir versión xml
						if ( ( subscriber_db->nombre !=  subscriber_updated->nombre ) ||
							 ( subscriber_db->tipo !=  subscriber_updated->tipo ) )
						{
							this->logger->Write( System::String::Format( "diferencias en el ssi {0} xml-db", subscriber_db->ssi ), -1 );
							// habrá que borrar de la base de datos de grupos o terminales
							if ( subscriber_db->grupo )
							{
								queryString = System::String::Format( "DELETE FROM SICOM.dbo.tetra_grupos WHERE ssi={0}", subscriber_db->ssi );
							}
							else
							{
								queryString = System::String::Format( "DELETE FROM SICOM.dbo.tetra_terminales WHERE ssi={0}", subscriber_db->ssi );
							}
							sql_query = gcnew OdbcCommand( queryString, this->connection );
							sql_query->ExecuteNonQuery();
							num_operaciones++;

							// habrá que insertar a la base de datos de grupos o terminales
							if ( subscriber_updated->grupo )
							{
								queryString = System::String::Format( " INSERT INTO SICOM.dbo.tetra_grupos (ssi, nombre, tipo)"
																	" VALUES ({0},'{1}','{2}')",
																	subscriber_updated->ssi, subscriber_updated->nombre, subscriber_updated->tipo );
							}
							else
							{
								queryString = System::String::Format( " INSERT INTO SICOM.dbo.tetra_terminales (ssi, nombre, tipo, t_cambio, registrado, zona)"
																	" VALUES ({0},'{1}','{2}',GETDATE(),'false','UNKNOWN')",
																	subscriber_updated->ssi, subscriber_updated->nombre, subscriber_updated->tipo );
							}
							sql_query = gcnew OdbcCommand( queryString, this->connection );
							sql_query->ExecuteNonQuery();
							num_operaciones++;

							subscriber_db->explorado = true;		// El subscriber es un NUEVO, no deberá ser borrado al final
							break;			// me salgo del bucle
						}
						else
						{		// Existe una entrada igual en el xml y en la base de datos, no es necesaria ninguna accion en la base de datos
							subscriber_db->explorado = true;
							break;			// me salgo del bucle
						}
					}
				}
				if ( ! found )		// Se ha recorrido los ssis de la base de datos sin encontrar un match
				{					// Habrá que insertar en la base de datos
					this->logger->Write( System::String::Format( "no se encontro el ssi {0} del xml", subscriber_updated->ssi ), -1 );
					if ( subscriber_updated->grupo )
					{
						queryString = System::String::Format( " INSERT INTO SICOM.dbo.tetra_grupos (ssi, nombre, tipo)"
															" VALUES ({0},'{1}','{2}')",
															subscriber_updated->ssi, subscriber_updated->nombre, subscriber_updated->tipo );
					}
					else
					{
						queryString = System::String::Format( " INSERT INTO SICOM.dbo.tetra_terminales (ssi, nombre, tipo, t_cambio, registrado, zona)"
															" VALUES ({0},'{1}','{2}',GETDATE(),'false','UNKNOWN')",
															subscriber_updated->ssi, subscriber_updated->nombre, subscriber_updated->tipo );
					}
					sql_query = gcnew OdbcCommand( queryString, this->connection );
					sql_query->ExecuteNonQuery();
					num_operaciones++;
				}
			}
			catch ( System::Exception^ e )
			{
				this->logger->Write( System::String::Format( "Problema actualizando el ssi {0} del archivo xml subscribers", subscriber_updated->ssi ), 4 );
			}
		}
		// Se ha terminado de analizar las dos listas. Se analizará la lista de subscriptores en la db para ver si hay alguno no explorado
		//		=> el subscriptor ya no existe en el NMS y habrá que borrarlo

		after = System::DateTime::Now;
		duration_ms = ( after->Ticks - before->Ticks ) / 10000;
		this->logger->Write( System::String::Format( "UPDATE_SUBSCRIBERS recorrer estructura en {0} ms, operaciones realizadas: {1}", duration_ms, num_operaciones ), 1 );
		before = System::DateTime::Now;

		for each ( subscriber_db in subscriber_db_list )
		{
			try
			{
				if ( ! subscriber_db->explorado )
				{
					this->logger->Write( System::String::Format( "El ssi {0} de la db no está en el xml!!", subscriber_db->ssi ), -2 );
					// habrá que borrar de la base de datos de grupos o terminales
					if ( subscriber_db->grupo )
					{
						queryString = System::String::Format( "DELETE FROM SICOM.dbo.tetra_grupos WHERE ssi={0}", subscriber_db->ssi );
					}
					else
					{
						queryString = System::String::Format( "DELETE FROM SICOM.dbo.tetra_terminales WHERE ssi={0}", subscriber_db->ssi );
					}
					sql_query = gcnew OdbcCommand( queryString, this->connection );
					sql_query->ExecuteNonQuery();
					num_operaciones++;
				}
			}
			catch ( System::Exception^ e )
			{
				this->logger->Write( System::String::Format( "Problema borrando ssi {0} de la base de datos", subscriber_db->ssi ), 4 );
			}
		}
		after = System::DateTime::Now;
		duration_ms = ( after->Ticks - before->Ticks ) / 10000;
		this->logger->Write( System::String::Format( "UPDATE_SUBSCRIBERS eliminar de db en {0} ms, operaciones realizadas: {1}", duration_ms, num_operaciones ), 2 );

		end = System::DateTime::Now;
		duration_ms = ( end->Ticks - start->Ticks ) / 10000;

		this->logger->Write( System::String::Format( "COMPLETADO update_subscribers en {0} ms, operaciones realizadas: {1}", duration_ms, num_operaciones ), 2 );

		return true;
	}
	catch(System::Exception^ e)
	{
		this->logger->Write( "error actualizando subscribers", 4 );
		return false;
	}
	finally
	{
		this->connection->Close();
		Monitor::Exit( this->connection );;
	}
}

/*
	Se carga en memoria todos las pertenencias de la base de datos => pertenencias_db
	Cada pertenencia del archivo xml está contenida en "pertenencias_estaticas_xml"
	Se compara cada pertenencia del xml con cada pertenencia de la base de datos. Se pueden dar los siguientes casos:

		pertenencia existe y es igual en xml y db				ninguna accion en la base de datos
		pertenencia existe en xml pero no en db					insertar pertenencia en db
		pertenencia existe en db pero no en xml					se borra de la base de datos
		no pueden existir pertenencias distintas, se compara unicamente la pareja (g_ssi, t_ssi)

*MODIFICACION:
	Los grupos extensibles crean una nueva problemática al tener pertenencias estáticas y dinámicas a la vez.
		Si una pertenencia estática se intenta modificar por DGNA se ignora. Pero que pasa si existe una pertenencia dinámica,
		y tras una actualización del NMS se crea una pertenencia estática del mismo ssi al mismo grupo?

		=> habrá que cargar todas las pertenencias de la base de datos distinguiendo entre pertenencias estaticas y no estáticas
		para actualizar únicamente aquellas que aparecen dinámicas en la base de datos y estáticas en el xml!!
			El estado anterior de la pertenencia dinámica se ignora, dará igual que este ok, pendiente, error,...

	devuelve false en caso de fallo
*/
bool Database::update_pertenencias_estaticas ( array<Pertenencia^>^ pertenencias_estaticas_xml )
{
	System::String^ queryString;
	OdbcCommand^ sql_query;
	System::DateTime^ now;
	Generic::List<Pertenencia^>^ pertenencias_db;
	Pertenencia^ pertenencia_xml;
	Pertenencia^ pertenencia_db;
	OdbcDataReader^ reader;
	bool found;
	System::DateTime ^before, ^after;
	System::DateTime ^start, ^end;
	System::Int32 duration_ms, num_operaciones;

	try
	{
		Monitor::Enter( this->connection );
		this->logger->Write( "¡¡¡¡¡¡¡¡Comienza update_pertenencias_estaticas!!!!", 1 );
		before = System::DateTime::Now;
		start = System::DateTime::Now;
		num_operaciones = 0;

		if ( ! this->db_enabled )
		{
			this->logger->Write( "¡¡¡¡¡¡¡¡Base de datos deshabilitada!!!!", 5 );
			throw gcnew System::Exception();
		}
		this->connection->Open();
		pertenencias_db = gcnew Generic::List<Pertenencia^>();
		
		/* -------------construir lista de pertenencias
			Se recuperan de la base de datos todas las pertenencias de grupos estáticos o extensibles
				El parámetro estado servirá para distinguir aquellas pertenencias estáticas ( cuando == PERTENENCIA_ESTATICA )
		*/

		wchar_t g_estatico = SUBSCRIBER_GRUPO_ESTATICO;
		wchar_t g_extensible = SUBSCRIBER_GRUPO_EXTENSIBLE;

		queryString =	System::String::Format( 
						"SELECT g_ssi, t_ssi, estado, tipo FROM SICOM.dbo.tetra_pertenencias "
						"INNER JOIN SICOM.dbo.tetra_grupos ON SICOM.dbo.tetra_pertenencias.g_ssi = SICOM.dbo.tetra_grupos.ssi "
						"WHERE tipo='{0}' OR tipo = '{1}'",
						g_estatico , g_extensible );

		sql_query = gcnew OdbcCommand( queryString, this->connection );
		reader = sql_query->ExecuteReader();
		num_operaciones ++;
		// Call Read before accessing data

		after = System::DateTime::Now;
		duration_ms = ( after->Ticks - before->Ticks ) / 10000;
		this->logger->Write( System::String::Format( "UPDATE_PERTENENCIAS_ESTATICAS leer de db en {0} ms, operaciones realizadas: {1}", duration_ms, num_operaciones ), 1 );
		before = System::DateTime::Now;

		while ( reader->Read() )
		{
			pertenencia_db = gcnew Pertenencia( reader->GetInt32(0), reader->GetInt32(1) );
			pertenencia_db->estatica = ( reader->GetInt32(2) == PERTENENCIA_ESTATICA );			// Se distinguirán las pertenencias no estáticas
			pertenencias_db->Add( pertenencia_db );
		}
		// Call Close when done reading.
		reader->Close();
		
		//--------------------------------------------------------------------------------------------
		//Para cada subscriber de "subscriber_updated_list" (xml) hay que ver si hay algun subscriber con el mismo
		//ssi y actuar en consecuencia. Bucle con complejidad O(n*n)!!!!
		
		//pertenencias_db->TrimExcess();	//ahorro memoria

		for each ( pertenencia_xml in pertenencias_estaticas_xml )
		{
			try
			{
				found = false;
				for each ( pertenencia_db in pertenencias_db )
				{
					// si se trata del mismo ssi
					if ( ( pertenencia_db->g_ssi == pertenencia_xml->g_ssi ) && ( pertenencia_db->t_ssi == pertenencia_xml->t_ssi ) )
					{
						if ( ! pertenencia_db->estatica )		// La pertenencia es dinámica!!! habrá que actualizarla a estática!
						{
							this->logger->Write( System::String::Format( "se encontro la pertencia dinamica!!!! g:{0}-t:{1} del xml", pertenencia_xml->g_ssi, pertenencia_xml->t_ssi ), -1 );

							queryString = System::String::Format(	"UPDATE SICOM.dbo.tetra_pertenencias SET estado = {0} where g_ssi = {1} and t_ssi={2}",
																PERTENENCIA_ESTATICA, pertenencia_xml->g_ssi, pertenencia_xml->t_ssi );

							sql_query = gcnew OdbcCommand( queryString, this->connection );
							sql_query->ExecuteNonQuery();
							num_operaciones ++;
						}

						found = true;		// Se ha encontrado la pertenencia del xml en la base de datos, no se requiere ninguna acción
						pertenencia_db->explorado = true;		// La pertenencia estática se ha encontrado, no se borrará
						break;			// me salgo del bucle
					}
				}
				if ( ! found )		// Se ha recorrido las pertencias de la db sin encontrar un match
				{					// Habrá que insertar una pertencia estatica
					this->logger->Write( System::String::Format( "no se encontro la pertencia g:{0}-t:{1} del xml", pertenencia_xml->g_ssi, pertenencia_xml->t_ssi ), -1 );

					queryString = System::String::Format(	"INSERT INTO SICOM.dbo.tetra_pertenencias (g_ssi, t_ssi, estado, t_cambio, intentos, attached) "
															"VALUES ({0}, {1}, {2}, GETDATE(), 0, {3} )",
														pertenencia_xml->g_ssi, pertenencia_xml->t_ssi, PERTENENCIA_ESTATICA, PER_ATTACH_UNKNOWN );

					sql_query = gcnew OdbcCommand( queryString, this->connection );
					sql_query->ExecuteNonQuery();
					num_operaciones ++;
				}
			}
			catch ( System::Exception^ e )
			{
				this->logger->Write( System::String::Format( "Problema actualizando la pertenencia estatica g:{0}-t:{1} del xml", pertenencia_db->g_ssi, pertenencia_db->t_ssi ), 4 );
			}
		}

		after = System::DateTime::Now;
		duration_ms = ( after->Ticks - before->Ticks ) / 10000;
		this->logger->Write( System::String::Format( "UPDATE_PERTENENCIAS_ESTATICAS recorrer estructuras en {0} ms, operaciones realizadas: {1}", duration_ms, num_operaciones ), 2 );
		before = System::DateTime::Now;

		// Se ha terminado de analizar las dos listas. Se analizará la lista de subscriptores en la db para ver si hay alguno no explorado
		//		=> el subscriptor ya no existe en el NMS y habrá que borrarlo
		for each ( pertenencia_db in pertenencias_db )
		{
			try
			{
				if ( ( ! pertenencia_db->explorado ) && pertenencia_db->estatica )		// Solo se borrarán aquellas pertenencias estáticas en la base de datos que no se encuentren en el xml
				{
					this->logger->Write( System::String::Format( "La pertenencia estatica g:{0}-t:{1} de la db no está en el xml!!", pertenencia_db->g_ssi, pertenencia_db->t_ssi ), -1 );

					queryString = System::String::Format( "DELETE FROM SICOM.dbo.tetra_pertenencias WHERE g_ssi={0} AND t_ssi={1}", pertenencia_db->g_ssi, pertenencia_db->t_ssi );
					sql_query = gcnew OdbcCommand( queryString, this->connection );
					sql_query->ExecuteNonQuery();
					num_operaciones ++;
				}
			}
			catch ( System::Exception^ e )
			{
				this->logger->Write( System::String::Format( "Problema borrando la pertenencia estatica g:{0}-t:{1} del xml", pertenencia_db->g_ssi, pertenencia_db->t_ssi ), 4 );
			}
		}
		after = System::DateTime::Now;
		duration_ms = ( after->Ticks - before->Ticks ) / 10000;
		this->logger->Write( System::String::Format( "UPDATE_PERTENENCIAS_ESTATICAS borrar de db en {0} ms, operaciones realizadas: {1}", duration_ms, num_operaciones ), 1 );

		end = System::DateTime::Now;
		duration_ms = ( end->Ticks - start->Ticks ) / 10000;
		this->logger->Write( System::String::Format( "COMPLETADO update_pertenencias_estaticas en {0} ms", duration_ms ), 1 );

		return true;
	}
	catch(System::Exception^ e)
	{
		this->logger->Write( "error actualizando pertenencias estaticas", 4 );
		return false;
	}
	finally
	{
		this->connection->Close();
		Monitor::Exit( this->connection );;
	}
}

void Database::fill_db ( )
{
	System::String ^queryString;
	OdbcCommand^ sql_query;
	System::DateTime ^before, ^after;
	try
	{
		Monitor::Enter( this->connection );
		before = System::DateTime::Now;
		this->connection->Open();
		if ( ! this->db_enabled )
		{
			this->logger->Write( "¡¡¡¡¡¡¡¡Base de datos deshabilitada!!!!", 5 );
			throw gcnew System::Exception();
		}
		
		for ( int i=1; i<6000; i++ )
		{
			queryString = System::String::Format(	"INSERT INTO SICOM.dbo.tetra_terminales (ssi, nombre, tipo, t_cambio, registrado, zona) "
														"VALUES ({0},'{1}','A',GETDATE(),'false','UNKNOWN')",
														i, "HELLOOOO" );

			sql_query = gcnew OdbcCommand( queryString, this->connection );
			sql_query->ExecuteNonQuery();

			queryString = System::String::Format(	"INSERT INTO SICOM.dbo.tetra_grupos (ssi, nombre, tipo) "
														"VALUES ({0},'{1}','S')",
														i + 9000, "GGGGG" );
			sql_query = gcnew OdbcCommand( queryString, this->connection );
			sql_query->ExecuteNonQuery();
		}
		after = System::DateTime::Now;
		System::Int32 duration_ms = ( after->Ticks - before->Ticks ) / 10000;
		this->logger->Write( System::String::Format( "COMPLETADO fill_db en {0} ms", duration_ms ), 1 );
	}
	catch(System::Exception^ e)
	{
		this->logger->Write( "error rellenando tablas", 4 );
	}
	finally
	{
		this->connection->Close();
		Monitor::Exit( this->connection );;
	}
}

void Database::fill_db_pertenencias ( )
{
	System::String ^queryString;
	OdbcCommand^ sql_query;
	System::DateTime ^before, ^after;
	try
	{
		Monitor::Enter( this->connection );
		before = System::DateTime::Now;
		this->connection->Open();
		if ( ! this->db_enabled )
		{
			this->logger->Write( "¡¡¡¡¡¡¡¡Base de datos deshabilitada!!!!", 5 );
			throw gcnew System::Exception();
		}
		for ( int i=1; i<6000; i++ )
		{
			queryString = System::String::Format(	"INSERT INTO SICOM.dbo.tetra_pertenencias (g_ssi, t_ssi, estado, t_cambio, intentos) "
													"VALUES ({0}, {1}, {2}, GETDATE(), 0, {3} )",
														i + 9000, i, PERTENENCIA_ESTATICA, PER_ATTACH_UNKNOWN );
			

			sql_query = gcnew OdbcCommand( queryString, this->connection );
			sql_query->ExecuteNonQuery();

		}
		after = System::DateTime::Now;
		System::Int32 duration_ms = ( after->Ticks - before->Ticks ) / 10000;
		this->logger->Write( System::String::Format( "COMPLETADO fill_db_pertenencias en {0} ms", duration_ms ), 1 );
	}
	catch(System::Exception^ e)
	{
		this->logger->Write( "error rellenando pertenencias", 4 );
	}
	finally
	{
		this->connection->Close();
		Monitor::Exit( this->connection );;
	}
}
