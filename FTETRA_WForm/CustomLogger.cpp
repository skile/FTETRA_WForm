#include "CustomLogger.h"

CustomLogger::CustomLogger (int warningLevel, int modo)
{
	this->warningLevel = warningLevel;
	this->modo = modo;
	this->color_code = gcnew array<System::ConsoleColor>(8);
	this->color_code[0] = System::ConsoleColor::DarkGray;		// <0-TRAZAS
	this->color_code[1] = System::ConsoleColor::Gray;			// 0-INFO
	this->color_code[2] = System::ConsoleColor::DarkGreen;		// 1-INFO IMPTE
	this->color_code[3] = System::ConsoleColor::DarkYellow;		// 2-UNEXPECTED
	this->color_code[4] = System::ConsoleColor::DarkCyan;		// 3-LINE INFO
	this->color_code[5] = System::ConsoleColor::DarkRed;		// 4-ERROR
	this->color_code[6] = System::ConsoleColor::Red;			// 5-ERROR GRAVE
	this->color_code[7] = System::ConsoleColor::Magenta;		// 6-temporal pinpoint
}

 bool CustomLogger::RegisterWriter ( System::String^ path_folder )
{
	try
	{
		if( System::IO::Directory::Exists( path_folder ) == false )
		{
			System::IO::Directory::CreateDirectory( path_folder );
		}

		//this->log = TextWriter::Synchronized( File::AppendText( System::String::Concat( path, System::DateTime::Now.ToString( "/yyyy-MM-dd_HH:mm:ss" ),".txt" ) ) );			// Thread-safe!!
		
		//System::String^ name_file = gcnew System::String (System::String::Format( "{0}/{1}.txt",path,System::DateTime::Now.ToString( "/yyyy-MM-dd_HH:mm:ss" ) ) ); 


		//this->log = TextWriter::Synchronized( File::AppendText( path ) );			// Thread-safe!!
		//return true;

		//FileStream^ new_file = File::Create( System::String::Format( "{0}/{1}.txt", path, System::DateTime::Now.ToString( "/yyyy-MM-dd_HH:mm:ss" ) ) );
		//System::File^ new_file = gcnew System::File ();
		this->abs_file = System::String::Format( "{0}\{1}.txt", path_folder, System::DateTime::Now.ToString( "/yyyy-MM-dd_HH.mm.ss" ) );
		//FileInfo^ file_info = gcnew FileInfo( abs_file );
		this->log = TextWriter::Synchronized( File::AppendText( this->abs_file ) );			// Thread-safe!!
		return true;
	}
	catch ( System::Exception^ e )
	{
		return false;
	}
	

	/*
	
	try
	{

		//Comprueba si el directorio existe, y sino lo prueba
		if(System::IO::Directory::Exists( NAME_FOLDER )==false)
		{

		System::IO::Directory::CreateDirectory( NAME_FOLDER );

		}
		
		bool file_exists = File::Exists ( path );

		//System::String::Format( "{0}/{1}",name_folder,name_file ); 
		//FileAttributes ^aux_attributes= System::IO::File::FileAttributes (path);

		FileInfo^ file_info = gcnew FileInfo( path );

		if( (file_exists == true) &&( file_info->Length >MAX_LOG_SIZE ) == true )
		{
			//System::IO::File::Create( System::String::Concat( path, System::DateTime::Now.ToString( "yyyy-MM-dd_HH:mm:ss" ),".txt" ) );
			//System::String::Format( "{0}/{1}.txt",NAME_FOLDER,System::DateTime::Now.ToString( "yyyy-MM-dd_HH:mm:ss" )  )  ); 

			System::String^ name_file = gcnew System::String (System::String::Format( "{0}/{1}.txt",NAME_FOLDER,System::DateTime::Now.ToString( "yyyy-MM-dd_HH:mm:ss" ) ) ); 

			System::String^ name_file2 = gcnew System::String (System::String::Format( "{1}.txt",System::DateTime::Now.ToString( "yyyy-MM-dd_HH:mm:ss" ) ) ); 

			this->log = TextWriter::Synchronized ( File::AppendText( name_file2 ) );

			
		}
		//this->folder_file = System::String::Format( "{0}/{1}",NAME_FOLDER,name_file ); 
		//FileInfo^ file_info = gcnew FileInfo( path );

		//this->log = TextWriter::Synchronized( File::AppendText( System::String::Format ( "{0}/{1}.txt",NAME_FOLDER,System::DateTime::Now.ToString( "yyyy-MM-dd_HH:mm:ss" ) ) ) );			// Thread-safe!!
		return true;
	}

	catch ( System::Exception^ e )
	{
		return false;
	}
	*/
	
}


void CustomLogger::Check_size (){

		FileInfo^ file_info = gcnew FileInfo( this->abs_file );

		int number = file_info->Length;

		if(( file_info->Length > MAX_LOG_SIZE ) == true )
		{			
		this->abs_file = System::String::Format( "log_folder\{0}.txt", System::DateTime::Now.ToString( "/yyyy-MM-dd_HH.mm.ss" ) );

		this->log = TextWriter::Synchronized( File::AppendText( this->abs_file ) );				
		}
 }

/*
	Modos de funcionamiento:
		0 - Se escribe solo al archivo de texto si el warninglevel es mayor que el definido al crear el logger
		1 - Se escribe solo a la consola con un código de colores según el warninglevel mayor que el definido al crear el logger
		2 - Se escribe a ambos, archivo y consola
*/
bool CustomLogger::Write ( System::String^ line, int warningLevel )
{
	try
	{
		if ( warningLevel >= this->warningLevel )
		{
			if ( this->modo == 0 || this->modo == 2 )
			{
				this->log->WriteLine( System::String::Concat( "[", System::DateTime::Now.ToString( "yyyy-MM-dd HH:mm:ss" ), "] ", line ) );
				this->log->Flush();
			}
			if ( this->modo == 1 || this->modo == 2 )
			{
				System::Int32 color_index;
				if ( warningLevel < -1 )
					color_index = -1;
				else if ( warningLevel > 6 )
					color_index = 6;
				else
					color_index = warningLevel + 1;

				System::Console::ForegroundColor = this->color_code[ color_index ];
				System::Console::WriteLine( line );
				System::Console::ResetColor();
			}
		}
		
		this->Check_size();

		return true;
	}
	catch ( System::Exception^ e )
	{
		return false;
	}
}