/* Copyright (c) 2015, The Measurement Factory. */

#ifndef HTTP_PAXXER_HEADER_FIELD_PAXXER_H
#define HTTP_PAXXER_HEADER_FIELD_PAXXER_H

#include <HttpPaxxer/Paxxer.h>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <stdint.h>


namespace HttpPaxxer {

/* HTTP/2 Header Compression (HPACK) */


// HTTP header field name:value pair with an HPACK size calculation algorithm
// TODO: Support a user-provided type (with this type as the default?).
// TOOD: Or should we switch to std::pair<>?
class RecentHeaderField {
	public:
		std::string name;
		std::string value;
};

// A pointer to RecentHeaderField. Used as a HpackEncoderHeaderFields map key
// to minimize RAM usage by not storing RecentHeaderField in map _and_ queue.
// XXX: This class is probably not really neeeded, but it is difficult to
// remove it because HeaderFieldPacker searches RecentHeaderField in the map.
class RecentHeaderFieldRef {
	public:
		RecentHeaderFieldRef(const RecentHeaderField &aField): ref(&aField) {}
		const std::string &name() const { return ref->name; }
		const std::string &value() const { return ref->value; }
		const RecentHeaderField &field() const { return *ref; }

	private:
		const RecentHeaderField *ref;
};

inline
size_t HpackSize(const RecentHeaderField &field) {
	return field.name.size() + field.value.size() + 32;
}

// HPACK Header field storage. Maintains order and dynamic table size limits.
// This class is sufficient for a decoder field index.
template <class Store>
class HpackHeaderFields {
	public:
		// 4096 default from HTTP/2 Section 6.5.2 (SETTINGS_HEADER_TABLE_SIZE).
		HpackHeaderFields(): hpackSize_(0), hpackSizeMax_(4096) {}

		const Store &store() const { return store_; }

		// either adds field to the storage or returns false
		bool push(const typename Store::value_type &field) {
			const size_t fieldSize = HpackSize(field);
			if (!shrinkToFit(fieldSize))
				return false;
			store_.push_front(field);
			hpackSize_ += fieldSize;
			return true;
		}

		// Increases or decreases hpackSize limit.
		// TODO: HTTP/2 hpackSize is bounded by 2^32. The decoder MUST not
		// signal (or accept!) table sizes larger than max(SendTime).
		void limitHpackSize(const size_t newMax) {
			// TODO: honor absolute HTTP/2 maximum?
			hpackSizeMax_ = newMax;
			// We could call shrinkToFit(0), but that would require that method
			// to handle overflows differently, which would slow it down.
			while (hpackSize_ > hpackSizeMax_) // may remove all entries
				pop();
		}

	protected:
		bool shrinkToFit(const size_t extraSize) {
			// This shape of the loop condition prevents size_t overflows.
			// TODO: Optimize by storing the "remaining space" size?
			while (extraSize > hpackSizeMax_ - hpackSize_) {
				if (store_.empty())
					return false; // cannot accomodate extraSize
				pop();
			}
			return true;
		}

		void pop() {
			hpackSize_ -= HpackSize(store_.back());
			store_.pop_back();
		}

	private:
		Store store_; // a FIFO queue of recent header fields

		size_t hpackSize_; // the sum of all queued entry sizes
		size_t hpackSizeMax_; // hpackSize_ cannot exceed this maximum
		// HTTP/2 hpackSizeMax_ is itself bounded by 2^32 because the
		// SETTINGS_HEADER_TABLE_SIZE value takes 32 bits. The decoder
		// MUST not advertisze support for more than sizeof(size_t)!
};


// HPACK [Dynamic] Header Table for an encoder.
// Works like a queue, but also maps fields to their queue position.
// Suitable for being used as HpackHeaderFields::Storage.
template <class Queue, class Map>
class HpackEncoderHeaderFields {
	public:
		// The "time" when a header field was "pushed" or "sent". Measured in
		// the number of push_front() calls, including the sending one. Unlike
		// the Queue position, "time" is immune to subsequent push_front calls.
		// Tolerates under/overflows as long as max(SendTime) >= queue length.
		// TODO: Assert the inequality for the actual types.
		typedef typename Map::mapped_type SendTime;
		typedef typename Queue::size_type size_type;
		typedef typename Queue::value_type value_type;

		// convert push/pop-immune field "send time" value into queue position
		size_type timeToPos(const SendTime fieldSendTime) const {
			return lastSendTime_ - fieldSendTime; // underflow OK
		}

		// finds a full or partial match in any RecentHeaderField:index mapping
		typedef enum { matchNone, matchPartial, matchFull } MatchKind;

		MatchKind find(const value_type &field, size_type &index) const {
			typename Map::const_iterator it = map.lower_bound(field);
			if (it != map.end() && it->first.name() == field.name) {
				index = timeToPos(it->second);
				return (it->first.value() == field.value) ? matchFull : matchPartial;
			}

			if (it != map.begin()) {
				it--;
				if (it->first.name() == field.name) {
					index = timeToPos(it->second);
					return matchPartial;
				}
			}

			return matchNone;
		}


		/* HpackHeaderFields::Store API */

		HpackEncoderHeaderFields(): lastSendTime_(0) {}

		bool empty() const { return queue.empty(); }

		size_type size() const { return queue.size(); }

		const value_type &at(size_type index) const { return queue.at(index); }

		const value_type &back() const { return queue.back(); }

		void push_front(const value_type &field) {
			queue.push_front(field);
			const typename Map::key_type fieldRef(queue.front());
			map.insert(std::make_pair(fieldRef, ++lastSendTime_)); // overflow OK
		}

		void pop_back() {
			map.erase(queue.back());
			queue.pop_back();
			// pop_back() preserves "time" -- all queue indexes remain the same
		}

	public:
		Queue queue; // FIFO queue of sent fields
		Map map; // sent times of queued fields

	private:
		SendTime lastSendTime_;
};


typedef size_t Index;

// converts header fields into bytes on the wire while maintaining their index
class HeaderFieldPacker {
	public:
		// HPACK: Literal Header Field with Incremental Indexing
		void sendAndIndexField(const RecentHeaderField &field);

	private:
		void noteBytes(const char *buf, const size_t size);
		void packField(const Index index);
		void packField(const Index index, const RecentHeaderField &field);
		void packString(const std::string &str);
		void packInteger(const uint64_t value, const uint8_t mask, const int prefixSize);

		typedef HpackEncoderHeaderFields<
			std::deque<RecentHeaderField>,
			std::map<RecentHeaderFieldRef, Index> > FieldStore;
		// TODO: Make this a user-defined type.
		typedef HpackHeaderFields<FieldStore> Fields;
		Fields dynamicHeaders;

		std::vector<char> theBuf;
};

// creates header fields from bytes on the wire while maintaining their index
class HeaderFieldParser {
	public:
		size_t loadBytes(const char *buf, size_t size);

	private:
		void noteField(const RecentHeaderField &field);
		long parseField(const char *buf, size_t size, RecentHeaderField &field);
		long parseString(const char *buf, size_t size, std::string &str);
		long parseInteger(uint64_t &value, const char *buf, const size_t size, const int prefixSize);
		void findField(size_t index, RecentHeaderField &field) const;

		// HPACK [Dynamic] Header Table for a decoder.
		// TODO: Make this a user-defined type.
		typedef HpackHeaderFields< std::deque<RecentHeaderField> > HpackDecoderFields;
		HpackDecoderFields dynamicHeaders;
};

void HeaderFieldPaxxerInit();
void HeaderFieldPaxxerClean();

} // namespace HttpPaxxer;

#endif /* HTTP_PAXXER_HEADER_FIELD_PAXXER_H */
