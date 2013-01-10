/*---------------------------------------------------------------------------- */

#ifndef CACAPI_DLL_INTERFACE_H
#define CACAPI_DLL_INTERFACE_H

// Includes

// ----------------------------------------------------------------------------

/** ClassDescription */
class CAcapiDllInterface
{
  public:
    CAcapiDllInterface() {};
    virtual ~CAcapiDllInterface() {};

    virtual void receivedMessage(std::string message) = 0;

  private:
    CAcapiDllInterface(const CAcapiDllInterface&);
    const CAcapiDllInterface& operator=(const CAcapiDllInterface&);
};


#endif  //CACAPI_DLL_INTERFACE_H
