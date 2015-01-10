/* Copyright (c) 2015, The Measurement Factory. */

#ifndef HTTP_PAXXER_ALL_H
#define HTTP_PAXXER_ALL_H

#include <HttpPaxxer/Paxxer.h>
#include <cstddef>
#include <stdint.h> /* TODO: <use cstdint> if available */

// This all-in-one file is a temporary hack to simplify initial development.
// TODO: Move Foo-related components to Foo.h.
// HttpPaxxer users should already use Foo.h instead of this All.h!
#ifndef I_AM_INCLUDED_BY_AN_HTTP_PAXXER_FILE
#error HttpPaxxer user #included All.h
#endif
#undef I_AM_INCLUDED_BY_AN_HTTP_PAXXER_FILE

#include <HttpPaxxer/Forward.h>


namespace HttpPaxxer {

// An API for a user-provided "data-storage class" such as
// std::string in STL, SBuf in Squid, or String in Web Polygraph.
// HTTP Paxxer never copies or splits the underlying data but
// may introduce its own simple Raw objects for on-stack data.
// TODO: Try to get rid if this without adding a Raw template 
// parameter to nearly all classes that deal with raw data.
class Raw {
	public:
		virtual ~Raw() {}

		virtual size_t size() const = 0;
		virtual const char *data() const = 0;
};


// An API for a user-provided "on-the-wire bytes writer"
// such as an TLS-encrypting and network-writing code.
class Writer {
	public:
		virtual ~Writer() {}

		/* Writer MUST write or copy/move/preserve everything.
         * Paxxer lacks storage except for temporary on-stack variables. */
		virtual void writeRaw(const Raw &raw) = 0;
};


// From Paxxer point of view, an Interpreter may change or dissappear
// at any time. However, if there is no Interpreter to deliver an event
// to, Paxxer will throw (TODO: throw what?).

// An API for a user-provided "stream events manager"
// such as server code that interprets and satisfies an HTTP request.
class StreamInterpreter {
	public:
		virtual ~StreamInterpreter() {}

		virtual void noteHeaderField(HeaderField, const bool eoh) = 0;
		virtual void noteBody(const Raw &raw, const bool eob) = 0;
		// TODO: add other stream-specific event notification methods
};


// An API for a user-provided "connection events manager"
// such as server code that responds to PINGs.
class ConnectionInterpreter {
	public:
		virtual ~ConnectionInterpreter() {}

		// TODO: add connection-specific event notification methods
};


// represents HTTP/2 stream
// sends HTTP message via Connection using HTTP/2 frames
class Stream {
	public:
		typedef enum { stIdle, stReservedLocal, stReservedRemote, stOpen,
			stHalfClosedLocal, stHalfClosedRemote, stClosed } State;

		typedef uint32_t Id;

	public:
		Stream(const Id anId, const weak_ptr<Connection> &aConnection);

		const State &state() const { return state_; }

		// either HEADERS or CONTINUATION frame, depending on the stream state
		void sendHeaders(const Headers &hdr, const bool eoh);

		void sendData(const Raw &data, const bool eof);
		void sendData(const Raw &data, const bool eof, const size_t padding);
		void end(); // send RST_STREAM

		// TODO: PRIORITY.

	public:
		/* a user may reset non-const members at will */
		shared_ptr<StreamInterpreter> interpreter; // gets stream-specific events
		const shared_ptr<Connection> connection;
		const Id id;

	private:
		State state_;
};

// represents HTTP/2 connection; creates Streams
class Connection {
	public:
		/* sending */

		shared_ptr<Stream> addStream(); // sends nothing
		// TODO: SETTINGS, PUSH_PROMISE, PING, GOAWAY, WINDOW_UPDATE.

		/* Methods for sending pre-built HTTP/2 frames.
		 * Do not use for Stream-specific frames! */
		void sendFrame(const FrameHeader &frame, const Raw &payload); // at once
		void beginFrame(const FrameHeader &frame); // expects frame.size payload
		void morePayload(const Raw &payload); // ends based on frame.size


		/* receiving */

		/* Methods to call after receiving raw decrypted connection data. They
         * may result in calls to Interpreters
         * ensure doneReceiving() returns correct value, and
         * return the number of bytes used.
         * The user should not consume unused bytes */
		size_t parseRaw(const Raw &data, const bool eof);
		size_t parseBytes(const char *bytes, const size_t size, const bool eof);

		// returns true if and only if no more data is needed
		bool doneReceiving() const { return doneReceiving_; }

	public:
		/* a user may reset non-const members at will */
		shared_ptr<ConnectionInterpreter> interpreter; // gets connection-wide events
		shared_ptr<Writer> writer; // sent bytes go here in "on the wire" format

	private:
		bool doneReceiving_; // whether 
};

} // namespace HttpPaxxer;

// XXX: What about trailers?

#endif
