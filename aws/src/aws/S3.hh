#ifndef AWS_S3_S3_HH
# define AWS_S3_S3_HH

# include <map>
# include <vector>

#include <boost/property_tree/ptree.hpp>

# include <elle/Printable.hh>
# include <elle/Buffer.hh>
# include <elle/attribute.hh>

# include <aws/CanonicalRequest.hh>
# include <aws/Credentials.hh>
# include <aws/StringToSign.hh>

namespace aws
{
  class S3:
    public elle::Printable
  {
    /*------.
    | Types |
    `------*/
  public:
    typedef uint64_t FileSize;
    typedef std::vector<std::pair<std::string, FileSize>> List;

    /*-------------.
    | Construction |
    `-------------*/
  public:
    /// Create a new S3 handler.
    /// This requires a bucket name, remote folder and set of credentials.
    /// Use of this should be optimised based on S3 pricing:
    /// http://aws.amazon.com/s3/pricing/
    S3(std::string const& bucket_name,
       std::string const& remote_folder,
       Credentials const& credentials);
    /// Version taking a function able to refresh credentials
    S3(std::string const& bucket_name,
       std::string const& remote_folder,
       std::function<Credentials()> query_credentials);
    /*-----------.
    | Operations |
    `-----------*/
  public:
    /// Put a block (as a file) into the remote folder.
    /// This function does a single PUT call, so block size should be something
    /// reasonable for an HTTP request.
    /// @return the object ETag.
    std::string
    put_object(elle::ConstWeakBuffer const& object,
               std::string const& object_name,
               RequestQuery const& query = RequestQuery(),
               bool ommit_redundancy = false);

    /// Returns a list of all files names and their respective sizes inside the
    /// remote folder.
    /// This is limited to 1000 results but a starting offset (marker) can be
    /// used if more are required. It is important to note that the files are
    /// listed in alphabetical order.
    std::vector<std::pair<std::string, FileSize>>
    list_remote_folder(std::string const& marker = "");
    /// List the full folder content.
    std::vector<std::pair<std::string, FileSize>>
    list_remote_folder_full();
    /// Fetch an object from the remote folder.
    /// The fetch is done in a single GET.
    elle::Buffer
    get_object(std::string const& object_name);

    /// Delete an object in the remote folder.
    /// The folder itself can be deleted only once it is empty. This can be done
    /// by setting the object_name to an empty string.
    void
    delete_object(std::string const& object_name,
      RequestQuery const& query = RequestQuery());
    /// Delete folder and all its content
    void
    delete_folder();

    /// Initialize multipart upload for given object
    /// @return an upload key needed by further operations
    std::string
    multipart_initialize(std::string const& object_name,
      std::string const& mime_type = "binary/octet-stream");

    typedef std::pair<int, std::string> MultiPartChunk;
    /// Upload one part of a multipart upload
    /// @return id for chunk information that needs to be passed to multipart_finalize
    std::string
    multipart_upload(std::string const& object_name,
                     std::string const& upload_key,
                     elle::ConstWeakBuffer const& object,
                     int chunk);

    /// Finalize a multipart by joining all parts together
    void
    multipart_finalize(std::string const& object_name,
                       std::string const& upload_key,
                       std::vector<MultiPartChunk> const& chunks);

    /// Abort a multipart transfer, delete all chunks
    void
    multipart_abort(std::string const& object_name,
                    std::string const& upload_key);

    std::vector<MultiPartChunk>
    multipart_list(std::string const& object_name,
                   std::string const& upload_key);

    /*-----------.
    | Attributes |
    `-----------*/
  private:
    ELLE_ATTRIBUTE(std::string, bucket_name);
    ELLE_ATTRIBUTE(std::string, remote_folder);
    ELLE_ATTRIBUTE(Credentials, credentials);
    ELLE_ATTRIBUTE(std::string, host_name);
    ELLE_ATTRIBUTE(std::function<Credentials()>, query_credentials);

    /*--------.
    | Helpers |
    `--------*/
  private:
    std::string
    _md5_digest(elle::ConstWeakBuffer const& buffer);

    std::string
    _sha256_hexdigest(elle::ConstWeakBuffer const& buffer);

    std::string
    _amz_date(RequestTime const& request_time);

    std::vector<std::string>
    _signed_headers(RequestHeaders const& headers);

    StringToSign
    _make_string_to_sign(RequestTime const& request_time,
                         std::string const& canonical_request_sha256);

    std::vector<std::pair<std::string, FileSize>>
    _parse_list_xml(std::istream& stream);

    reactor::http::Request::Configuration
    _initialize_request(RequestTime request_time,
                        CanonicalRequest const& canonical_request,
                        const RequestHeaders& initial_headers,
                        boost::posix_time::time_duration timeout);

    /// Check return code and throw appropriate exception if error
    /// ELLE_WARN the request response in case of error
    /// Dumps it if dump_response is true, which eats the stream content.
    void
    _check_request_status(reactor::http::Request& request,
                          std::string const& operation,
                          std::string const& object,
                          bool dump_response = false);

    // build and emit request, retries in case of credentials expiry
    std::unique_ptr<reactor::http::Request>
    _build_send_request(
      std::string const& url,
      reactor::http::Method method,
      RequestQuery const& query = RequestQuery(),
      RequestHeaders const& extra_headers = RequestHeaders(),
      boost::posix_time::time_duration timeout = 30_sec,
      std::string const& content_type = "application/json",
      elle::ConstWeakBuffer const& payload = elle::ConstWeakBuffer());

    /*----------.
    | Printable |
    `----------*/
  public:
    virtual
    void
    print(std::ostream& stream) const;
  };
}

#endif
