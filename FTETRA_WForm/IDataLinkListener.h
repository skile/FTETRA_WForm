
#ifndef IDATALINKLISTENER_H
#define IDATALINKLISTENER_H

// Includes
#include "asn/asn-incl.h"
#include "asn/acapi.h"
#include "BaseTypes.h"

using namespace SNACC;
// ----------------------------------------------------------------------------

/** ClassDescription */
class IDataLinkListener
{
  public:
    IDataLinkListener() {};
    virtual ~IDataLinkListener() {}; 
    virtual void dataReceived(ACAPI_PDU* acapi_pdu) = 0;
    virtual void connectionClosed() = 0;
    virtual void putLoggerQueue(std::string) = 0;
    virtual void putMsgQueue(std::string) = 0;


  private:
    IDataLinkListener(const IDataLinkListener&);
    const IDataLinkListener& operator=(const IDataLinkListener&);
};


#endif
