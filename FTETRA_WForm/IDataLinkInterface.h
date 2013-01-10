/*---------------------------------------------------------------------------- */

#ifndef FTP_IDATALINKINTERFACE_H
#define FTP_IDATALINKINTERFACE_H

// Includes
#include "BaseTypes.h"
#include "IDataLinkListener.h"

// ----------------------------------------------------------------------------

/** ClassDescription */
class IDataLinkInterface
{
  public:
    IDataLinkInterface() {};
    virtual ~IDataLinkInterface() {};

    virtual void registerDataLinkListener(IDataLinkListener* listener) = 0;
//    virtual void sendData(const SubscriberId &subscriberId, const Data& data) = 0;

  private:
    IDataLinkInterface(const IDataLinkInterface&);
    const IDataLinkInterface& operator=(const IDataLinkInterface&);
};


#endif
