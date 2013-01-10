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

 bool CustomLogger::RegisterWriter ( System::String^ path )
{
	try
	{
		this->log = TextWriter::Synchronized( File::AppendText( path ) );			// Thread-safe!!
		return true;
	}
	catch ( System::Exception^ e )
	{
		return false;
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
		return true;
	}
	catch ( System::Exception^ e )
	{
		return false;
	}
}