/* Copyright (c) 2015, The Measurement Factory. */

#ifndef HTTP_PAXXER_HEADER_COMPRESSION_H
#define HTTP_PAXXER_HEADER_COMPRESSION_H

#include <HttpPaxxer/Paxxer.h>

namespace HttpPaxxer {

/* HTTP/2 Header Compression (HPACK) */

typedef void* TBD_t; // XXX: the exact type(s) are to-be-determined
typedef TBD_t StaticHeaders; // HPACK: "Static Table" defined in Appendix B.
typedef TBD_t DynamicHeaders; // HPACK: [Dynamic] Header Table.

// converts header fields into bytes on the wire while maintaining their index
class Compressor {
	public:
		// HPACK: Literal Header Field with Incremental Indexing
		void sendAndIndexField(const std::string &name, const std::string &value);

	private:
		void noteBytes(const char *buf, size_type size);

		DynamicHeaders dynamicHeaders;
};

// creates header fields from bytes on the wire while maintaining their index
class Decompressor {
	public:
		// HPACK: Literal Header Field with Incremental Indexing
		void loadBytes(const char *buf, size_type size);

	private:
		void noteField(const std::string &name, const std::string &value);

		DynamicHeaders dynamicHeaders;
};

} // namespace HttpPaxxer;

#endif /* HTTP_PAXXER_HEADER_COMPRESSION_H */
