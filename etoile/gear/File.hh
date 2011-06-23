//
// ---------- header ----------------------------------------------------------
//
// project       etoile
//
// license       infinit
//
// file          /home/mycure/infinit/etoile/gear/File.hh
//
// created       julien quintard   [fri aug 14 23:13:51 2009]
// updated       julien quintard   [thu jun 23 16:20:09 2011]
//

#ifndef ETOILE_GEAR_FILE_HH
#define ETOILE_GEAR_FILE_HH

//
// ---------- includes --------------------------------------------------------
//

#include <elle/Elle.hh>
#include <nucleus/Nucleus.hh>

#include <etoile/gear/Object.hh>
#include <etoile/gear/Nature.hh>

namespace etoile
{
  namespace gear
  {

//
// ---------- classes ---------------------------------------------------------
//

    ///
    /// this context represents a file object as it embeds
    /// a data along with inherited object-related stuff.
    ///
    class File:
      public Object
    {
    public:
      //
      // types
      //
      typedef nucleus::Data			C;

      //
      // constructors & destructors
      //
      File();
      ~File();

      //
      // interfaces
      //

      // XXX object

      // dumpable
      elle::Status	Dump(const elle::Natural32 = 0) const;

      // XXX archivable

      //
      // attributes
      //
      nucleus::Contents<C>*	contents;
    };

  }
}

#endif
