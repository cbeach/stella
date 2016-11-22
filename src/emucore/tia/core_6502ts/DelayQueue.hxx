//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef TIA_6502TS_CORE_DELAY_QUEUE
#define TIA_6502TS_CORE_DELAY_QUEUE

#include "bspf.hxx"
#include "DelayQueueMember.hxx"

namespace TIA6502tsCore {

class DelayQueue
{
  public:
    DelayQueue(uInt8 length, uInt8 size);

  public:

    void push(uInt8 address, uInt8 value, uInt8 delay);

    void reset();

    template<class T> void execute(T executor);

  private:

    vector<DelayQueueMember> myMembers;

    uInt8 myIndex;

    uInt8 myIndices[0xFF];

  private:
    DelayQueue() = delete;
    DelayQueue(const DelayQueue&) = delete;
    DelayQueue(DelayQueue&&) = delete;
    DelayQueue& operator=(const DelayQueue&) = delete;
    DelayQueue& operator=(DelayQueue&&) = delete;
};

// ############################################################################
// Implementation
// ############################################################################

template<class T> void DelayQueue::execute(T executor)
{
  DelayQueueMember& currentMember = myMembers.at(myIndex);

  for (auto&& entry : currentMember) {
    executor(entry.address, entry.value);
    myIndices[entry.address] = 0xFF;
  }

  currentMember.clear();

  myIndex = (myIndex + 1) % myMembers.size();
}

} // namespace TIA6502tsCore

#endif //  TIA_6502TS_CORE_DELAY_QUEUE
