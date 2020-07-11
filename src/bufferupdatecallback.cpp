// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/*
 * bit7z - A C++ static library to interface with the 7-zip DLLs.
 * Copyright (c) 2014-2019  Riccardo Ostani - All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * Bit7z is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bit7z; if not, see https://www.gnu.org/licenses/.
 */

#include "../include/bufferupdatecallback.hpp"

#include "../include/cbufferinstream.hpp"

using namespace std;
using namespace bit7z;

/* Most of this code is taken from the CUpdateCallback class in Client7z.cpp of the 7z SDK
 * Main changes made:
 *  + Use of std::vector instead of CRecordVector, CObjectVector and UStringVector
 *  + Use of tstring instead of UString (see Callback base interface)
 *  + Error messages are not showed (see comments in ExtractCallback)
 *  + The work performed originally by the Init method is now performed by the class constructor
 *  + FSItem class is used instead of CDirItem struct */

BufferUpdateCallback::BufferUpdateCallback( const BitArchiveCreator& creator,
                                            const vector< byte_t >& in_buffer,
                                            const tstring& in_buffer_name )
    : UpdateCallback( creator ),
      mBuffer( in_buffer ),
      mBufferName( in_buffer_name.empty() ? kEmptyFileAlias : in_buffer_name ) {}

COM_DECLSPEC_NOTHROW
STDMETHODIMP BufferUpdateCallback::GetProperty( UInt32 index, PROPID propID, PROPVARIANT* value ) {
    BitPropVariant prop;

    if ( propID == kpidIsAnti ) {
        prop = false;
    } else if ( index < mOldArcItemsCount ) {
        prop = mOldArc->getItemProperty( index, static_cast< BitProperty >( propID ) );
    } else {
        switch ( propID ) {
            case kpidPath:
                prop = mBufferName.wstring();
                break;
            case kpidIsDir:
                prop = false;
                break;
            case kpidSize:
                prop = static_cast< uint64_t >( sizeof( byte_t ) * mBuffer.size() );
                break;
            case kpidAttrib:
                prop = static_cast< uint32_t >( FILE_ATTRIBUTE_NORMAL );
                break;
            case kpidCTime:
            case kpidATime:
            case kpidMTime: {
                FILETIME ft;
                SYSTEMTIME st;

                GetSystemTime( &st ); // gets current time
                SystemTimeToFileTime( &st, &ft ); // converts to file time format
                prop = ft;
                break;
            }
            default: //empty prop
                break;
        }
    }

    *value = prop;
    prop.bstrVal = nullptr;
    return S_OK;
}

uint32_t BufferUpdateCallback::itemsCount() const {
    return mOldArcItemsCount + 1;
}

COM_DECLSPEC_NOTHROW
STDMETHODIMP BufferUpdateCallback::GetStream( UInt32 index, ISequentialInStream** inStream ) {
    RINOK( Finalize() )

    if ( index < mOldArcItemsCount ) { //old item in the archive
        return S_OK;
    }

    CMyComPtr< ISequentialInStream > inStreamLoc = new CBufferInStream( mBuffer );
    *inStream = inStreamLoc.Detach();
    return S_OK;
}

/* IArchiveUpdateCallback2 specific methods are unnecessary, but we need a common interface (CompressCallback) for both
   this class and UpdateCallback! */
COM_DECLSPEC_NOTHROW
STDMETHODIMP BufferUpdateCallback::GetVolumeSize( UInt32 /*index*/, UInt64* /*size*/ ) {
    return S_OK;
}

COM_DECLSPEC_NOTHROW
STDMETHODIMP BufferUpdateCallback::GetVolumeStream( UInt32 /*index*/, ISequentialOutStream** /*volumeStream*/ ) {
    return S_OK;
}
