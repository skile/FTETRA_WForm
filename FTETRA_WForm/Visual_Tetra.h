#ifndef VISUAL_TETRA_H
#define VISUAL_TETRA_H

#include "Database.h"
#include "TetraInterface.h"
#include "CustomLogger.h"
#include "TcpManager.h"
#include "Config_data.h"


#using <System.dll>

using namespace System::Diagnostics;
using namespace System::Runtime::InteropServices;
using namespace System::Collections;
using namespace System::Collections::Concurrent;

ref class Database;
ref class TcpManager;
ref class Config_data;


ref class Visual_Tetra
{
private:
	 
public:
	Visual_Tetra ();

	bool genera_lineas ();

	
};

#endif