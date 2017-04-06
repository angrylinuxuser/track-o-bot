#include "PeVersionExtractor.h"
#include <QString>
#include <QFile>
#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QVarLengthArray>

PeVersionExtractor::PeVersionExtractor(const QString& filename) {
  QFile hsExe(filename);
  // read all data to buffer
  if( hsExe.open( QIODevice::ReadOnly )) {
    mFileData = hsExe.readAll();
    mBuffer.setBuffer( &mFileData );
    hsExe.close();
  }
}

int PeVersionExtractor::extract() {
  union {
    IMAGE_NT_HEADERS32 nt32;
    IMAGE_NT_HEADERS64 nt64;
  } header;
  IMAGE_DATA_DIRECTORY resourceDataDir;
  IMAGE_SECTION_HEADER resourceSectionHdr;
  IMAGE_RESOURCE_DATA_ENTRY dataEntry;
  IMAGE_RESOURCE_DIRECTORY rootResourceDir;
  VS_VERSIONINFO fileinfo;

  DWORD offset = 0, nSections = 0;
  WORD dosSignature = 0, nEntries = 0;
  int version = 0;

  // let's check dos signature
  readValue( dosSignature, offset );
  if ( dosSignature != IMAGE_DOS_SIGNATURE ) {
    ERR( "Incorrect IMAGE_DOS_SIGNATURE!: %s", dosSignature );
    return version;
  }

  // get offset to IMAGE_NT_HEADER
  readValue( offset, IMAGE_DOS_EXTENDER_OFFSET );

  // read header 32 ( we only need header and signature and they are the same for 32 and 64 bit)
  readValue( header.nt32, offset );

  // we can do that since singature and
  // image_file_header are the same size for 32 and 64bit
  // the only difference is 64bit optional header size
  if ( header.nt32.Signature != IMAGE_NT_SIGNATURE ) {
    ERR( "Incorrect IMAGE_NT_SIGNATURE!: %s", header.nt32.Signature );
    return version;
  }

  switch ( header.nt32.FileHeader.SizeOfOptionalHeader ) {
    // 32 bit
    case IMAGE_SIZEOF_OPTIONAL32_HEADER: {
        resourceDataDir = header.nt32.
                          OptionalHeader.
                          DataDirectory[ IMAGE_DIRECTORY_ENTRY_RESOURCE ];
        //offset += IMAGE_SIZEOF_FILE_HEADER + IMAGE_SIZEOF_OPTIONAL32_HEADER + IMAGE_SIZEOF_NT_SIGNATURE;
        offset += sizeof( header.nt32 );
        break;
      }
    // 64 bit
    case IMAGE_SIZEOF_OPTIONAL64_HEADER: {
        resourceDataDir = header.nt64.
                          OptionalHeader.
                          DataDirectory[ IMAGE_DIRECTORY_ENTRY_RESOURCE ];
        //offset += IMAGE_SIZEOF_FILE_HEADER + IMAGE_SIZEOF_OPTIONAL64_HEADER + IMAGE_SIZEOF_NT_SIGNATURE;
        offset += sizeof( header.nt64 );
        break;
      }
    default: {
        ERR( "Wrong size of optional header!" );
        return version;
        break;
      }
  }
  // we can do it for 32 and 64
  nSections = header.nt32.FileHeader.NumberOfSections;
  // sections reside right after the optional header
  // resource dir rva is relative to the section it resides in
  for ( unsigned s = 0; s < nSections; ++s ) {
    readValue( resourceSectionHdr, offset );
    offset += IMAGE_SIZEOF_SECTION_HEADER;
    if (( resourceDataDir.VirtualAddress >= resourceSectionHdr.VirtualAddress ) &&
        ( resourceDataDir.VirtualAddress < resourceSectionHdr.VirtualAddress + resourceSectionHdr.SizeOfRawData )) {
      break;
    }
  }
  // get offset to resources
  offset = resourceSectionHdr.PointerToRawData;

  readValue( rootResourceDir, offset );

  //nEntries = rootResourceDir.NumberOfIdEntries + rootResourceDir.NumberOfNamedEntries;
  nEntries = rootResourceDir.NumberOfIdEntries;
  // root directory entries are located right after the section header
  // first are named entries then id entries ( we want id entries)
  QVarLengthArray< IMAGE_RESOURCE_DIRECTORY_ENTRY > dirEntries( nEntries );
  readValue( *(dirEntries.data() ), nEntries * sizeof( IMAGE_RESOURCE_DATA_ENTRY ),
             // we must add also number of named entries
             offset + sizeof( rootResourceDir ) + rootResourceDir.NumberOfNamedEntries * sizeof ( rootResourceDir.NumberOfNamedEntries ));

  // find entry containing version info
  // entry points to image resource dir and that points to entry (or resource data - last level of the tree: this is what we are looking for)
  for ( const auto& dirEntry: dirEntries ) {
    if ( !dirEntry.DUMMYUNIONNAME.DUMMYSTRUCTNAME.NameIsString ) {
      if ( dirEntry.DUMMYUNIONNAME.Id == RT_VERSION ) {
        // find our resource data (dig deeper into tree)
        bool found = false;
        DWORD resOffset = offset + dirEntry.DUMMYUNIONNAME2.DUMMYSTRUCTNAME2.OffsetToDirectory;
        IMAGE_RESOURCE_DIRECTORY_ENTRY entry;
        while(!found) {
          IMAGE_RESOURCE_DIRECTORY resDir;
          readValue(resDir, resOffset);
          // at this level resDir should contain only 1 id entry
          Q_ASSERT(resDir.NumberOfIdEntries == 1 && resDir.NumberOfNamedEntries == 0);
          resOffset += sizeof( IMAGE_RESOURCE_DIRECTORY );
          readValue(entry, resOffset);
          // if is dir then dig deeper
          if (entry.DUMMYUNIONNAME2.DUMMYSTRUCTNAME2.DataIsDirectory) {
            resOffset = offset + entry.DUMMYUNIONNAME2.DUMMYSTRUCTNAME2.OffsetToDirectory;
            continue;
          }
          // if not then its our resource containing version info struct
          else {
            resOffset = offset + entry.DUMMYUNIONNAME2.OffsetToData;
            readValue( dataEntry, resOffset);
            found = true;
          }
        }
        break;
      }
    }
  }

  // calculate rva to vs_versioninfo structure
  // we have to add section's pointerToRawData to entries offsetToData
  // and substract section's virtualAddress
  // more info here: http://microsoft.public.win32.programmer.kernel.narkive.com/C60Zmgca/handling-win32-pe-exe-files
  offset = resourceSectionHdr.PointerToRawData + dataEntry.OffsetToData - resourceSectionHdr.VirtualAddress;
  readValue( fileinfo, offset );

  if( fileinfo.wValueLength > 0 ) {
    if( fileinfo.Value.dwSignature == VS_FIXEDFILEINFO_SIGNATURE ) {
      version = ( fileinfo.Value.dwFileVersionLS >> 0 ) & 0xffff;
      DBG( "Detected build: %d", version );
    } else {
      ERR( "Wrong VS_FIXEDFILEINFO signature!: %d", fileinfo.Value.dwSignature );
    }
  } else {
    ERR( "VS_FIXEDFILEINFO size is 0! No version info available" );
  }
  return version;
}
