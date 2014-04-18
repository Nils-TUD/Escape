/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <esc/common.h>
#include <esc/test.h>
#include <iostream>
#include <iomanip>
#include <stdio.h>

#include "circularbuf.h"

ssize_t CircularBuf::push(seq_type seqNo,const void *data,size_t size) {
	assert(size <= _max);
	// if we put in a control-message, it can't be out-of-order
	if(data == NULL && seqNo != _seqAcked)
		return -EINVAL;

	// normalize numbers so that the window starts at 0
	seq_type winStart = _seqAcked - _seqStart;
	seq_type winEnd = _max;
	seq_type relStart = seqNo - _seqStart;
	seq_type relEnd = seqNo + size - _seqStart;
	// if both points are outside the window, we can't use the data
	if((relStart < winStart || relStart >= winEnd) &&
		(relEnd < winStart || (relEnd > winEnd || relEnd == 0)))
		return -EINVAL;

	// adjust the received data accordingly
	const uint8_t *begin = reinterpret_cast<const uint8_t*>(data);
	if(relStart < winStart) {
		size_t diff = winStart - relStart;
		begin += diff;
		seqNo += diff;
		size -= diff;
		relStart = winStart;
	}
	else if(relStart >= winEnd) {
		begin -= relStart;
		seqNo -= relStart;
		size += relStart;
		relStart = 0;
	}

	if(relEnd > winEnd) {
		size -= relEnd - winEnd;
		relEnd = winEnd;
	}
	// now we know that seqNo and [begin,begin+size] fits into our window

	// find the position in the list
	size_t oldcur = _current;
	if(_packets.size() == 0) {
		_packets.push_back(SeqPacket(seqNo,begin,size));
		_current += size;
	}
	else {
		for(auto it = _packets.begin(); size > 0 && it != _packets.end(); ++it) {
			seq_type relSeq = it->start - _seqStart;
			// skip packet if it is already acked, but not yet pulled data
			// this might have a start before _seqStart if we already pulled some of that data
			if(relSeq >= _max)
				continue;

			// is there something missing?
			if(relStart < relSeq) {
				size_t amount = std::min(size,static_cast<size_t>(relSeq - relStart));
				_packets.insert(it,SeqPacket(seqNo,begin,amount));
				_current += amount;
				relStart += amount;
				seqNo += amount;
				begin += amount;
				size -= amount;
			}

			// duplicate data?
			if(relStart < relSeq + it->size) {
				// we're done
				if(relEnd <= relSeq + it->size)
					size = 0;
				else {
					// skip this part
					size_t diff = relSeq + it->size - relStart;
					relStart += diff;
					seqNo += diff;
					begin += diff;
					size -= diff;
				}
			}
		}
		// something left to insert?
		if(size > 0) {
			_packets.push_back(SeqPacket(seqNo,begin,size));
			_current += size;
		}
	}
	return _current - oldcur;
}

int CircularBuf::forget(seq_type seqNo) {
	seq_type relSeq = seqNo - _seqAcked;
	if(relSeq > _current)
		return -EINVAL;

	_seqAcked = seqNo;
	pull(NULL,relSeq);
	return 0;
}

CircularBuf::seq_type CircularBuf::getAck() {
	// search for the first not acked packet
	auto it = _packets.begin();
	for(; it != _packets.end() && it->start != _seqAcked; ++it)
		;

	// search for the next hole
	seq_type last = _seqAcked;
	for(; it != _packets.end(); ++it) {
		// is there still something missing?
		if(it->start != last)
			break;
		last += it->size;
	}
	_seqAcked = last;
	return last;
}

size_t CircularBuf::getOffset(const SeqPacket &pkt,seq_type seqNo) {
	size_t offset = 0;
	if(pkt.start != seqNo) {
		if(pkt.start < seqNo)
			offset = seqNo - pkt.start;
		else
			offset = (std::numeric_limits<seq_type>::max() - pkt.start) + seqNo + 1;
	}
	return offset;
}

size_t CircularBuf::get(seq_type seqNo,uint8_t *buf,size_t size) {
	// search for the first not acked packet
	auto it = _packets.begin();
	for(; it != _packets.end() && it->start != _seqAcked; ++it)
		;

	seq_type relSeq = seqNo - _seqStart;
	size_t orgsize = size;
	for(; size > 0 && it != _packets.end(); ++it) {
		// skip packets that are in front of what we're looking for
		seq_type relPktNo = it->start - _seqStart;
		if(relPktNo < relSeq)
			continue;

		size_t offset = getOffset(*it,seqNo);
		size_t amount = std::min(size,it->size - offset);
		// skip control packets
		if(it->data) {
			memcpy(buf,it->data + offset,amount);
			buf += amount;
			size -= amount;
		}
		seqNo += amount;
	}
	return orgsize - size;
}

size_t CircularBuf::pull(uint8_t *buf,size_t size) {
	size_t oldsize = size;
	while(size > 0 && _packets.size() > 0) {
		SeqPacket &pkt = _packets.front();
		if(pkt.start == _seqAcked)
			break;

		size_t offset = getOffset(pkt,_seqStart);
		size_t amount = std::min(size,pkt.size - offset);
		// skip control packets when we want to pull data
		if(buf && pkt.data) {
			memcpy(buf,pkt.data + offset,amount);
			buf += amount;
			size -= amount;
		}
		else if(!buf)
			size -= amount;
		_seqStart += amount;
		if(amount != pkt.size - offset)
			break;

		_current -= pkt.size;
		_packets.pop_front();
	}
	return oldsize - size;
}

void CircularBuf::print(std::ostream &os) {
	for(auto it = _packets.begin(); it != _packets.end(); ++it) {
		os << "[" << it->start << " .. " << (it->start + it->size) << "]";
		if(it->data) {
			for(size_t i = 0; i < it->size; ++i) {
				if(i % 8 == 0)
					os << "\n ";
				os << std::hex << std::setw(2) << std::setfill('0') << it->data[i];
				os << std::dec << std::setfill(' ') << ' ';
			}
		}
		os << "\n";
	}
}

static void test_assertSequence(const CircularBuf &cb) {
	size_t i = 0;
	for(auto it = cb.packets().begin(); it != cb.packets().end(); ++it) {
		for(size_t start = i, end = i + it->size; i < end; ++i)
			test_assertInt(it->data[i - start],i);
	}
}

void CircularBuf::unittest() {
	uint8_t data[128];
	uint8_t testdata[sizeof(data)] = {0};
	for(size_t i = 0; i < ARRAY_SIZE(data); ++i)
		data[i] = i;

	// simple case: no overflows, no holes
	{
		CircularBuf buf;
		buf.init(0,1024);
		buf.push(0,data,4);
		buf.push(4,data + 4,8);
		buf.push(12,data + 12,4);

		test_assertSequence(buf);

		// overlap of a complete packet
		test_assertSSize(buf.push(0,data,4),0);
		test_assertSequence(buf);

		test_assertSSize(buf.push(4,data + 4,8),0);
		test_assertSequence(buf);

		// overlap at the end
		test_assertSSize(buf.push(6,data + 6,6),0);
		test_assertSequence(buf);

		// overlap at the beginning
		test_assertSSize(buf.push(4,data + 4,4),0);
		test_assertSequence(buf);

		// overlap in the middle
		test_assertSSize(buf.push(2,data + 2,4),0);
		test_assertSequence(buf);

		// overlap of multiple packets
		test_assertSSize(buf.push(2,data + 2,12),0);
		test_assertSequence(buf);

		// out of window
		test_assertSSize(buf.push(1024,data,1),-EINVAL);
		test_assertSSize(buf.push(1026,data,4),-EINVAL);
		test_assertSSize(buf.push(-4,data,2),-EINVAL);
		test_assertSSize(buf.push(-4,data,4),-EINVAL);
		test_assertSequence(buf);

		fflush(stdout);
	}

	// still no overflows, but holes
	{
		CircularBuf buf;
		buf.init(0,1024);
		test_assertSSize(buf.push(12,data + 12,4),4);
		test_assertSSize(buf.push(24,data + 24,3),3);
		test_assertSSize(buf.push(28,data + 28,4),4);

		// directly in front of the beginning
		test_assertSSize(buf.push(8,data + 8,4),4);

		// fill hole at beginning
		test_assertSSize(buf.push(0,data + 0,8),8);

		// somewhere in the middle
		test_assertSSize(buf.push(20,data + 20,2),2);

		// overlapping data (1 byte at the end)
		test_assertSSize(buf.push(18,data + 18,3),2);
		// overlapping data (2 byte at beginning)
		test_assertSSize(buf.push(14,data + 14,4),2);
		// overlapping data (1 byte at beginning and 1 byte at the end)
		test_assertSSize(buf.push(21,data + 21,4),2);

		// overlap
		test_assertSSize(buf.push(16,data + 16,4),0);
		test_assertSSize(buf.push(25,data + 25,3),1);

		// complete overlap
		test_assertSSize(buf.push(0,data + 0,32),0);

		test_assertSequence(buf);

		fflush(stdout);
	}

	// overflow in circular buffer
	{
		CircularBuf buf;
		buf.init(-4,8);

		test_assertSSize(buf.push(-3,data + 1,1),1);
		test_assertSSize(buf.push(-2,data + 2,2),2);

		// end is behind
		test_assertSSize(buf.push(0,data + 4,6),4);
		// start is before
		test_assertSSize(buf.push(-5,data + -1,4),1);

		// start and end are behind
		test_assertSSize(buf.push(4,data + 8,2),-EINVAL);
		// start and end are before
		test_assertSSize(buf.push(-6,data + -2,1),-EINVAL);
		// start is before and end directly before
		test_assertSSize(buf.push(-6,data + -2,2),-EINVAL);

		fflush(stdout);
	}

	// ack and pull
	{
		CircularBuf buf;
		buf.init(-4,16);
		memset(testdata,0,sizeof(testdata));

		test_assertSSize(buf.push(-2,data + 2,4),4);
		test_assertLInt(buf.getAck(),-4);

		test_assertSSize(buf.push(-4,data + 0,2),2);
		test_assertLInt(buf.getAck(),2);

		test_assertSSize(buf.push(2,data + 6,16),10);
		test_assertLInt(buf.getAck(),12);

		test_assertSSize(buf.pull(testdata,1),1);
		test_assertSSize(buf.pull(testdata + 1,4),4);
		test_assertSSize(buf.pull(testdata + 5,8),8);

		test_assertLInt(buf.getAck(),12);

		for(size_t i = 0; i < 13; ++i)
			test_assertInt(testdata[i],i);

		fflush(stdout);
	}

	// ack and pull and push
	{
		CircularBuf buf;
		buf.init(-4,16);
		memset(testdata,0,sizeof(testdata));

		test_assertSSize(buf.push(-2,data + 2,4),4);
		test_assertLInt(buf.getAck(),-4);

		test_assertSSize(buf.push(-4,data + 0,2),2);
		test_assertLInt(buf.getAck(),2);

		test_assertSSize(buf.pull(testdata,1),1);

		test_assertSSize(buf.push(2,data + 6,8),8);
		test_assertLInt(buf.getAck(),10);

		test_assertSSize(buf.pull(testdata + 1,13),13);

		for(size_t i = 0; i < 14; ++i)
			test_assertInt(testdata[i],i);

		fflush(stdout);
	}
}
