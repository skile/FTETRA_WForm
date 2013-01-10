#ifndef CUSTOMLOGGER_H
#define CUSTOMLOGGER_H

#using <System.dll>

using namespace System::IO;

ref class CustomLogger
{
private:
	 
public:
	CustomLogger ( int warningLevel, int modo );
	bool RegisterWriter ( System::String^ path );
	bool Write ( System::String^ line, int warningLevel );

	TextWriter^ log;			// wrapper alrededor de streamWriter para que sea Thread-safe

	int warningLevel;			// 0 muy bajo nivel, detalle	1 bajo nivel	2 importante	3 muy importante, general
	int modo;					// 0-Console,	1-log.txt,	2-ambos

	array<System::ConsoleColor>^ color_code;
};

#endif