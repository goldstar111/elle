//
// ---------- header ----------------------------------------------------------
//
// project       elle
//
// license       infinit
//
// file          /home/mycure/infinit/elle/network/Slot.cc
//
// created       julien quintard   [wed feb  3 21:52:30 2010]
// updated       julien quintard   [sun mar  7 23:34:02 2010]
//

//
// ---------- includes --------------------------------------------------------
//

#include <elle/network/Slot.hh>

namespace elle
{
  namespace network
  {

//
// ---------- constructors & destructors --------------------------------------
//

    ///
    /// default constructor.
    ///
    Slot::Slot():
      Socket(Socket::TypeSlot),

      socket(NULL),
      port(0)
    {
    }

    ///
    /// the destructor releases the associated resources.
    ///
    Slot::~Slot()
    {
      // release the socket.
      if (this->socket != NULL)
	delete this->socket;
    }

//
// ---------- methods ---------------------------------------------------------
//

    ///
    /// XXX
    ///
    Status		Slot::Create()
    {
      enter();

      // allocate a new UDP socket.
      this->socket = new ::QUdpSocket;

      // bind the socket.
      if (this->socket->bind() == false)
	escape(this->socket->errorString().toStdString().c_str());

      // retrieve the port.
      this->port = this->socket->localPort();

      // connect the signals.
      if (this->connect(this->socket, SIGNAL(readyRead()),
			this, SLOT(Fetch())) == false)
	escape("unable to connect the signal");

      leave();
    }

    ///
    /// XXX
    ///
    Status		Slot::Create(const Port			port)
    {
      enter();

      // allocate a new UDP socket.
      this->socket = new ::QUdpSocket;
      this->port = port;

      // bind the socket to the port.
      if (this->socket->bind(this->port) == false)
	escape(this->socket->errorString().toStdString().c_str());

      // connect the signals.
      if (this->connect(this->socket, SIGNAL(readyRead()),
			this, SLOT(Fetch())) == false)
	escape("unable to connect the signal");

      leave();
    }

//
// ---------- dumpable --------------------------------------------------------
//

    ///
    /// XXX
    ///
    Status		Slot::Dump(const Natural32		margin) const
    {
      String		alignment(margin, ' ');
      String		shift(2, ' ');

      enter();

      std::cout << alignment << "[Slot]" << std::endl;

      // XXX hasPendingDatagrams, BindMode, SocketState, Valid,
      // XXX TransportProtocol, ...

      leave();
    }

//
// ---------- slots -----------------------------------------------------------
//

    ///
    /// this method fetches the data datagram(s) on the socket.
    ///
    /// note that since UDP datagrams are constrained by a maxium size,
    /// there is no need to buffer the fetched datagram hoping for the next
    /// one to complete it.
    ///
    /// therefore, whenever a datagram is fetched, the method tries to makes
    /// sense out of it. if unable, the fetched datagrams are simply dropped.
    ///
    /// indeed there is no way to extract a following datagram since if
    /// the first one is invalid since datagrams are variable in size.
    ///
    void		Slot::Fetch()
    {
      Raw		raw;
      Address		address;
      Natural32		offset;

      enter();

      //
      // read the pending datagrams in a raw.
      //
      {
	Natural32	size;

	// return if there is no pending datagrams.
	if (this->socket->hasPendingDatagrams() == false)
	  return;

	// retrieve the size of the pending packet.
	size = this->socket->pendingDatagramSize();

	// prepare the raw
	if (raw.Prepare(size) == StatusError)
	  alert("unable to prepare the raw");

	// set the address as being an IP address.
	if (address.host.Create(Host::TypeIP) == StatusError)
	  alert("unable to create an IP address");

	// read the datagram from the socket.
	if (this->socket->readDatagram((char*)raw.contents,
				       size,
				       &address.host.location,
				       &address.port) != size)
	  alert(this->socket->errorString().toStdString().c_str());

	// set the raw's size.
	raw.size = size;
      }

      // initialize the offset.
      offset = 0;

      //
      // try to extract a serie of packet from the received raw.
      //
      {
	Region		frame;
	Packet		packet;
	Context*	context;
	Header*		header;
	Data*		data;

	enter();

	// create the frame based on the previously extracted raw.
	if (frame.Wrap(raw.contents + offset,
		       raw.size - offset) == StatusError)
	  alert("unable to wrap a frame in the raw");

	// prepare the packet based on the frame.
	if (packet.Prepare(frame) == StatusError)
	  alert("unable to prepare the packet");

	// detach the frame from the packet so that the region is
	// not released once the packet is destroyed.
	if (packet.Detach() == StatusError)
	  alert("unable to detach the frame");

	// allocate the header.
	header = new Header;

	// extract the header.
	if (header->Extract(packet) == StatusError)
	  alert("unable to extract the header");

	// allocate the data.
	data = new Data;

	// extract the data.
	if (packet.Extract(*data) == StatusError)
	  alert("unable to extract the data");

	// allocate the context.
	context = new Context(this, address);

	// record this packet to the network manager.
	if (Network::Dispatch(context, header, data) == StatusError)
	  alert("unable to record the packet");

	// move to the next frame by setting the offset at the end of
	// the extracted frame.
	offset = offset + packet.offset;

	release();
      }

      release();
    }

  }
}
