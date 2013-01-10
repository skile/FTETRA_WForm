/*-----------------------------------------------------------------------------
  Cymation Technology GmbH, D-61273 Wehrheim   Copyright (C) by Cymation Technology GmbH 2004
-------------------------------------------------------------------------------
  Project       ACAPI-DLL
-------------------------------------------------------------------------------

  Modification History
  --------------------

  $Workfile:     $
  $Revision: 1.1 $
  $Log: BaseTypes.h,v $
  Revision 1.1  2004/11/16 16:04:16  rothkampu
  *** empty log message ***




---------------------------------------------------------------------------- */
#ifndef BASETYPES_H
#define BASETYPES_H

// Includes

/*---------------------------------------------------------------------------- */
#ifdef LIBTETRAFTP_EXPORTS
#define LIBTETRAFTP_API __declspec(dllexport)
#else
#define LIBTETRAFTP_API __declspec(dllimport)
#endif

#pragma warning(disable: 4786) // Bezeichner wurde auf '255' Zeichen in den Debug-Informationen verkürzt
#pragma warning(disable: 4514) // inline-Funktionen wegoptimiert, bleibt abgeschaltet.
#pragma warning(disable: 4511) 
#pragma warning(disable: 4512) 
#pragma warning(disable: 4663) 


#include <string>
#include <vector>


typedef std::string SubscriberId;
typedef std::string FileName;
typedef std::string DataBlock;
typedef std::string BitField;
typedef std::vector<unsigned char> Data;



#endif
