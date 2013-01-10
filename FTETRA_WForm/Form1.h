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
	private: System::Windows::Forms::ListView^  listView1;
	protected: 
	private: System::Windows::Forms::Button^  button1;




	private: System::Windows::Forms::Timer^  timer1;
	private: System::Windows::Forms::ColumnHeader^  columnHeader1;
	private: System::Windows::Forms::ColumnHeader^  columnHeader2;
	private: System::Windows::Forms::ColumnHeader^  columnHeader3;
	private: System::Windows::Forms::ColumnHeader^  columnHeader4;
	private: System::ComponentModel::IContainer^  components;


	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>


#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->components = (gcnew System::ComponentModel::Container());
			this->listView1 = (gcnew System::Windows::Forms::ListView());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->timer1 = (gcnew System::Windows::Forms::Timer(this->components));
			this->columnHeader1 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader2 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader3 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader4 = (gcnew System::Windows::Forms::ColumnHeader());
			this->SuspendLayout();
			// 
			// listView1
			// 
			this->listView1->Columns->AddRange(gcnew cli::array< System::Windows::Forms::ColumnHeader^  >(4) {this->columnHeader1, this->columnHeader2, 
				this->columnHeader3, this->columnHeader4});
			this->listView1->Location = System::Drawing::Point(24, 29);
			this->listView1->Name = L"listView1";
			this->listView1->Size = System::Drawing::Size(687, 571);
			this->listView1->TabIndex = 0;
			this->listView1->UseCompatibleStateImageBehavior = false;
			this->listView1->View = System::Windows::Forms::View::Details;
			this->listView1->SelectedIndexChanged += gcnew System::EventHandler(this, &Form1::listView1_SelectedIndexChanged);
			// 
			// button1
			// 
			this->button1->Location = System::Drawing::Point(730, 36);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(92, 29);
			this->button1->TabIndex = 1;
			this->button1->Text = L"Start";
			this->button1->UseVisualStyleBackColor = true;
			this->button1->Click += gcnew System::EventHandler(this, &Form1::button1_Click);
			// 
			// timer1
			// 
			this->timer1->Enabled = true;
			this->timer1->Interval = 1000;
			this->timer1->Tick += gcnew System::EventHandler(this, &Form1::timer1_Tick);
			// 
			// columnHeader1
			// 
			this->columnHeader1->Text = L"Activa";
			this->columnHeader1->Width = 61;
			// 
			// columnHeader2
			// 
			this->columnHeader2->Text = L"Despachada";
			this->columnHeader2->Width = 74;
			// 
			// columnHeader3
			// 
			this->columnHeader3->Text = L"Pinchada";
			// 
			// columnHeader4
			// 
			this->columnHeader4->Text = L"Attached";
			// 
			// Form1
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(832, 622);
			this->Controls->Add(this->button1);
			this->Controls->Add(this->listView1);
			this->Name = L"Form1";
			this->Text = L"Visual Tetra";
			this->ResumeLayout(false);

		}
#pragma endregion
	private: System::Void listView1_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 }
	private: System::Void button1_Click(System::Object^  sender, System::EventArgs^  e) {	
			bool activa = consumidor->lineas[1]->activa;
				 			 
			 }
	private: System::Void timer1_Tick(System::Object^  sender, System::EventArgs^  e) {
			 }
};
}

