/* Copyright (c) 2015, The Measurement Factory. */

#include <iostream>
#include <cstring>
#include "HeaderFieldPaxxer.h"

namespace HttpPaxxer {

// TODO: Find a way for users to configure this type?
typedef HpackEncoderHeaderFields<
	std::deque<RecentHeaderField>,
	std::map<RecentHeaderFieldRef, Index> > StaticFieldStore;
// TODO: Make this a user-defined type.
typedef HpackHeaderFields<StaticFieldStore> StaticHeaders;
static StaticHeaders *StaticHeaders_;

static
bool operator <(const RecentHeaderField &f1, const RecentHeaderField &f2) {
	if (const int byName = f1.name.compare(f2.name))
		return byName < 0;
	else
		return f1.value < f2.value;
}

static inline
bool operator <(const RecentHeaderFieldRef f1, const RecentHeaderFieldRef f2) {
	return f1.field() < f2.field();
}

} // namespace HttpPaxxer


void HttpPaxxer::HeaderFieldPaxxerInit() {
	if (!StaticHeaders_) {
		StaticHeaders_ = new StaticHeaders;

		const int staticTableSize = 61;
		const RecentHeaderField staticTable[staticTableSize] = {
			{":authority",                  ""}, // 1
			{":method",                     "GET"}, // 2
			{":method",                     "POST"}, // 3
			{":path",                       "/"}, // 4
			{":path",                       "/index.html"}, // 5
			{":scheme",                     "http"}, // 6
			{":scheme",                     "https"}, // 7
			{":status",                     "200"}, // 8
			{":status",                     "204"}, // 9
			{":status",                     "206"}, // 10
			{":status",                     "304"}, // 11
			{":status",                     "400"}, // 12
			{":status",                     "404"}, // 13
			{":status",                     "500"}, // 14
			{"accept-charset",              ""}, // 15
			{"accept-encoding",             "gzip, deflate"}, // 16
			{"accept-language",             ""}, // 17
			{"accept-ranges",               ""}, // 18
			{"accept",                      ""}, // 19
			{"access-control-allow-origin", ""}, // 20
			{"age",                         ""}, // 21
			{"allow",                       ""}, // 22
			{"authorization",               ""}, // 23
			{"cache-control",               ""}, // 24
			{"content-disposition",         ""}, // 25
			{"content-encoding",            ""}, // 26
			{"content-language",            ""}, // 27
			{"content-length",              ""}, // 28
			{"content-location",            ""}, // 29
			{"content-range",               ""}, // 30
			{"content-type",                ""}, // 31
			{"cookie",                      ""}, // 32
			{"date",                        ""}, // 33
			{"etag",                        ""}, // 34
			{"expect",                      ""}, // 35
			{"expires",                     ""}, // 36
			{"from",                        ""}, // 37
			{"host",                        ""}, // 38
			{"if-match",                    ""}, // 39
			{"if-modified-since",           ""}, // 40
			{"if-none-match",               ""}, // 41
			{"if-range",                    ""}, // 42
			{"if-unmodified-since",         ""}, // 43
			{"last-modified",               ""}, // 44
			{"link",                        ""}, // 45
			{"location",                    ""}, // 46
			{"max-forwards",                ""}, // 47
			{"proxy-authenticate",          ""}, // 48
			{"proxy-authorization",         ""}, // 49
			{"range",                       ""}, // 50
			{"referer",                     ""}, // 51
			{"refresh",                     ""}, // 52
			{"retry-after",                 ""}, // 53
			{"server",                      ""}, // 54
			{"set-cookie",                  ""}, // 55
			{"strict-transport-security",   ""}, // 56
			{"transfer-encoding",           ""}, // 57
			{"user-agent",                  ""}, // 58
			{"vary",                        ""}, // 59
			{"via",                         ""}, // 60
			{"www-authenticate",            ""} // 61
		};

		for (int i = staticTableSize -1; i >= 0; --i)
			StaticHeaders_->push(staticTable[i]);
	}
}

void HttpPaxxer::HeaderFieldPaxxerClean() {
	delete StaticHeaders_;
	StaticHeaders_ = 0;
}

/* HeaderFieldParser */

size_t HttpPaxxer::HeaderFieldParser::loadBytes(const char *buf, size_t size) {
	RecentHeaderField field;
	while (const long n = parseField(buf, size, field)) {
		buf += n;
		size -= n;
		noteField(field);
	}

	return size;
}

void HttpPaxxer::HeaderFieldParser::noteField(const RecentHeaderField &field) {
	std::cout << field.name << ": " << field.value << std::endl;
}

long HttpPaxxer::HeaderFieldParser::parseField(const char *buf, size_t size, RecentHeaderField &field) {
	if (!size)
		return 0;

	const unsigned char cmd = buf[0];
	if (cmd & 0x80) { // index for name:value
		uint64_t index;
		const long indexSize = parseInteger(index, buf, size, 7);
		if (indexSize)
			findField(index, field);
		return indexSize;
	}

	if ((cmd & 0xE0) == 0x20) { // dynamic table size update
		// TODO: implement
		throw std::string("dynamic table size updates are not supported");
	}

	int prefixSize, prefix;
	bool insert;
	if (cmd & 0x40) {
		prefixSize = 6;
		prefix = cmd & 0x3F;
		insert = true;
	}
	else {
		prefixSize = 4;
		prefix = cmd & 0xF;
		insert = false;
	}

	long byteCount = 0;

	if (prefix) { // name index
		uint64_t index;
		const long indexSize = parseInteger(index, buf, size, prefixSize);
		if (!indexSize)
			return 0;
		byteCount += indexSize;
		buf += indexSize;
		size -= indexSize;
		findField(index, field);
	} else { // name literal
		const long strSize = parseString(buf + 1, size - 1, field.name);
		if (!strSize)
			return 0;
		byteCount += strSize + 1;
		buf += strSize + 1;
		size -= strSize + 1;
	}

	// value literal
	const long strSize = parseString(buf, size, field.value);
	if (!strSize)
		return 0;
	byteCount += strSize;

	if (insert)
		dynamicHeaders.push(field);

	return byteCount;
}

long HttpPaxxer::HeaderFieldParser::parseString(const char *buf, size_t size, std::string &str) {
	if (!size)
		return 0;

	long numSize;
	const bool huffman = buf[0] & 0x80;
	uint64_t stringSize = 0;
	if ((numSize = parseInteger(stringSize, buf, size, 7))) {
		buf += numSize;
		size -= numSize;
	}
	if (!numSize || stringSize > size)
		return 0;

	if (huffman) // TODO: Use an external Huffman code parser instead.
		throw std::string("Huffman encoding is not supported"); // TODO
	else
		str.assign(buf, stringSize);

	return numSize + stringSize;
}

// TODO: Use an external integer parser instead.
long HttpPaxxer::HeaderFieldParser::parseInteger(uint64_t &value, const char *buf, const size_t size, const int prefixSize) {
	if (!size)
		throw std::string("lack of input is not supported");

	static const unsigned char prefMask[9] = {0, 1, 3, 7, 0xf, 0x1f, 0x3f, 0x7f, 0xff};
	const unsigned char prefix = buf[0] & prefMask[prefixSize];
	if (prefix == prefMask[prefixSize])
		throw std::string("long numbers are not supported"); // TODO

	value = prefix;
	return 1;
}

void HttpPaxxer::HeaderFieldParser::findField(size_t index, RecentHeaderField &field) const {
	if (!index--)
		throw std::string("prohibited zero index");

	const size_t staticTableSize = StaticHeaders_->store().size();
	if (index < staticTableSize)
		field = StaticHeaders_->store().at(index);
	else
		field = dynamicHeaders.store().at(index - staticTableSize);
}


/* HeaderFieldPacker */

void HttpPaxxer::HeaderFieldPacker::sendAndIndexField(const RecentHeaderField &field) {
	theBuf.clear();

	// Conflict resolution among static/dynamic and full/partial matches:
	// A full match is better than a partial match.
	// A static match is better than a dynamic match.
	// A full dynamic match is better than a partial static match.
	StaticFieldStore::size_type statIndex = 0;
	const StaticFieldStore::MatchKind statMatch = // check the most likely to match map first
		StaticHeaders_->store().find(field, statIndex);
	if (statMatch == StaticFieldStore::matchFull) {
		// Ideal case: Statically Indexed Header Field Representation.
		packField(statIndex + 1);
		return;
	}

	const size_t staticTableSize = StaticHeaders_->store().size();
	FieldStore::size_type dynIndex = 0;
	const FieldStore::MatchKind dynMatch = // must check now; could be a dynamic full match
		dynamicHeaders.store().find(field, dynIndex);
	if (dynMatch == FieldStore::matchFull) {
		// Dynamically Indexed Header Field Representation.
		packField(dynIndex + staticTableSize + 1);
		return;
	}

	if (statMatch == StaticFieldStore::matchPartial) {
		// Statically Indexed Name + Literal Header Field Value.
		packField(statIndex + 1, field);
		return;
	}

	if (dynMatch == FieldStore::matchPartial) {
		// Dynamically Indexed Name + Literal Header Field Value.
		packField(dynIndex + staticTableSize + 1, field);
		return;
	}

	// New Name + Literal Header Field Value.
	packField(0, field);
}

void HttpPaxxer::HeaderFieldPacker::noteBytes(const char *buf, const size_t size) {
	std::cout.write(buf, size);
	std::cout.flush();
}

// HPACK Section 6.1: Indexed Header Field Representation.
void HttpPaxxer::HeaderFieldPacker::packField(const Index index) {
	// These fields never update the dynamic table.
	packInteger(index, 0x80, 7); // 0b10000000
	noteBytes(&theBuf[0], theBuf.size());
}

// HPACK Section 6.2.1: Literal Header Field with Incremental Indexing.
void HttpPaxxer::HeaderFieldPacker::packField(const Index index, const RecentHeaderField &field) {
	// TODO: This will change when we support sendButDoNotIndexField().
	(void)dynamicHeaders.push(field); // sender ignores insertion failures?

	packInteger(index, 0x40, 6); // 0b01000000

	if (!index)
		packString(field.name);

	packString(field.value);

	noteBytes(&theBuf[0], theBuf.size());
}

void HttpPaxxer::HeaderFieldPacker::packInteger(const uint64_t value, const uint8_t mask, const int prefixSize) {
	// TODO: Move this static outside if it is not initialized until the method
	// is called because that late initialization is not thread-safe.
	static const unsigned char prefMask[9] = {0, 1, 3, 7, 0xf, 0x1f, 0x3f, 0x7f, 0xff};
	if (value >= prefMask[prefixSize])
		throw std::string("long numbers are not supported"); // TODO
	if (mask & value)
		throw std::string("mask and prefix intersect");

	const uint8_t byte = mask | value;
	theBuf.push_back(static_cast<char>(byte));
}

void HttpPaxxer::HeaderFieldPacker::packString(const std::string &str) {
	packInteger(str.size(), 0, 7);
	const size_t vectorSize = theBuf.size();
	theBuf.resize(vectorSize + str.size());
	memcpy(&theBuf[vectorSize], str.c_str(), str.size());
}

// TODO: extract/encapsulate reusable paxxer testing logic from all *Paxxers.
int main(int argc, char *argv[]) {
	if (argc != 2) {
		// TODO: move usage printing into a dedicated function.
		std::cerr << "error: wrong number of arguments" << std::endl
			<< "Usage:" << std::endl
			<< "\t" << argv[0] << " --parse" << std::endl
			<< "\t" << argv[0] << " --pack" << std::endl;
		return 1;
	}

	const std::string argument(argv[1]);
	if (argument == "--parse") {
		HttpPaxxer::HeaderFieldPaxxerInit();
		std::vector<char> block;
		int ch;
		while ((ch = std::cin.get()) >= 0)
			block.push_back(ch);

		try {
			HttpPaxxer::HeaderFieldParser decomp;
			decomp.loadBytes(&block[0], block.size());
		}
		catch(std::string err) {
			std::cerr << "runtime error: " << err << std::endl;
			return 2;
		}
		HttpPaxxer::HeaderFieldPaxxerClean();
	}
	else if (argument == "--pack") {
		HttpPaxxer::HeaderFieldPaxxerInit();

		HttpPaxxer::HeaderFieldPacker packer;
		while (1) {
			std::string str;
			std::getline(std::cin, str);
			if (std::cin.eof() || str.empty())
				break;
			size_t pos = str.find(":", 1);
			if (pos == std::string::npos) {
				std::cerr << "error: missing ':' in HTTP header field: " << str << std::endl;
				return 2;
			}

			HttpPaxxer::RecentHeaderField field;
			field.name = str.substr(0, pos);
			if (pos + 1 < str.size())
				field.value = str.substr(pos + 2);

			try {
				packer.sendAndIndexField(field);
			}
			catch(std::string err) {
				std::cerr << "runtime error: " << err << std::endl;
				return 2;
			}
		}
		HttpPaxxer::HeaderFieldPaxxerClean();
	}
	else {
		// TODO: move usage printing into a dedicated function.
		std::cerr << "error: unknow command: " << argument << std::endl
			<< "Usage:" << std::endl
			<< "\t" << argv[0] << " --parse" << std::endl
			<< "\t" << argv[0] << " --pack" << std::endl;
	}

	return 0;
}
