//
// ---------- header ----------------------------------------------------------
//
// project       elle
//
// license       infinit
//
// author        Alexandre Bique   [mon oct  3 15:58:14 2011]
//

namespace elle
{
  namespace utility
  {
    /**
     * @brief converts an utf8 string to an utf16 string
     *
     * @param input valid input
     * @param input_size the size in byte of input, can be -1 if input is null
     * terminated
     * @param output the output destination, will be realloced
     * @param output_size the size of the outpout destination, non null
     */
    Status Utf8To16(const char * input,
                    size_t       input_size,
                    wchar_t **   output,
                    size_t *     output_size);

    /**
     * @brief converts an utf16 string to an utf8 string
     *
     * @param input valid input
     * @param input_size the size in byte of input, can be -1 if input is null
     * terminated
     * @param output the output destination, will be realloced
     * @param output_size the size of the outpout destination, non null
     */
    Status Utf16To8(const wchar_t * input,
                    size_t          input_size,
                    char **         output,
                    size_t *        output_size);
  }
}
