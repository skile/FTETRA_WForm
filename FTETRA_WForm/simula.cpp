/*#include "simula.h"


void simulaAll()
{
	Database ^db = gcnew Database();
	db->connect("DSN=testSQL;Uid=sa;Pwd=sasql;");
	db->insert();
	db->read();

	TcpManager ^manager = gcnew TcpManager(13000);
	manager->listen();

	Random^ rand = gcnew Random();

	array<Process^>^ procesos = gcnew array<Process^>(numClients);

	ProcessStartInfo^ info = gcnew ProcessStartInfo("C:\\SICOM\\SQL\\cliente_SQL\\Debug\\cliente_SQL.exe");
	info->CreateNoWindow=true;
	info->UseShellExecute=false;

	for(int i=0;i<numClients;i++)
	{
		procesos[i] = Process::Start(info);
	}
	
	System::Int32 id,dur;

		manager->SimInit->WaitOne();
		System::Console::WriteLine(String::Concat("Alcanzadas ", System::Convert::ToString(manager->numCon), " conexiones. Comienza Simulacion"));
		//manager->SimInit->Reset();

		db->start = System::DateTime::Now;
		for(int i=0;i<numOperations;i++)
		{
			db->insert();
			if(i==0)
				id = db->read();
			manager->avisaAcCore(id);
			id++;
			dur = rand->Next(10);
			System::Threading::Thread::Sleep(dur);
		}
		db->end = System::DateTime::Now;
		System::Console::WriteLine("FINISHED in: {0}", db->end.Ticks - db->start.Ticks);
		System::Console::WriteLine("numOperations: {0}  Last id: {1}  Mean duration: {2} ", db->numInsert -1 , id-1, (db->ticks/(db->numInsert -3)));
		manager->avisaAcCore(-1);
		System::Threading::Thread::Sleep(2000);
		System::Console::WriteLine("Mean Read at Clients: {0}", manager->readAll());
		for(int i=0;i<numClients;i++)
		{
			procesos[i]->Kill();
		}
		//break;
	System::Console::Read();
}
*/