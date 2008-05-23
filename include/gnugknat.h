/*
 * gnugknat.h
 *
 * GnuGk NAT Traversal class.
 *
 * h323plus library
 *
 * Copyright (c) 2008 ISVO (Asia) Pte. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the General Public License (the  "GNU License"), in which case the
 * provisions of GNU License are applicable instead of those
 * above. If you wish to allow use of your version of this file only
 * under the terms of the GNU License and not to allow others to use
 * your version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GNU License. If you do not delete
 * the provisions above, a recipient may use your version of this file
 * under either the MPL or the GNU License."
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 *
 * The Initial Developer of the Original Code is ISVO (Asia) Pte. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log$
 * Revision 1.4  2008/02/01 07:50:17  shorne
 * added shutdown mutex to fix occasion shutdown timing errors.
 *
 * Revision 1.3  2008/01/18 01:36:42  shorne
 * Fix blocking and timeout on call ending
 *
 * Revision 1.2  2008/01/02 18:35:40  willamowius
 * make SetAvailable() return void
 *
 * Revision 1.1  2008/01/01 00:16:12  shorne
 * Added GnuGknat and FileTransfer support
 *
 *
 *
 */

#include <ptlib.h>
#include <h323.h>

#ifdef H323_GNUGK

#ifndef GNUGK_NAT
#define GNUGK_NAT

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class GNUGK_Feature;
class GNUGKTransport  : public H323TransportTCP
{
  PCLASSINFO(GNUGKTransport, H323TransportTCP);

  public:

	enum PDUType {
		e_raw,
	};

    /**Create a new transport channel.
     */
    GNUGKTransport(
      H323EndPoint & endpoint,        /// H323 End Point object
	  GNUGK_Feature * feat,			  /// Feature
	  PString & gkid                  /// Gatekeeper ID
    );

	~GNUGKTransport();

	/**Handle the GNUGK Signalling
	  */
	PBoolean HandleGNUGKSignallingChannelPDU();

	/**Handle the GNUGK Signalling
	  */
	PBoolean HandleGNUGKSignallingSocket(H323SignalPDU & pdu);

    /**Write a protocol data unit from the transport.
       This will write using the transports mechanism for PDU boundaries, for
       example UDP is a single Write() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual PBoolean WritePDU(
      const PBYTEArray & pdu  /// PDU to write
    );

    /**Read a protocol data unit from the transport.
       This will read using the transports mechanism for PDU boundaries, for
       example UDP is a single Read() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
	virtual PBoolean ReadPDU(
         PBYTEArray & pdu  /// PDU to Read
	);

	PBoolean CreateNewTransport();

	PBoolean InitialPDU();

	PBoolean SetGKID(const PString & newid);

	PBoolean isCall() { return isConnected; };

	void ConnectionLost(PBoolean established);

	PBoolean IsConnectionLost();


// Overrides
    /**Connect to the remote party.
      */
    virtual PBoolean Connect();

    /**Close the channel.(Don't do anything)
      */
    virtual PBoolean Close();

    virtual PBoolean IsListening() const;

	virtual PBoolean IsOpen () const;

	PBoolean CloseTransport() { return closeTransport; };

  protected:
	 PString GKid;

	 PMutex connectionsMutex;
	 PMutex WriteMutex;
	 PMutex IntMutex;
	 PMutex shutdownMutex;
	 PTimeInterval ReadTimeOut;
	 PSyncPoint ReadMutex;

	 GNUGK_Feature * Feature;

	 PBoolean   isConnected;
	 PBoolean   remoteShutDown;
	 PBoolean	closeTransport;
	 
};



class GNUGK_Feature : public PObject  
{

	PCLASSINFO(GNUGK_Feature, PObject);

public:
	GNUGK_Feature(H323EndPoint & ep, 
		H323TransportAddress & remoteAddress, 
		PString gkid,
		WORD KeepAlive = 10
		);

	~GNUGK_Feature();

	PBoolean CreateNewTransport();

	PBoolean ReRegister(const PString & newid);

	PBoolean IsOpen() { return open; };

	static WORD keepalive;
	static GNUGKTransport * curtransport;
	static PBoolean connectionlost;
		
protected:	

	H323EndPoint & ep;
	H323TransportAddress address;
	PString GKid;
	PBoolean open;

};

class PNatMethod_GnuGk  : public PNatMethod
{
	PCLASSINFO(PNatMethod_GnuGk,PNatMethod);

public:

  /**@name Construction */
  //@{
	/** Default Contructor
	*/
	PNatMethod_GnuGk();

	/** Deconstructor
	*/
	~PNatMethod_GnuGk();
  //@}

  /**@name General Functions */
  //@{
   void AttachEndPoint(H323EndPoint * ep);

   virtual PBoolean GetExternalAddress(
      PIPSocket::Address & externalAddress, /// External address of router
      const PTimeInterval & maxAge = 1000   /// Maximum age for caching
	  );

  /**  CreateSocketPair
		Create the UDP Socket pair
  */
    virtual PBoolean CreateSocketPair(
      PUDPSocket * & socket1,
      PUDPSocket * & socket2,
      const PIPSocket::Address & binding = PIPSocket::GetDefaultIpAny()
    );

  /**  isAvailable.
		Returns whether the Nat Method is ready and available in
		assisting in NAT Traversal. The principal is function is
		to allow the EP to detect various methods and if a method
		is detected then this method is available for NAT traversal
		The Order of adding to the PNstStrategy determines which method
		is used
  */
   virtual bool IsAvailable(const PIPSocket::Address&) { return available; };

   void SetAvailable() { available = TRUE; };

   PBoolean OpenSocket(PUDPSocket & socket, PortInfo & portInfo) const;

   static PStringList GetNatMethodName() {  return PStringList("GNUGK"); };

   virtual PStringList GetName() const
            { return GetNatMethodName(); }
  //@}

protected:
	PBoolean available;

};

PWLIB_STATIC_LOAD_PLUGIN(GnuGk, PNatMethod);

class GNUGKUDPSocket : public PUDPSocket
{
  PCLASSINFO(GNUGKUDPSocket, PUDPSocket);
  public:
  /**@name Construction/Deconstructor */
  //@{
	/** create a UDP Socket Fully Nat Supported
		ready for H323plus to Call.
	*/
    GNUGKUDPSocket();

	/** Deconstructor to reallocate Socket and remove any exiting
		allocated NAT ports, 
	*/
	~GNUGKUDPSocket();

	virtual void SetSendAddress(
      const Address & address,    /// IP address to send packets.
      WORD port                   /// Port to send packets.
    );
   //@}

  protected:

	PIPSocket::Address Remote;

};

#endif // GNUGK_NAT

#endif // H323_GNUGK



