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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <cassert>
#include <cstring>

#include "System.hxx"
#include "TIA.hxx"
#include "Cart3E.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3E::Cartridge3E(const uInt8* image, uInt32 size)
  : mySize(size)
{
  // Allocate array for the ROM image
  myImage = new uInt8[mySize];

  // Copy the ROM image into my buffer
  memcpy(myImage, image, mySize);

  // This cart can address a 1024 byte bank of RAM @ 0x1000
  // However, it may not be addressable all the time (it may be swapped out)
  // so probably most of the time, the area will point to ROM instead
  registerRamArea(0x1000, 1024, 0x00, 0x400);  // 1024 bytes RAM @ 0x1000

  // Remember startup bank
  myStartBank = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge3E::~Cartridge3E()
{
  delete[] myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3E::reset()
{
  // Initialize RAM with random values
  for(uInt32 i = 0; i < 32768; ++i)
    myRam[i] = mySystem->randGenerator().next();

  // We'll map the startup bank into the first segment upon reset
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3E::install(System& system)
{
  mySystem = &system;
  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Make sure the system we're being installed in has a page size that'll work
  assert((0x1800 & mask) == 0);

  // Set the page accessing methods for the hot spots (for 100% emulation
  // we need to chain any accesses below 0x40 to the TIA. Our poke() method
  // does this via mySystem->tiaPoke(...), at least until we come up with a
  // cleaner way to do it).
  System::PageAccess access;
  for(uInt32 i = 0x00; i < 0x40; i += (1 << shift))
  {
    access.device = this;
    access.directPeekBase = 0;
    access.directPokeBase = 0;
    mySystem->setPageAccess(i >> shift, access);
  }

  // Setup the second segment to always point to the last ROM slice
  for(uInt32 j = 0x1800; j < 0x2000; j += (1 << shift))
  {
    access.device = this;
    access.directPeekBase = &myImage[(mySize - 2048) + (j & 0x07FF)];
    access.directPokeBase = 0;
    mySystem->setPageAccess(j >> shift, access);
  }

  // Install pages for the startup bank into the first segment
  bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Cartridge3E::peek(uInt16 address)
{
  uInt16 peekAddress = address;
  address &= 0x0FFF;

  if(address < 0x0800)
  {
    if(myCurrentBank < 256)
      return myImage[(address & 0x07FF) + (myCurrentBank << 11)];
    else
    {
      if(address < 0x0400)
        return myRam[(address & 0x03FF) + ((myCurrentBank - 256) << 10)];
      else
      {
        // Reading from the write port triggers an unwanted write
        uInt8 value = mySystem->getDataBusState(0xFF);

        if(bankLocked())
          return value;
        else
        {
          triggerReadFromWritePort(peekAddress);
          return myRam[(address & 0x03FF) + ((myCurrentBank - 256) << 10)] = value;
        }
      }
    }
  }
  else
  {
    return myImage[(address & 0x07FF) + mySize - 2048];
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3E::poke(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  // Switch banks if necessary. Armin (Kroko) says there are no mirrored
  // hotspots.
  if(address == 0x003F)
  {
    bank(value);
  }
  else if(address == 0x003E)
  {
    bank(value + 256);
  }

  // Pass the poke through to the TIA. In a real Atari, both the cart and the
  // TIA see the address lines, and both react accordingly. In Stella, each
  // 64-byte chunk of address space is "owned" by only one device. If we
  // don't chain the poke to the TIA, then the TIA can't see it...
  mySystem->tia().poke(address, value);

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge3E::bank(uInt16 bank)
{ 
  if(bankLocked()) return;

  if(bank < 256)
  {
    // Make sure the bank they're asking for is reasonable
    if(((uInt32)bank << 11) < uInt32(mySize))
    {
      myCurrentBank = bank;
    }
    else
    {
      // Oops, the bank they're asking for isn't valid so let's wrap it
      // around to a valid bank number
      myCurrentBank = bank % (mySize >> 11);
    }
  
    uInt32 offset = myCurrentBank << 11;
    uInt16 shift = mySystem->pageShift();
  
    // Setup the page access methods for the current bank
    System::PageAccess access;
    access.device = this;
    access.directPokeBase = 0;
  
    // Map ROM image into the system
    for(uInt32 address = 0x1000; address < 0x1800; address += (1 << shift))
    {
      access.directPeekBase = &myImage[offset + (address & 0x07FF)];
      mySystem->setPageAccess(address >> shift, access);
    }
  }
  else
  {
    bank -= 256;
    bank %= 32;
    myCurrentBank = bank + 256;

    uInt32 offset = bank << 10;
    uInt16 shift = mySystem->pageShift();
    uInt32 address;
  
    // Setup the page access methods for the current bank
    System::PageAccess access;
    access.device = this;
    access.directPokeBase = 0;
  
    // Map read-port RAM image into the system
    for(address = 0x1000; address < 0x1400; address += (1 << shift))
    {
      access.directPeekBase = &myRam[offset + (address & 0x03FF)];
      mySystem->setPageAccess(address >> shift, access);
    }

    access.directPeekBase = 0;

    // Map write-port RAM image into the system
    for(address = 0x1400; address < 0x1800; address += (1 << shift))
    {
      access.directPokeBase = &myRam[offset + (address & 0x03FF)];
      mySystem->setPageAccess(address >> shift, access);
    }
  }
  myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Cartridge3E::bank()
{
  return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Cartridge3E::bankCount()
{
  // In addition to the number of 2K banks in ROM, there are 32 more 1K
  // banks for RAM (doubled to 2K because of a read and write port)
  return (mySize >> 11) + 32;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3E::patch(uInt16 address, uInt8 value)
{
  address &= 0x0FFF;

  if(address < 0x0800)
  {
    if(myCurrentBank < 256)
      myImage[(address & 0x07FF) + (myCurrentBank << 11)] = value;
    else
      myRam[(address & 0x03FF) + ((myCurrentBank - 256) << 10)] = value;
  }
  else
    myImage[(address & 0x07FF) + mySize - 2048] = value;

  return myBankChanged = true;
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8* Cartridge3E::getImage(int& size)
{
  size = mySize;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3E::save(Serializer& out) const
{
  const string& cart = name();

  try
  {
    out.putString(cart);
    out.putInt(myCurrentBank);

    // Output RAM
    out.putInt(32768);
    for(uInt32 addr = 0; addr < 32768; ++addr)
      out.putByte((char)myRam[addr]);
  }
  catch(const char* msg)
  {
    cerr << "ERROR: Cartridge3E::save" << endl << "  " << msg << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge3E::load(Serializer& in)
{
  const string& cart = name();

  try
  {
    if(in.getString() != cart)
      return false;

    myCurrentBank = (uInt16) in.getInt();

    // Input RAM
    uInt32 limit = (uInt32) in.getInt();
    for(uInt32 addr = 0; addr < limit; ++addr)
      myRam[addr] = (uInt8) in.getByte();
  }
  catch(const char* msg)
  {
    cerr << "ERROR: Cartridge3E::load" << endl << "  " << msg << endl;
    return false;
  }

  // Now, go to the current bank
  bank(myCurrentBank);

  return true;
}
