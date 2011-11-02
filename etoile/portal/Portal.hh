//
// ---------- header ----------------------------------------------------------
//
// project       etoile
//
// license       infinit
//
// author        julien.quintard   [mon oct 31 13:52:37 2011]
//

#ifndef ETOILE_PORTAL_PORTAL_HH
#define ETOILE_PORTAL_PORTAL_HH

//
// ---------- includes --------------------------------------------------------
//

#include <elle/Elle.hh>
#include <lune/Lune.hh>

#include <etoile/portal/Application.hh>

#include <elle/idiom/Close.hh>
# include <map>
#include <elle/idiom/Open.hh>

namespace etoile
{
  ///
  /// XXX
  ///
  namespace portal
  {

//
// ---------- classes ---------------------------------------------------------
//

    ///
    /// this class is responsible for managing the external applications
    /// connecting to Etoile and triggering operations.
    ///
    class Portal
    {
    public:
      //
      // types
      //
      typedef std::pair<elle::Door*, Application*>	Value;
      typedef std::map<elle::Door*, Application*>	Container;
      typedef Container::iterator			Iterator;
      typedef Container::const_iterator			Scoutor;

      //
      // static methods
      //
      static elle::Status	Initialize();
      static elle::Status	Clean();

      static elle::Status	Add(Application*);
      static elle::Status	Retrieve(elle::Door*,
					 Application*&);
      static elle::Status	Remove(Application*);

      static elle::Status	Show(const elle::Natural32 = 0);

      //
      // static callbacks
      //
      static elle::Status	Connection(elle::Door*);
      static elle::Status	Authenticate(const lune::Phrase&);

      //
      // static attributes
      //
      static elle::String	Line;
      static Container		Applications;
    };

  }
}

//
// ---------- includes --------------------------------------------------------
//

#include <etoile/portal/Manifest.hh>

#endif
