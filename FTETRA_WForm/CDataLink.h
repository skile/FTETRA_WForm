/* ---------------------------------------------------------------------
 $RCSfile: CDataLink.h,v $
 Project: Application Platform DLL
 Subproject:
 Date of Creation: 23.11.2004
 Main Author: Cymation Technology GmbH 
 Copyright: (c) 2004 Cymation Technology GmbH.
            All Rights Reserved.

            N O T I C E
            THIS MATERIAL IS CONSIDERED A TRADE SECRET BY Cymation Technology GmbH.
            UNAUTHORIZED ACCESS, USE, REPRODUCTION OR
            DISTRIBUTION IS PROHIBITED.
 $Revision: 1.11 $
 ---------------------------------------------------------------------
 Description:
  
 ---------------------------------------------------------------------
 Class list:
 ---------------------------------------------------------------------
 $Log: CDataLink.h,v $
 Revision 1.11  2007/06/26 07:46:48  rothkampu
 Version 1.2.17

 Revision 1.10  2006/12/04 15:03:22  rothkampu
 *** empty log message ***

 Revision 1.9  2006/04/25 07:00:09  guentherf
 neue Testfälle

 Revision 1.8  2006/02/01 16:47:51  rothkampu
 *** empty log message ***

 Revision 1.7  2006/02/01 16:27:28  rothkampu
 *** empty log message ***

 Revision 1.6  2005/12/02 12:00:10  guentherf
 *** empty log message ***

 Revision 1.5  2004/12/21 12:32:54  rothkampu
 *** empty log message ***

 Revision 1.4  2004/12/17 16:19:14  rothkampu
 *** empty log message ***

 Revision 1.3  2004/12/17 10:51:32  rothkampu
 *** empty log message ***

 Revision 1.2  2004/11/25 08:52:24  rothkampu
 *** empty log message ***

 Revision 1.1  2004/11/25 08:46:03  rothkampu
 *** empty log message ***

 Revision 1.2  2004/11/25 08:43:25  rothkampu
 Zwischenspeicherung


 ---------------------------------------------------------------------
*/
#ifndef CDataLink_H
#define CDataLink_H

/* ---------------------------------------------------------------------
   Include-Files
   ---------------------------------------------------------------------
*/
#ifdef WIN32
  #include <windows.h>
  #include <time.h>
  #include "asn/asn-incl.h"
  #include "asn/acapi.h"
#else
  #include "asn/asn-incl.h"
  #include "asn/acapi.h"
  #include "socket/CTcpSocket.h"
  #include <sstream>
  #include <unistd.h>

//  #include "general/ext.h"

#endif

#include <string>
#include <list>
#include "BaseTypes.h"
#include "IDataLinkListener.h"
#include "CreatePDU.h"

/* ---------------------------------------------------------------------
   NAMESPACE
   ---------------------------------------------------------------------
*/
using namespace SNACC;
using namespace std;

/* ---------------------------------------------------------------------
   KLASSENDEFINITIONEN
   ---------------------------------------------------------------------
*/
class CDataLink
{
  public:
    CDataLink (const std::string& host_name, unsigned short portNumber, char *licence_key, char logger_state);
    virtual ~CDataLink(void);
    bool ConnectTap ();
    void SetHostname (const std::string& host_name, unsigned short portNumber, char logger_state);

    bool start();
    bool stop();
    bool connectedTAP;
    bool getConnectedTap()
    {
      return connectedTAP;
    }

    void registerDataLinkListener(IDataLinkListener* pInterface);

    virtual bool sendData(ACAPI_PDU* acapi_pdu);
    virtual long getSentQueueSize();


  private:
    void   sendRecvThread();
    bool   sendData(ACAPI_Transport* acapi_trans);
    #ifdef WIN32
      static DWORD WINAPI sendRecvThreadStatic(LPVOID lpParameter);
    #else
      static void *sendRecvThreadStatic(void *lpParameter);
    #endif
    void   binaryToAcapi (const Data& data);
    void   SendConnectionState();
    void   insertIntoLoggerQueue(string text);


  private:
    struct SendData
    {
      std::string m_handle;
      std::string m_data;

      SendData() :
               m_handle(),
               m_data() {}
               SendData(std::string handle, std::string data) :
               m_handle(handle),
               m_data(data) {}
    };

  private:
    std::string ownHostName;
    unsigned short ownPortNumber;
    IDataLinkListener *ownDataLinkListener;

    bool finishThread;
    bool finishedThread;
    #ifdef WIN32
      CRITICAL_SECTION sendDataCriticalSection;
    #else
      pthread_mutex_t sendDataCriticalSection;
    #endif
    std::list<SendData> sendDataList;

    unsigned long applRxMessageID;
    unsigned long applTxMessageID;

    int sd;

    CreatePDU _create_pdu;

    int i_d, i_m, i_y;

    char _logger_state;
    clock_t _time_for_last_send;


    #ifndef WIN32
      CTcpSocket tcp_socket;
      char c_getFrame(std::string &z_error);
      /*==>
      Read-Buffer, Parse-Buffer und Frame-Buffer.
      <==*/
      std::string _z_read_buf;
      std::string _z_parse_buf;
      std::string _z_frame_buf;

      pthread_t z_send_rec_thread;
    #endif
};

#endif
