//
// ---------- header ----------------------------------------------------------
//
// project       elle
//
// license       GPL
//
// file          /home/mycure/infinit/elle/crypto/PrivateKey.cc
//
// created       julien quintard   [tue oct 30 10:07:31 2007]
// updated       julien quintard   [wed mar 11 15:56:15 2009]
//

//
// ---------- includes --------------------------------------------------------
//

#include <elle/crypto/PrivateKey.hh>

namespace elle
{
  using namespace io;
  using namespace core;
  using namespace misc;
  using namespace archive;

  namespace crypto
  {

//
// ---------- definitions -----------------------------------------------------
//

    ///
    /// the class name.
    ///
    const String		PrivateKey::Class = "PrivateKey";

//
// ---------- methods ---------------------------------------------------------
//

    ///
    /// this method constructs a valid private key.
    ///
    Status		PrivateKey::Create(const ::EVP_PKEY*	key)
    {
      if (this->Create(::BN_dup(key->pkey.rsa->n),
		       ::BN_dup(key->pkey.rsa->d),
		       ::BN_dup(key->pkey.rsa->p),
		       ::BN_dup(key->pkey.rsa->q)) == StatusError)
	escape("unable to create the private key");

      leave();
    }

    ///
    /// this method constructs a valid private key given the proper
    /// numbers.
    ///
    Status		PrivateKey::Create(Large*		n,
					   Large*		d,
					   Large*		p,
					   Large*		q)
    {
      ::RSA*		rsa;

      //
      // key
      //

      // initialise the private key structure.
      this->key = ::EVP_PKEY_new();

      // create the RSA structure.
      if ((rsa = ::RSA_new()) == NULL)
	escape(::ERR_error_string(ERR_get_error(), NULL));

      // duplicate the big numbers relevant to the private key.
      rsa->n = n;
      rsa->d = d;
      rsa->p = p;
      rsa->q = q;

      // set the rsa structure into the private key.
      if (::EVP_PKEY_set1_RSA(this->key, rsa) <= 0)
	escape(::ERR_error_string(ERR_get_error(), NULL));

      //
      // contexts
      //

      // create, initialize and configure the decrypt context.
      if ((this->contexts.decrypt = ::EVP_PKEY_CTX_new(this->key, NULL)) ==
	  NULL)
	escape(::ERR_error_string(ERR_get_error(), NULL));

      if (::EVP_PKEY_decrypt_init(this->contexts.decrypt) <= 0)
        escape(::ERR_error_string(ERR_get_error(), NULL));

      if (::EVP_PKEY_CTX_ctrl(this->contexts.decrypt,
			      EVP_PKEY_RSA,
			      -1,
			      EVP_PKEY_CTRL_RSA_PADDING,
			      RSA_PKCS1_OAEP_PADDING,
			      NULL) <= 0)
	escape(::ERR_error_string(ERR_get_error(), NULL));

      // create and initialize a sign context.
      if ((this->contexts.sign = ::EVP_PKEY_CTX_new(this->key, NULL)) == NULL)
	escape(::ERR_error_string(ERR_get_error(), NULL));

      if (::EVP_PKEY_sign_init(this->contexts.sign) <= 0)
	escape(::ERR_error_string(ERR_get_error(), NULL));

      leave();
    }

    ///
    /// this method decrypts a serialized object and extracts it
    /// to return a pretty newly created object.
    ///
    Status		PrivateKey::Decrypt(const Code&		code,
					    Archivable&		object) const
    {
      Archive		archive;
      Clear		clear;

      // decrypt the code.
      if (this->Decrypt(code, clear) == StatusError)
	escape("unable to decrypt the code");

      // wrap the clear into an archive.
      if (archive.Prepare(clear) == StatusError)
	escape("unable to prepare the archive");

      // detach the data so that not both the clear and archive
      // release the data.
      if (archive.Detach() == StatusError)
	escape("unable to detach the archive's data");

      // extract the object.
      if (archive.Extract(object) == StatusError)
	escape("unable to extract the object");

      leave();
    }

    ///
    /// this method decrypts a plain text which should actually be
    /// an archive containing both a secret key and some data.
    ///
    /// this method starts by (i) extracting the key and data
    /// in their encrypted forms (ii) decrypt the symmetric key
    /// with the private key and (iii) decipher the data with the
    /// symmetric key.
    ///
    Status		PrivateKey::Decrypt(const Code&		code,
					    Clear&		clear) const
    {
      SecretKey		secret;

      Archive		archive;
      Code		key;
      Cipher		data;

      // (i)
      {
	// wrap the code into an archive.
	if (archive.Prepare(code.region) == StatusError)
	  escape("unable to prepare the archive");

	// detach the data from the archive so that the archive
	// does not release the memory belonging to the code.
	if (archive.Detach() == StatusError)
	  escape("unable to detach the ownership from the archive");

	// extract the secret key, in its encrypted form.
	if (archive.Extract(key) == StatusError)
	  escape("unable to extract the asymetrically-encrypted secret key");

	// extract the data, in its encrypted form as well, obviously!
	if (archive.Extract(data) == StatusError)
	  escape("unable to extract the symetrically-encrypted data");
      }

      // (ii)
      {
	Archive		archive;
	Region		clear;
	size_t		size;

	// compute the size of the decrypted portion to come.
	if (::EVP_PKEY_decrypt(this->contexts.decrypt,
			       NULL,
			       &size,
			       (const unsigned char*)key.region.contents,
			       key.region.size) <= 0)
	escape(::ERR_error_string(ERR_get_error(), NULL));

	// allocate the required memory for the region object.
	if (clear.Prepare(size) == StatusError)
	  escape("unable to allocate the required memory");

	// perform the decrypt operation.
	if (::EVP_PKEY_decrypt(this->contexts.decrypt,
			       (unsigned char*)clear.contents,
			       &size,
			       (const unsigned char*)key.region.contents,
			       key.region.size) <= 0)
	  escape(::ERR_error_string(ERR_get_error(), NULL));

	// set the region size.
	clear.size = size;

	// detach the data from the region so that the data
	// is not release twice by both 'region' and 'archive'.
	if (clear.Detach() == StatusError)
	  escape("unable to detach the data from the region");

	// prepare the archive.
	if (archive.Prepare(clear) == StatusError)
	  escape("unable to prepare the archive");

	// extract the secret key.
	if (archive.Extract(secret) == StatusError)
	  escape("unable to extract the secret key from the archive");
      }

      // (iii)
      {
	// finally, decrypt the data with the secret key.
	if (secret.Decrypt(data, clear) == StatusError)
	  escape("unable to decrypt the data with the secret key");
      }

      leave();
    }

    ///
    /// this method signs an Archivable object by serializing it
    /// before applying the signature process.
    ///
    Status		PrivateKey::Sign(const Archivable&	object,
					 Signature&		signature)
      const
    {
      Archive		archive;
      Digest		digest;

      // create th archive.
      if (archive.Create() == StatusError)
	escape("unable to create the archive");

      // serialize the object.
      if (archive.Serialize(object) == StatusError)
	escape("unable to serialize the object");

      // re-launch the Sign() method for a digest.
      if (this->Sign(archive, signature) == StatusError)
	escape("unable to sign the object's archive");

      leave();
    }

    ///
    /// this method signs a plain text.
    ///
    inline Status	PrivateKey::Sign(const Plain&		plain,
					 Signature&		signature)
      const
    {
      Digest		digest;
      size_t		size;

      // compute the plain's digest.
      if (OneWay::Hash(plain, digest) == StatusError)
	escape("unable to hash the plain");

      // sign the portion.
      if (::EVP_PKEY_sign(this->contexts.sign,
			  NULL,
			  &size,
			  (const unsigned char*)digest.region.contents,
			  digest.region.size) <= 0)
	escape(::ERR_error_string(ERR_get_error(), NULL));

      // expand the signature so it can receive the upcoming portion.
      if (signature.region.Expand(size) == StatusError)
	escape("unable to expand the signature");

      // actually sign the portion.
      if (::EVP_PKEY_sign(this->contexts.sign,
			  (unsigned char*)signature.region.contents,
			  &size,
			  (const unsigned char*)digest.region.contents,
			  digest.region.size) <= 0)
	escape(::ERR_error_string(ERR_get_error(), NULL));

      // increase the code pointer.
      signature.region.size += size;

      leave();
    }

//
// ---------- object ----------------------------------------------------------
//

    ///
    /// this method initializes the object.
    ///
    Status		PrivateKey::New(PrivateKey&		k)
    {
      k.key = NULL;

      k.contexts.decrypt = NULL;
      k.contexts.sign = NULL;

      leave();
    }

    ///
    /// this method releases the resources.
    ///
    Status		PrivateKey::Delete(PrivateKey&		k)
    {
      // release the resources.
      if (k.key != NULL)
	::EVP_PKEY_free(k.key);

      if (k.contexts.decrypt != NULL)
	::EVP_PKEY_CTX_free(k.contexts.decrypt);

      if (k.contexts.sign != NULL)
	::EVP_PKEY_CTX_free(k.contexts.sign);

      leave();
    }

    ///
    /// assign the publickey.
    ///
    PrivateKey&		PrivateKey::operator=(const PrivateKey&	element)
    {
      // self-check.
      if (this == &element)
	return (*this);

      // reinitialize the object.
      if ((PrivateKey::Delete(*this) == StatusError) ||
	  (PrivateKey::New(*this) == StatusError))
	yield("unable to reinitialize the object", *this);

      // re-create the private key by duplicating the internal numbers.
      if (this->Create(element.key) == StatusError)
	yield("unable to duplicate the private key", *this);

      return (*this);
    }

    ///
    /// this method check if two objects match.
    ///
    Boolean		PrivateKey::operator==(const PrivateKey& element)
    {
      // compare the internal numbers.
      if ((::BN_cmp(this->key->pkey.rsa->n, element.key->pkey.rsa->n) != 0) ||
	  (::BN_cmp(this->key->pkey.rsa->d, element.key->pkey.rsa->d) != 0) ||
	  (::BN_cmp(this->key->pkey.rsa->p, element.key->pkey.rsa->p) != 0) ||
	  (::BN_cmp(this->key->pkey.rsa->q, element.key->pkey.rsa->q) != 0))
	false();

      true();
    }

    ///
    /// this method checks if two objects dis-match.
    ///
    Boolean		PrivateKey::operator!=(const PrivateKey& element)
    {
      return (!(*this == element));
    }

//
// ---------- dumpable --------------------------------------------------------
//

    ///
    /// this method dumps the private key internals.
    ///
    Status		PrivateKey::Dump(const Natural32	margin)
    {
      String		alignment(margin, ' ');
      String		shift(2, ' ');

      std::cout << alignment << "[PrivateKey]" << std::endl;
      std::cout << alignment << shift << "[n] " << *this->key->pkey.rsa->n << std::endl;
      std::cout << alignment << shift << "[d] " << *this->key->pkey.rsa->d << std::endl;
      std::cout << alignment << shift << "[p] " << *this->key->pkey.rsa->p << std::endl;
      std::cout << alignment << shift << "[q] " << *this->key->pkey.rsa->q << std::endl;

      leave();
    }

//
// ---------- archivable ------------------------------------------------------
//

    ///
    /// this method serializes a private key object.
    ///
    Status		PrivateKey::Serialize(Archive&		archive) const
    {
      Archive		ar;

      // prepare the object archive.
      if (ar.Create() == StatusError)
	escape("unable to prepare the object archive");

      // serialize the class name.
      if (ar.Serialize(PrivateKey::Class) == StatusError)
	escape("unable to serialize the class name");

      // serialize the internal numbers.
      if (ar.Serialize(*this->key->pkey.rsa->n) == StatusError)
	escape("unable to serialize 'n'");

      if (ar.Serialize(*this->key->pkey.rsa->d) == StatusError)
	escape("unable to serialize 'd'");

      if (ar.Serialize(*this->key->pkey.rsa->p) == StatusError)
	escape("unable to serialize 'p'");

      if (ar.Serialize(*this->key->pkey.rsa->q) == StatusError)
	escape("unable to serialize 'q'");

      // record the object archive into the given archive.
      if (archive.Serialize(ar) == StatusError)
	escape("unable to serialize the object archive");

      leave();
    }

    ///
    /// this method extract a private key from the given archive.
    ///
    Status		PrivateKey::Extract(Archive&		archive)
    {
      Archive		ar;
      String		name;
      Large*		n;
      Large*		d;
      Large*		p;
      Large*		q;

      // extract the private key archive object.
      if (archive.Extract(ar) == StatusError)
	escape("unable to extract the private key archive object");

      // extract the name.
      if (ar.Extract(name) == StatusError)
	escape("unable to extract the class name");

      // check the name.
      if (PrivateKey::Class != name)
	escape("wrong class name in the extract object");

      // allocate the big numbers.
      if ((n = ::BN_new()) == NULL)
	escape(::ERR_error_string(ERR_get_error(), NULL));

      if ((d = ::BN_new()) == NULL)
	escape(::ERR_error_string(ERR_get_error(), NULL));

      if ((p = ::BN_new()) == NULL)
	escape(::ERR_error_string(ERR_get_error(), NULL));

      if ((q = ::BN_new()) == NULL)
	escape(::ERR_error_string(ERR_get_error(), NULL));

      // extract the numbers.
      if (ar.Extract(*n) == StatusError)
	escape("unable to extract 'n'");

      if (ar.Extract(*d) == StatusError)
	escape("unable to extract 'd'");

      if (ar.Extract(*p) == StatusError)
	escape("unable to extract 'p'");

      if (ar.Extract(*q) == StatusError)
	escape("unable to extract 'q'");

      // create the EVP_PKEY object from the extract numbers.
      if (this->Create(n, d, p, q) == StatusError)
	escape("unable to create the private key from the archive");

      leave();
    }

  }
}

//
// ---------- operators -------------------------------------------------------
//

namespace std
{

  ///
  /// this function overloads the << operator and computes a fingerprint
  /// of the key in its serialised form.
  ///
  std::ostream&		operator<<(std::ostream&		stream,
				   const elle::crypto::PrivateKey& key)
  {
    elle::archive::Archive	archive;
    elle::crypto::Digest	digest;

    // prepare the archive.
    if (archive.Create() == elle::misc::StatusError)
      yield("unable to create the archive", stream);

    // serialize the secret key.
    if (key.Serialize(archive) == elle::misc::StatusError)
      yield("unable to serialize the private key", stream);

    // digest the archive.
    if (elle::crypto::OneWay::Hash(archive, digest) == elle::misc::StatusError)
      yield("unable to hash the private key's archive", stream);

    // put the fingerprint into the stream.
    stream << digest;

    return (stream);
  }

}
