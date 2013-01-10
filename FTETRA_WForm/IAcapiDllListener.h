
#ifndef IACAPIDLLLISTENER_H
#define IACAPIDLLLISTENER_H

// Includes
#include "BaseTypes.h"
// ----------------------------------------------------------------------------
/* ---------------------------------------------------------------------
   ENUMERATIONS
   ---------------------------------------------------------------------
*/
    enum
    {
      MSG_SDS_DATA,                             // 0 0x00
      MSG_SDS_STATUS,	                        // 1 0x01
      MSG_SDS_ACK,                              // 2 0x02
      MSG_CONNECTION_STATE,                     // 3 0x03
      MSG_SDS_SERVICE_CONNECTION_STATE,         // 4 0x04
      MSG_MON_SERVICE_CONNECTION_STATE,         // 5 0x05
      MSG_SUPPLEMETARY_SERVICE_CONNECTION_STATE,// 6 0x06
      MSG_DYN_GROUP_ADD_ACK,                    // 7 0x07
      MSG_DYN_GROUP_DEL_ACK,                    // 8 0x08
      MSG_DYN_GROUP_INTERROGATE_ACK,            // 9 0x09
      MSG_MON_FLEET_MONITORING_ACK,             //10 0x0A
      MSG_MON_CLOSE,                            //11 0x0B
      MSG_MON_LOCATION_UPDATE,                  //12 0x0C
      MSG_MON_LOCATION_DETACH,                  //13 0x0D
      MSG_MON_SDS_DATA,                         //14 0x0E
      MSG_MON_SDS_ACK,                          //15 0x0F
      MSG_MON_CC_INFORMATION,                   //16 0x10
      MSG_MON_TX_DEMAND,                        //17 0x11
      MSG_MON_TX_CEASED,                        //18 0x12
      MSG_MON_TX_GRANT,                         //19 0x13
      MSG_MON_DISCONNECT,                       //20 0x14
      MSG_MON_SDS_STATUS,                       //21 0x15
      MSG_MON_MONITORING_ACK,                   //22 0x16
      MSG_MON_CLOSE_ACK,                        //23 0x17
      MSG_OOCI_ASSIGN_ACK,                      //24 0x18
      MSG_OOCI_DEASSIGN_ACK,                    //25 0x19
      MSG_OOCI_INTERROGATE_ACK,                 //26 0x1A
// CALL CONTROL
      MSG_CC_SERVICE_CONNECTION_STATE,          //27 0x1B
      MSG_CC_INFORMATION,                       //28 0x1C
      MSG_CC_CONNECT,                           //29 0x1D
      MSG_CC_DISCONNECT,                        //30 0x1E
      MSG_CC_DISCONNECT_ACK,                    //31 0x1F
      MSG_CC_SETUP,                             //32 0x20
      MSG_CC_ACCEPTED,                          //33 0x21
      MSG_CC_CEASED,                            //34 0x22
      MSG_CC_GRANT,                             //35 0x23

      MSG_GROUP_ATTACH_ACK,                     //36 0x24
      MSG_GROUP_DETACH_ACK,                     //37 0x25

      MSG_CC_WAIT,                              //38 0x26
      MSG_CC_WAIT_ACK,                          //39 0x27
      MSG_CC_CONTINUE,                          //40 0x28
      MSG_CC_CONTINUE_ACK,                      //41 0x29
      MSG_CC_SETUP_ACK,                         //42 0x2A
      MSG_MON_INTERCEPT_ACK,                    //43 0x2B 
      MSG_MON_INTERCEPT_CONNECT,                //44 0x2C 
      MSG_MON_INTERCEPT_DISCONNECT,             //45 0x2D 
      MSG_MON_INTERCEPT_DISCONNECT_ACK,         //46 0x2E 
	  MSG_CC_CALL_FORCED_END_ACK,				//47 0x2F
	  MSG_CC_CANCEL_INCLUDE_ACK,				//48 0x30
	  MSG_OOCI_CANCEL_GATEWAY_ACK,				//49 0x31
	  MSG_CALL_FORWARD_ACTIVATE_ACK,		    //50 0x32
	  MSG_CALL_FORWARD_DEACTIVATE_ACK,			//51 0x33
	  MSG_CALL_FORWARD_CANCEL_ALL_ACK,			//52 0x34
	  MSG_CALL_FORWARD_INTERROGATE_ACK,			//53 0x35
	  MSG_GROUP_REPORT_ACK,						//54 0x36
	  MSG_GROUP_REPORT_INFO,					//55 0x37
	  MSG_MON_DTM_SIGN,							//56 0x38
	  MSG_MON_FORCED_CALL_END_ACK,			    //57 0x39
	  MSG_MON_ORDER_ACK,						//58 0x3A
  	  MSG_MON_ORDERCLOSE,						//59 0x3B
  	  MSG_MON_ORDERCLOSE_ACK,					//60 0x3C
      MSG_MON_INTERCEPT_ACK_RTP,                //61 0x3D 
      MSG_MON_INTERCEPT_CONNECT_RTP             //62 0x3E 

    };


/** ClassDescription */
class IAcapiDllListener
{
  public:
    IAcapiDllListener() {};
    virtual ~IAcapiDllListener() {}; 
    virtual void putMsgQueue(std::string) = 0;
    virtual void putLoggerQueue(std::string msg) = 0;


  private:
    IAcapiDllListener(const IAcapiDllListener&);
    const IAcapiDllListener& operator=(const IAcapiDllListener&);
};


#endif
