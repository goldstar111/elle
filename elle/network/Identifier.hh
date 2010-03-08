//
// ---------- header ----------------------------------------------------------
//
// project       elle
//
// license       infinit
//
// file          /home/mycure/infinit/elle/network/Identifier.hh
//
// created       julien quintard   [wed mar  3 13:37:54 2010]
// updated       julien quintard   [mon mar  8 23:09:45 2010]
//

#ifndef ELLE_NETWORK_IDENTIFIER_HH
#define ELLE_NETWORK_IDENTIFIER_HH

//
// ---------- includes --------------------------------------------------------
//

#include <elle/core/Core.hh>
#include <elle/misc/Misc.hh>
#include <elle/archive/Archive.hh>

#include <openssl/rand.h>
#include <openssl/err.h>

namespace elle
{
  using namespace core;
  using namespace misc;
  using namespace archive;

  namespace network
  {

//
// ---------- types -----------------------------------------------------------
//

    ///
    /// XXX
    ///
    class Identifier:
      public Entity,
      public Dumpable, public Archivable
    {
    public:
      //
      // constants
      //
      static const Identifier	Null;

      //
      // constructors & destructors
      //
      Identifier();

      //
      // methods
      //
      Status		Generate();

      //
      // interfaces
      //

      // entity
      embed(Entity, Identifier);
      Boolean		operator==(const Identifier&) const;

      // archivable
      Status		Serialize(Archive&) const;
      Status		Extract(Archive&);

      // dumpable
      Status		Dump(const Natural32 = 0) const;

      //
      // attributes
      //
      Natural32		value;
    };

//
// ---------- operators -------------------------------------------------------
//

    Boolean		operator<(const Identifier&,
				  const Identifier&);

  }
}

#endif
