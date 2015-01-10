/* Copyright (c) 2015, The Measurement Factory. */

#ifndef HTTP_PAXXER_FORWARD_H
#define HTTP_PAXXER_FORWARD_H

#include <HttpPaxxer/Paxxer.h>
#include <tr1/memory>

namespace HttpPaxxer {

class Raw;
class Writer;
class StreamInterpreter;
class ConnectionInterpreter;
class Stream;
class Connection;
class FrameHeader;
class Headers;
class HeaderField;

// TODO: add support for boost pointers if std::tr1 is not available?
using std::tr1::weak_ptr;
using std::tr1::shared_ptr;

} // namespace HttpPaxxer

#endif
