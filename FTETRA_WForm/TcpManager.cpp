#include "TcpManager.h"

TcpManager::TcpManager( System::String^ local_ip, System::Int32 port )
{
	this->numCon = 0;

	this->accore_port = port;

	if ( local_ip->Equals( System::String::Empty ) )
	{
		this->accore_ip = IPAddress::Any;						// Escucho en cualquier interfaz de red
	}
	else
	{
		this->accore_ip = IPAddress::Parse( local_ip );
	}

	//this->listener = gcnew TcpListener( this->accore_ip, this->accore_port );


	this->waitRead = gcnew AutoResetEvent(false);
	this->waitConnection = gcnew AutoResetEvent(false);

	this->incompleteRead = gcnew StringBuilder();
}

/*
	Método para escuchar indefinida y asincronamente a nuevas conexiones, una vez establecida una conexion se lee asincronamente
*/
void TcpManager::listen()
{
	NetworkStream^ stream;
	array<System::Byte>^ myReadBuffer;
	try
	{
		this->listener->Start();
		this->logger->Write( "Waiting for a connection...", 1 );
		// registrar aceptación asincrona de conexion
		this->listener->BeginAcceptTcpClient( gcnew System::AsyncCallback( &TcpManager::DoAcceptTcpClientCallback ), this );
		while( true )
		{
			try
			{
				this->waitConnection->WaitOne();
				stream = this->acCore->GetStream();
				while ( stream->CanRead )
				{
					myReadBuffer = gcnew array<System::Byte>(1024);
					// registrar lectura asíncrona
					stream->BeginRead( myReadBuffer, 0, myReadBuffer->Length, gcnew System::AsyncCallback( &TcpManager::AsyncRead ), this );
					this->waitRead->WaitOne();
					this->processAcCoreOrder( myReadBuffer );
				}	
			}
			catch ( System::Exception^ e ) 
			{
				this->logger->Write( "!! SOCKET readAsync() Exception: {0}", 4 );
				this->num_empty_reads = 0;

				//this->waitConnection->Reset();
				//this->waitRead->Reset();
			}
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "Problem with socket!!...", 4 );
	}
}

/*
	Se intentará conectarse al AcCore indefinidamente de manera sincrona, una vez establecida la conexion se leera de manera asincrona
*/
void TcpManager::stay_connected()
{
	NetworkStream^ stream;
	array<System::Byte>^ myReadBuffer;
	try
	{
		while( true )
		{
			try
			{
				this->acCore = gcnew TcpClient();
				this->logger->Write( "Waiting for a connection...", -2 );
				this->acCore->Connect( this->accore_ip, this->accore_port );
				this->logger->Write( "....CONNECTED!", 1 );
				stream = this->acCore->GetStream();
				this->consumidor->manager_connected();
				while ( stream->CanRead )
				{
					myReadBuffer = gcnew array<System::Byte>(1024);
					// registrar lectura asíncrona
					stream->BeginRead( myReadBuffer, 0, myReadBuffer->Length, gcnew System::AsyncCallback( &TcpManager::AsyncRead ), this );
					this->waitRead->WaitOne();
					this->processAcCoreOrder( myReadBuffer );
				}	
			}
			catch ( System::Exception^ e ) 
			{
				this->logger->Write( System::String::Format( "!! SOCKET Exception, retrying", e ),  -2 );
				this->num_empty_reads = 0;
				this->acCore->Close();
				System::Threading::Thread::Sleep(3000);
			}
		}
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "Problem with socket!!...", 4 );
	}
}
 
void TcpManager::avisaAcCore(System::String^ msg)
{
	try
	{
		array<System::Byte>^ msg_bytes = System::Text::Encoding::ASCII->GetBytes( msg );
		System::Int32 content_length = msg_bytes->Length;
		if ( content_length > 1000 )		// mensaje excesivamente largo
			throw gcnew System::Exception();
		array<System::Byte>^ final_msg_bytes = gcnew array<System::Byte>(content_length + 2);

		final_msg_bytes[0] = content_length & 0x000000FF;
		final_msg_bytes[1] = ( content_length & 0x0000FF00 ) >> 8;
		for (int i = 0; i < content_length; i ++ )
		{
			final_msg_bytes[2+i] = msg_bytes[i];
		}

		NetworkStream ^ stream;

		stream = this->acCore->GetStream();
		stream->Write( final_msg_bytes, 0, final_msg_bytes->Length );
		this->logger->Write( System::String::Format( "sent to AcCore: {0}", msg ), 0 );
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "Problem writing to AcCore Socket", 4 );
	}
}

// Process the client connection. 
void TcpManager::DoAcceptTcpClientCallback(System::IAsyncResult^ result)
{
    // Get the listener that handles the client request.
    TcpManager^ manager = safe_cast<TcpManager^>(result->AsyncState);
	manager->logger->Write( "Accepting a connection...:::::::::::::::::::::::::::::::::::::", 1 );
	TcpListener^ listener = manager->listener;

	manager->acCore = listener->EndAcceptTcpClient(result);
	manager->logger->Write( "acCore connected", 1 );

	manager->waitConnection->Set();

	NetworkStream^ stream = manager->acCore->GetStream();

	manager->listener->BeginAcceptTcpClient( gcnew System::AsyncCallback( &TcpManager::DoAcceptTcpClientCallback ), manager );
}

// Lee asincronamente del AcCore.
void TcpManager::AsyncRead( System::IAsyncResult^ _manager )
{
	try
	{
		TcpManager^ manager = safe_cast<TcpManager^>(_manager->AsyncState);
		manager->waitRead->Set();
	}
	catch( System::Exception^ e )
	{
		System::Console::WriteLine( "AsyncRead from AcCore exception" );
	}
}

/*
	Analiza la trama leída. Si es incompleta rellena this->incompleteRead y tratará de completarla más adelante
	Todas las tramas siguen el siguiente esquema:

	descripción	ini		cod		sep	.........	fin
	numBytes	3		n		1	.........	3
	valor		---				:	.........	,,,
*/
void TcpManager::processAcCoreOrder( array<System::Byte>^ myReadBuffer )
{
	array<System::String^>^ campos = nullptr;
	array<wchar_t>^ separadores = { '-', ':', ',' };
	StringBuilder^ data = gcnew StringBuilder();			// StringBuilder más comodo y eficiente para manipular Strings
	array<wchar_t>^ charToTrim = {0};
	//try
	//{
		// Trim es necesario para eliminar todos los bytes a '0', sino comportamiento inadecuado

		// Pruebas con codificaciones de texto => 
		//			UTF7
		//			28591-Western European ISO (iso-8859-1)
		//			1252 -Western European Windows (Windows-1252)

		data->Append( ( Encoding::GetEncoding( "iso-8859-1" )->GetString( myReadBuffer, 0, myReadBuffer->Length )->Trim( charToTrim ) ) );
		this->logger->Write( System::String::Concat( "socket AcCore, LEIDO::::::: ", data->ToString() ), 1 );
		if( data->Length == 0 )
		{
			this->num_empty_reads++;
		}
		else
		{
			this->num_empty_reads = 0;
		}
		if ( this->num_empty_reads > 5 )
		{
			throw gcnew System::Exception();
		}
		while ( data->Length > 0 )
		{
			System::Int32 indexIni = data->ToString()->IndexOf("---", System::StringComparison::Ordinal);
			System::Int32 indexEnd = data->ToString()->IndexOf(",,,", System::StringComparison::Ordinal);

			//trama entera desde inicio
			if ( indexIni >= 0 && indexEnd > indexIni )
			{
				campos = data->ToString()->Split( separadores, System::StringSplitOptions::RemoveEmptyEntries );
				try
				{
					EventoCola^ eventoAcCore = gcnew EventoCola();
					eventoAcCore->tipo = System::Convert::ToInt32( campos[0] );
					indexIni = indexIni + 4 + campos[0]->Length;									// el contenido del evento solo contendrá la información necesaria
					eventoAcCore->contenido = data->ToString()->Substring( indexIni, indexEnd - indexIni );

					this->consumidor->cola->Enqueue(eventoAcCore);

					if( this->consumidor->permiso_continua )	// Solo continuaré consumiendo si hay permiso
						this->consumidor->continua->Set();

					this->log_evento( eventoAcCore );
				}
				catch ( System::Exception^ e  )
				{
					this->logger->Write( System::String::Concat( "no se puede leer el codigo de la trama: ", campos[0] ), 2 );
				}
				// Elimino la trama leida del mensaje completo
				data->Remove( 0, indexEnd + 3 );
				this->incompleteRead->Clear();
			}
			// trama incompleta, inicio pero no final
			if ( indexIni == 0 && indexEnd < 0 )
			{
				this->incompleteRead->Clear();							// Si había un inicio de trama anterior se descarta
				this->incompleteRead->Append( data );
				data->Clear();
			}
			// trama incompleta, final sin inicio.
			if ( indexEnd > 0 )
			{
				if ( indexIni < 0 )									// En el mensaje recibido hay un final de trama solo
				{
					if ( this->incompleteRead->Length > 0 )					// Si no había un inicio de trama previamente recibido se descarta el final
					{
						data->Insert( 0, this->incompleteRead->ToString() );
						this->incompleteRead->Clear();
					}
					else
						data->Clear();
				}
				if ( indexIni > indexEnd )							// En el mensaje recibido hay un final de trama seguido por el inicio de otra trama
				{
					if ( this->incompleteRead->Length > 0 )					// Si no había un inicio de trama previamente recibido se descarta el final
					{
						data->Insert( 0, this->incompleteRead->ToString() );
						this->incompleteRead->Clear();
					}
					else
						data->Remove( 0, indexIni );
				}
			}
		//}
	}
}

void TcpManager::log_evento( EventoCola^ evento_a_encolar )
{
	try
	{
		System::String^ codigo_orden;
		switch( evento_a_encolar->tipo )
		{
		case AC_ENVIA_SDS:
			codigo_orden = "AC_ENVIA_SDS";
			break;
		case AC_MOD_DGNA:
			codigo_orden = "AC_MOD_DGNA";
			break;
		case AC_CALL_DUPLEX:
			codigo_orden = "AC_CALL_DUPLEX";
			break;
		case AC_CALL_SIMPLEX:
			codigo_orden = "AC_CALL_SIMPLEX";
			break;
		case AC_CALL_GROUP:
			codigo_orden = "AC_CALL_GROUP";
			break;
		case AC_PTT:
			codigo_orden = "AC_PTT";
			break;
		case AC_PINCHA_LINEA:
			codigo_orden = "AC_PINCHA_LINEA";
			break;
		case AC_CAMBIA_GRUPO_LINEA:
			codigo_orden = "AC_CAMBIA_GRUPO_LINEA";
			break;
		case AC_MOD_MON_SSI:
			codigo_orden = "AC_MOD_MON_SSI";
			break;
		case AC_MON_FORCE_CALL_END:
			codigo_orden = "AC_MON_FORCE_CALL_END";
			break;
		case AC_LISTEN_MON_SSI:
			codigo_orden = "AC_LISTEN_MON_SSI";
			break;
		case AC_RESET_MON:
			codigo_orden = "AC_RESET_MON";
			break;
		default:
			codigo_orden = "UNKNOWM";
			break;
		}
		this->logger->Write( System::String::Format( "TcpManager encola evento: cod- {0} , contenido- {1}", codigo_orden, evento_a_encolar->contenido ), 0 );
	}
	catch( System::Exception^ e )
	{
		this->logger->Write( "problema en log_evento", 4 );
	}

}
