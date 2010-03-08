//
// ---------- header ----------------------------------------------------------
//
// project       elle
//
// license       infinit (c)
//
// file          /home/mycure/infinit/elle/test/network/slot/Node.cc
//
// created       julien quintard   [fri nov 27 22:04:36 2009]
// updated       julien quintard   [sun mar  7 23:39:32 2010]
//

//
// ---------- includes --------------------------------------------------------
//

#include <elle/test/network/slot/Node.hh>

namespace elle
{
  namespace test
  {

//
// ---------- methods ---------------------------------------------------------
//

    ///
    /// this method initializes the node.
    ///
    Status		Node::Setup(const String&		name,
				    const String&		host,
				    const Port			port)
    {
      enter();

      // set the attributes.
      this->name = name;
      this->host = host;
      this->port = port;

      leave();
    }

    ///
    /// this is the thread entry point.
    ///
    void		Node::run()
    {
      static Method<String>	callback(this, &Node::Handle);
      Host			local;
      Address			remote;

      enter();

      // create an host.
      if (local.Create(this->host) == StatusError)
	alert("unable to create an host");

      // create the slot.
      if (this->slot.Create() == StatusError)
	alert("unable to create the slot");

      std::cout << "[port] " << this->slot.port << std::endl;

      // register the probe message.
      if (Network::Register<TagProbe>(callback) == StatusError)
	alert("unable to register the probe message");

      // create a new timer.
      this->timer = new ::QTimer;

      // connect the timeout signal to the refresh slot.
      if (this->connect(this->timer, SIGNAL(timeout()),
			this, SLOT(Refresh())) == false)
	alert("unable to connect the timeout signal");

      // start the timer.
      this->timer->start(10000);

      // create an address.
      if (remote.Create(local, this->port) == StatusError)
	alert("unable to create a location");

      // probe the peer.
      if (this->slot.Send(remote,
			  Inputs<TagProbe>(this->name)) == StatusError)
	alert("unable to send the probe");

      // wait for events.
      this->exec();

      release();
    }

    ///
    /// this method adds a new neighbour.
    ///
    Status		Node::Add(const Address&		address,
				  const String&			name)
    {
      Node::Iterator	iterator;

      enter();

      // try to locate a previous entry.
      if (this->Locate(address, iterator) == StatusOk)
	{
	  // update the name.
	  (*iterator)->name = name;

	  // re-set the timer.
	  (*iterator)->timer.stop();
	  (*iterator)->timer.start(20000);
	}
      else
	{
	  Neighbour*	neighbour;

	  enter(instance(neighbour));

	  // create a new neighbour.
	  neighbour = new Neighbour;

	  // assign the attributes.
	  neighbour->node = this;

	  neighbour->address = address;
	  neighbour->name = name;

	  // connect the timeout.
	  if (neighbour->connect(&neighbour->timer, SIGNAL(timeout()),
				 neighbour, SLOT(Discard())) == false)
	    escape("unable to connect the timeout signal");

	  // start the timer.
	  neighbour->timer.start(20000);

	  // add the neighbour to the list.
	  this->container.push_back(neighbour);

	  // stop tracking.
	  waive(neighbour);

	  release();
	}

      leave();
    }

    ///
    /// this method removes a neighbour.
    ///
    Status		Node::Remove(const Address&		address)
    {
      Node::Iterator	iterator;

      enter();

      // try to locate a previous entry.
      if (this->Locate(address, iterator) == StatusError)
	escape("unable to locate this neighbour");

      // remove the element from the list.
      this->container.erase(iterator);

      leave();
    }

    ///
    /// this method updates a existing neighbour.
    ///
    Status		Node::Update(const Address&		address,
				     const String&		name)
    {
      Node::Iterator	iterator;

      enter();

      // try to locate a previous entry.
      if (this->Locate(address, iterator) == StatusError)
	escape("unable to locate this neighbour");

      // update the name.
      (*iterator)->name = name;

      // re-set the timer.
      (*iterator)->timer.stop();
      (*iterator)->timer.start(20000);

      leave();
    }

    ///
    /// this method locates a neighbour in the list.
    ///
    Status		Node::Locate(const Address&		address,
				     Node::Iterator&		iterator)
    {
      enter();

      // iterator over the container.
      for (iterator = this->container.begin();
	   iterator != this->container.end();
	   iterator++)
	{
	  // if the address is found, return.
	  if ((*iterator)->address == address)
	    leave();
	}

      escape("unable to locate the given neighbour");
    }

//
// ---------- callbacks -------------------------------------------------------
//

    ///
    /// this method handles probe packets.
    ///
    Status		Node::Handle(String&			name)
    {
      enter();

      // simply add the sender to the list of neighbours.
      if (this->Add(context->address, name) == StatusError)
	escape("unable to add the new neighbour");

      leave();
    }

//
// ---------- slots -----------------------------------------------------------
//

    ///
    /// this slot is called whenever the state needs refreshing.
    ///
    void		Node::Refresh()
    {
      //
      // first, display the current state.
      //
      Node::Scoutor	scoutor;

      enter();

      std::cout << "[State]" << std::endl;

      for (scoutor = this->container.begin();
	   scoutor != this->container.end();
	   scoutor++)
	{
	  std::cout << "  [Neighbour] " << (*scoutor)->name << std::endl;

	  if ((*scoutor)->address.Dump(4) == StatusError)
	    alert("unable to dump the neighbour's address");
	}

      //
      // initiate the refreshing process.
      //
      for (scoutor = this->container.begin();
	   scoutor != this->container.end();
	   scoutor++)
	{
	  // send a probe message.
	  if (this->slot.Send((*scoutor)->address,
			      Inputs<TagProbe>(this->name)) == StatusError)
	    alert("unable to send a probe");
	}
    }

    ///
    /// XXX
    ///
    void		Neighbour::Discard()
    {
      enter();

      // discard the current neighbour as it has not been refreshed in time.
      if (this->node->Remove(this->address) == StatusError)
	alert("unable to remove the current neighbour");

      release();
    }

  }
}
