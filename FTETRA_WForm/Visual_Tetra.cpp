#include "Visual_Tetra.h"

Visual_Tetra::Visual_Tetra ()
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

void Visual_Tetra::tetra_start(  )
{
	try
	{
		
	}
	catch ( System::Exception^ e )
	{
		this->logger->Write( "Problema al lanzar la VISUAL TETRA !!!!!!", 8 );
	}
}