//
// ---------- header ----------------------------------------------------------
//
// project       elle
//
// license       infinit
//
// file          /home/mycure/infinit/elle/test/network/gate/Server.hh
//
// created       julien quintard   [fri nov 27 22:03:15 2009]
// updated       julien quintard   [thu aug 25 11:39:55 2011]
//

#ifndef ELLE_TEST_NETWORK_GATE_SERVER_HH
#define ELLE_TEST_NETWORK_GATE_SERVER_HH

//
// ---------- includes --------------------------------------------------------
//

#include <elle/Elle.hh>

#include <elle/test/network/gate/Manifest.hh>

namespace elle
{
  namespace test
  {

//
// ---------- classes ---------------------------------------------------------
//

    class Server
    {
    public:
      //
      // constructors & destructors
      //
      Server();
      ~Server();

      //
      // methods
      //
      Status		Setup(const String&);
      Status		Run();

      //
      // callbacks
      //
      Status		Connection(Gate*&);

      //
      // attributes
      //
      Point		point;
      Gate*		gate;
    };

  }
}

#endif
