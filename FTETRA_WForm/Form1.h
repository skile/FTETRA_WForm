#pragma once

#include "stdafx.h"
#include "Form1.h"
#include "TetraInterface.h"
#include "Consumidor.h"
#include "Database.h"
#include "CustomLogger.h"
#include "TcpManager.h"
#include "Config_data.h"

namespace FTETRA_WForm {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for Form1
	/// </summary>
	public ref class Form1 : public System::Windows::Forms::Form
	{
	public:
		Consumidor^ consumidor;
		Consumidor^ con_aux;
	private: System::Windows::Forms::ColumnHeader^  columnHeader5;
	private: System::Windows::Forms::ColumnHeader^  columnHeader13;
	private: System::Windows::Forms::ColumnHeader^  columnHeader14;
	private: System::Windows::Forms::CheckBox^  checkBox1;
	private: System::Windows::Forms::CheckBox^  checkBox2;
	private: System::Windows::Forms::CheckBox^  checkBox3;
	private: System::Windows::Forms::CheckBox^  checkBox4;
	private: System::Windows::Forms::CheckBox^  checkBox5;
	public: 
		static Consumidor ^aux;

		Form1(Consumidor^ consum)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//

			this->consumidor = consum;
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~Form1()
		{
			if (components)
			{
				delete components;
			}
		}
	public: System::Windows::Forms::ListView^  listView1;
	protected: 

	protected: 
	private: System::Windows::Forms::Button^  button1;




	private: System::Windows::Forms::Timer^  timer1;
	private: System::Windows::Forms::ColumnHeader^  columnHeader1;
	private: System::Windows::Forms::ColumnHeader^  columnHeader2;
	private: System::Windows::Forms::ColumnHeader^  columnHeader3;
	private: System::Windows::Forms::ColumnHeader^  columnHeader4;

	private: System::Windows::Forms::ColumnHeader^  columnHeader6;
	private: System::Windows::Forms::ColumnHeader^  columnHeader7;
	private: System::Windows::Forms::ColumnHeader^  columnHeader8;
	private: System::Windows::Forms::ColumnHeader^  columnHeader9;
	private: System::Windows::Forms::ColumnHeader^  columnHeader10;
	private: System::Windows::Forms::ColumnHeader^  columnHeader11;
	private: System::Windows::Forms::ColumnHeader^  columnHeader12;
	private: System::ComponentModel::IContainer^  components;


	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>

	void Fill_ListView()
	{
		//Vacia la ListView

		listView1->Items->Clear();

		//La rellena con nuevos elementos
		for(int i=0;i<consumidor->lineas->Length;i++){

			int^ num_linea = gcnew int(consumidor->lineas[i]->comunicacion->id);
			bool^ act = gcnew bool(consumidor->lineas[i]->activa);
			bool^ desp = gcnew bool(consumidor->lineas[i]->despachador);
			bool^ pin  = gcnew bool(consumidor->lineas[i]->pinchada);
			bool^ att = gcnew bool(consumidor->lineas[i]->attached);
			bool^ sds  = gcnew bool(consumidor->lineas[i]->sds->estado_ok);
			bool^ ss  = gcnew bool(consumidor->lineas[i]->ss->estado_ok);
			int^ ssi_lin = gcnew int(consumidor->lineas[i]->ssi_linea);
			int^ ssi_des = gcnew int(consumidor->lineas[i]->comunicacion->ssi_destino);
			int^ ssi_orig = gcnew int(consumidor->lineas[i]->comunicacion->ssi_origen);
			int^ ssi_grup_def = gcnew int(consumidor->lineas[i]->ssi_grupo_defecto);
			int^ pri = gcnew int(consumidor->lineas[i]->comunicacion->priority);

			System::String ^tipo, ^estado;
			Comunicacion_tetra^ llamada = consumidor->lineas[i]->comunicacion;
			
			
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
			
			array<String^>^ subItems = gcnew array <String^>(14);
			subItems[0] = System::String::Format( "{0}",i ); 
			subItems[1] = (tipo);
			subItems[2] = (estado);
			subItems[3] = System::String::Format( "{0}",pin ); 
			subItems[4] = System::String::Format( "{0}",pri ); 
			subItems[5] = System::String::Format( "{0}",ssi_lin );
			subItems[6] = System::String::Format( "{0}",ssi_des );
			subItems[7] = System::String::Format( "{0}",ssi_orig );
			subItems[8] = System::String::Format( "{0}",act );
			subItems[9] = System::String::Format( "{0}",desp );
			subItems[10] = System::String::Format( "{0}",att ); 
			subItems[11] = System::String::Format( "{0}",ssi_grup_def ); 
			subItems[12] = System::String::Format( "{0}",ss );
			subItems[13] = System::String::Format( "{0}",sds );

//			listView1->Items->Remove(i);
			
			ListViewItem^ itm = gcnew ListViewItem(subItems);
			//itm->SubItems[2]->BackColor = System::Drawing::Color::Red;
			//itm->SubItems[3]->Text = " "+i;
			listView1->Items->Add(itm);
			listView1->GridLines = true;
		}
	}


#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->components = (gcnew System::ComponentModel::Container());
			this->listView1 = (gcnew System::Windows::Forms::ListView());
			this->columnHeader10 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader9 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader8 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader3 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader11 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader12 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader6 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader7 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader1 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader2 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader4 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader5 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader13 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader14 = (gcnew System::Windows::Forms::ColumnHeader());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->timer1 = (gcnew System::Windows::Forms::Timer(this->components));
			this->checkBox1 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox2 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox3 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox4 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox5 = (gcnew System::Windows::Forms::CheckBox());
			this->SuspendLayout();
			// 
			// listView1
			// 
			this->listView1->Columns->AddRange(gcnew cli::array< System::Windows::Forms::ColumnHeader^  >(14) {this->columnHeader10, this->columnHeader9, 
				this->columnHeader8, this->columnHeader3, this->columnHeader11, this->columnHeader12, this->columnHeader6, this->columnHeader7, 
				this->columnHeader1, this->columnHeader2, this->columnHeader4, this->columnHeader5, this->columnHeader13, this->columnHeader14});
			this->listView1->Location = System::Drawing::Point(28, 26);
			this->listView1->Name = L"listView1";
			this->listView1->RightToLeft = System::Windows::Forms::RightToLeft::No;
			this->listView1->Size = System::Drawing::Size(885, 483);
			this->listView1->TabIndex = 0;
			this->listView1->UseCompatibleStateImageBehavior = false;
			this->listView1->View = System::Windows::Forms::View::Details;
			this->listView1->SelectedIndexChanged += gcnew System::EventHandler(this, &Form1::listView1_SelectedIndexChanged);
			// 
			// columnHeader10
			// 
			this->columnHeader10->Text = L"Indice";
			this->columnHeader10->Width = 46;
			// 
			// columnHeader9
			// 
			this->columnHeader9->Text = L"Tipo";
			this->columnHeader9->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// columnHeader8
			// 
			this->columnHeader8->Text = L"Estado";
			this->columnHeader8->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->columnHeader8->Width = 84;
			// 
			// columnHeader3
			// 
			this->columnHeader3->Text = L"Pinchada";
			this->columnHeader3->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// columnHeader11
			// 
			this->columnHeader11->Text = L"Prioridad";
			this->columnHeader11->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// columnHeader12
			// 
			this->columnHeader12->Text = L"SSI_Linea";
			this->columnHeader12->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->columnHeader12->Width = 70;
			// 
			// columnHeader6
			// 
			this->columnHeader6->Text = L"SSI_Destino";
			this->columnHeader6->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->columnHeader6->Width = 77;
			// 
			// columnHeader7
			// 
			this->columnHeader7->Text = L"SSI_Origen";
			this->columnHeader7->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->columnHeader7->Width = 77;
			// 
			// columnHeader1
			// 
			this->columnHeader1->Text = L"Activa";
			this->columnHeader1->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->columnHeader1->Width = 52;
			// 
			// columnHeader2
			// 
			this->columnHeader2->Text = L"Despachada";
			this->columnHeader2->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->columnHeader2->Width = 82;
			// 
			// columnHeader4
			// 
			this->columnHeader4->Text = L"Attached";
			this->columnHeader4->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// columnHeader5
			// 
			this->columnHeader5->Text = L"Grupo";
			this->columnHeader5->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// columnHeader13
			// 
			this->columnHeader13->Text = L"ss";
			this->columnHeader13->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->columnHeader13->Width = 47;
			// 
			// columnHeader14
			// 
			this->columnHeader14->Text = L"sds";
			this->columnHeader14->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->columnHeader14->Width = 46;
			// 
			// button1
			// 
			this->button1->Location = System::Drawing::Point(956, 26);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(92, 29);
			this->button1->TabIndex = 1;
			this->button1->Text = L"Useless";
			this->button1->UseVisualStyleBackColor = true;
			this->button1->Click += gcnew System::EventHandler(this, &Form1::button1_Click);
			// 
			// timer1
			// 
			this->timer1->Enabled = true;
			this->timer1->Interval = 1000;
			this->timer1->Tick += gcnew System::EventHandler(this, &Form1::timer1_Tick);
			// 
			// checkBox1
			// 
			this->checkBox1->AutoSize = true;
			this->checkBox1->Cursor = System::Windows::Forms::Cursors::No;
			this->checkBox1->Enabled = false;
			this->checkBox1->Location = System::Drawing::Point(28, 627);
			this->checkBox1->Name = L"checkBox1";
			this->checkBox1->Size = System::Drawing::Size(68, 17);
			this->checkBox1->TabIndex = 2;
			this->checkBox1->Text = L"TETRA1";
			this->checkBox1->UseVisualStyleBackColor = true;
			this->checkBox1->CheckedChanged += gcnew System::EventHandler(this, &Form1::checkBox1_CheckedChanged);
			// 
			// checkBox2
			// 
			this->checkBox2->AutoSize = true;
			this->checkBox2->Enabled = false;
			this->checkBox2->Location = System::Drawing::Point(28, 664);
			this->checkBox2->Name = L"checkBox2";
			this->checkBox2->Size = System::Drawing::Size(68, 17);
			this->checkBox2->TabIndex = 3;
			this->checkBox2->Text = L"TETRA2";
			this->checkBox2->UseVisualStyleBackColor = true;
			// 
			// checkBox3
			// 
			this->checkBox3->AutoSize = true;
			this->checkBox3->Enabled = false;
			this->checkBox3->Location = System::Drawing::Point(150, 627);
			this->checkBox3->Name = L"checkBox3";
			this->checkBox3->Size = System::Drawing::Size(56, 17);
			this->checkBox3->TabIndex = 4;
			this->checkBox3->Text = L"NMS1";
			this->checkBox3->UseVisualStyleBackColor = true;
			// 
			// checkBox4
			// 
			this->checkBox4->AutoSize = true;
			this->checkBox4->Enabled = false;
			this->checkBox4->Location = System::Drawing::Point(150, 664);
			this->checkBox4->Name = L"checkBox4";
			this->checkBox4->Size = System::Drawing::Size(56, 17);
			this->checkBox4->TabIndex = 5;
			this->checkBox4->Text = L"NMS2";
			this->checkBox4->UseVisualStyleBackColor = true;
			// 
			// checkBox5
			// 
			this->checkBox5->AutoSize = true;
			this->checkBox5->Enabled = false;
			this->checkBox5->Location = System::Drawing::Point(275, 627);
			this->checkBox5->Name = L"checkBox5";
			this->checkBox5->Size = System::Drawing::Size(56, 17);
			this->checkBox5->TabIndex = 6;
			this->checkBox5->Text = L"CORE";
			this->checkBox5->UseVisualStyleBackColor = true;
			this->checkBox5->CheckedChanged += gcnew System::EventHandler(this, &Form1::checkBox5_CheckedChanged);
			// 
			// Form1
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(1060, 715);
			this->Controls->Add(this->checkBox5);
			this->Controls->Add(this->checkBox4);
			this->Controls->Add(this->checkBox3);
			this->Controls->Add(this->checkBox2);
			this->Controls->Add(this->checkBox1);
			this->Controls->Add(this->button1);
			this->Controls->Add(this->listView1);
			this->Name = L"Form1";
			this->Text = L"Visual Tetra";
			this->ResumeLayout(false);
			this->PerformLayout();

		}

	 
#pragma endregion
	private: System::Void listView1_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 }
	private: System::Void button1_Click(System::Object^  sender, System::EventArgs^  e) {	
			
				
			 }
	private: System::Void timer1_Tick(System::Object^  sender, System::EventArgs^  e) {
				 
				 Fill_ListView();	
					
			 }
private: System::Void checkBox1_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void checkBox5_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
		 }
};
}

