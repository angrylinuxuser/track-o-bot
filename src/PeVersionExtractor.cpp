#include "PeVersionExtractor.h"

#include <iostream>	// for typedefs
#include "Local.h"  // error reporting

#include <QBuffer>
#include <QFile>
#include <QVarLengthArray>

namespace {

// typedefs
typedef uint8_t byte;
typedef uint16_t word;
typedef uint32_t dword;
typedef uint64_t ulonglong;

const word IMAGE_DOS_SIGNATURE = 0x5A4D;     /* MZ */
const dword IMAGE_NT_SIGNATURE = 0x00004550; /* PE00 */
// const int IMAGE_SIZEOF_NT_SIGNATURE = sizeof( dword );
const word IMAGE_DOS_EXTENDER_OFFSET = 0x3c; /* offset to IMAGE_NT_HEADER*/
// const int IMAGE_SIZEOF_FILE_HEADER = 20;
const int IMAGE_SIZEOF_OPTIONAL32_HEADER = 224;
const int IMAGE_SIZEOF_OPTIONAL64_HEADER = 240;
const int IMAGE_SIZEOF_SECTION_HEADER = 40;
// const word IMAGE_OPTIONAL32_HEADER_MAGIC = 0x10b;
// const word IMAGE_OPTIONAL64_HEADER_MAGIC = 0x20b;
const int IMAGE_NUMBEROF_DIRECTORY_ENTRIES = 16;
const int IMAGE_SIZEOF_SHORT_NAME = 8;
const int IMAGE_DIRECTORY_ENTRY_RESOURCE = 2; /* this directory we need */
const int RT_VERSION = 16; /* this id has image_resource_directory_entry
                            containing version information */
const dword VS_FIXEDFILEINFO_SIGNATURE = 0xFEEF04BD;

struct VS_FIXEDFILEINFO {
  dword dwSignature;
  dword dwStrucVersion;
  dword dwFileVersionMS;  // this is what we are looking for 2 bytes 1st nbr next
  // 2 bytes 2nd nbr
  dword dwFileVersionLS;  // same here
  dword dwProductVersionMS;
  dword dwProductVersionLS;
  dword dwFileFlagsMask;
  dword dwFileFlags;
  dword dwFileOS;
  dword dwFileType;
  dword dwFileSubtype;
  dword dwFileDateMS;
  dword dwFileDateLS;
};

struct VS_VERSIONINFO {
  word wLength;
  word wValueLength;
  word wType;
  word szKey[17];  // lets just simplify this by making it constant size to align
  // it to 32bits
  // word             Padding1; /* aligned to 32 bit */
  VS_FIXEDFILEINFO Value;
  word Padding2; /* aligned to 32 bit */
  word Children;
};

struct IMAGE_RESOURCE_DIRECTORY_ENTRY {
  union {
    struct {
      unsigned NameOffset : 31;
      unsigned NameIsString : 1;
    } DUMMYSTRUCTNAME;
    dword Name;
    word Id;
    word __pad;
  } DUMMYUNIONNAME;
  union {
    dword OffsetToData;
    struct {
      unsigned OffsetToDirectory : 31;
      unsigned DataIsDirectory : 1;
    } DUMMYSTRUCTNAME2;
  } DUMMYUNIONNAME2;
};

struct IMAGE_RESOURCE_DIRECTORY {
  dword Characteristics;
  dword TimeDateStamp;
  word MajorVersion;
  word MinorVersion;
  word NumberOfNamedEntries;
  word NumberOfIdEntries;
  /* right after this structure there is a array of
   * IMAGE_RESOURCE_DIRECTORY_ENTRY DirectoryEntries[]; */
};

struct IMAGE_SECTION_HEADER {
  byte Name[IMAGE_SIZEOF_SHORT_NAME];
  union {
    dword PhysicalAddress;
    dword VirtualSize;
  } Misc;
  dword VirtualAddress;
  dword SizeOfRawData;
  dword PointerToRawData;
  dword PointerToRelocations;
  dword PointerToLinenumbers;
  word NumberOfRelocations;
  word NumberOfLinenumbers;
  dword Characteristics;
};

struct IMAGE_DATA_DIRECTORY {
  dword VirtualAddress;
  dword Size;
};

struct IMAGE_FILE_HEADER {
  word Machine;
  word NumberOfSections;
  dword TimeDateStamp;
  dword PointerToSymbolTable;
  dword NumberOfSymbols;
  word SizeOfOptionalHeader;
  word Characteristics;
};

struct IMAGE_OPTIONAL_HEADER32 {
  /* Standard fields */
  word Magic; /* 0x10b or 0x107 */
  byte MajorLinkerVersion;
  byte MinorLinkerVersion;
  dword SizeOfCode;
  dword SizeOfInitializedData;
  dword SizeOfUninitializedData;
  dword AddressOfEntryPoint;
  dword BaseOfCode;
  dword BaseOfData;
  /* NT additional fields */
  dword ImageBase;
  dword SectionAlignment;
  dword FileAlignment;
  word MajorOperatingSystemVersion;
  word MinorOperatingSystemVersion;
  word MajorImageVersion;
  word MinorImageVersion;
  word MajorSubsystemVersion;
  word MinorSubsystemVersion;
  dword Win32VersionValue;
  dword SizeOfImage;
  dword SizeOfHeaders;
  dword CheckSum;
  word Subsystem;
  word DllCharacteristics;
  dword SizeOfStackReserve;
  dword SizeOfStackCommit;
  dword SizeOfHeapReserve;
  dword SizeOfHeapCommit;
  dword LoaderFlags;
  dword NumberOfRvaAndSizes;
  IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
};

struct IMAGE_OPTIONAL_HEADER64 {
  word Magic; /* 0x20b */
  byte MajorLinkerVersion;
  byte MinorLinkerVersion;
  dword SizeOfCode;
  dword SizeOfInitializedData;
  dword SizeOfUninitializedData;
  dword AddressOfEntryPoint;
  dword BaseOfCode;
  ulonglong ImageBase;
  dword SectionAlignment;
  dword FileAlignment;
  word MajorOperatingSystemVersion;
  word MinorOperatingSystemVersion;
  word MajorImageVersion;
  word MinorImageVersion;
  word MajorSubsystemVersion;
  word MinorSubsystemVersion;
  dword Win32VersionValue;
  dword SizeOfImage;
  dword SizeOfHeaders;
  dword CheckSum;
  word Subsystem;
  word DllCharacteristics;
  ulonglong SizeOfStackReserve;
  ulonglong SizeOfStackCommit;
  ulonglong SizeOfHeapReserve;
  ulonglong SizeOfHeapCommit;
  dword LoaderFlags;
  dword NumberOfRvaAndSizes;
  IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
};

struct IMAGE_NT_HEADERS32 {
  dword Signature; /* PE00 */
  IMAGE_FILE_HEADER FileHeader;
  IMAGE_OPTIONAL_HEADER32 OptionalHeader;
};

struct IMAGE_NT_HEADERS64 {
  dword Signature; /* "PE00" */
  IMAGE_FILE_HEADER FileHeader;
  IMAGE_OPTIONAL_HEADER64 OptionalHeader;
};

struct IMAGE_RESOURCE_DATA_ENTRY {
  dword OffsetToData;
  dword Size;
  dword CodePage;
  dword Reserved;
};

/**
 * @brief reads data from buffer and casts to T
 * @param data T&	- data to cast to
 * @param size DWORD (  uint32_t  ) - size of data to read
 * @param offset DWORD (  uint32_t  ) - buffer ofset
 */
template <typename T>
void readValue( QBuffer &buffer, T &data, dword size, dword offset ) {
  buffer.open( QIODevice::ReadOnly );
  buffer.seek( offset );
  buffer.read( reinterpret_cast<char *>( &data ), size );
  buffer.close();
}
/**
 * @brief reads data from buffer and casts to T
 * @param data T&	- data to cast to
 * @param offset DWORD (  uint32_t  ) - buffer ofset
 */
template <typename T>
void readValue( QBuffer &buffer, T &data, dword offset ) {
  buffer.open( QIODevice::ReadOnly );
  buffer.seek( offset );
  buffer.read( reinterpret_cast<char *>( &data ), sizeof( data ) );
  buffer.close();
}
};

namespace PeVersionExtractor {

int Extract( const QString &fileName ) {
  union {
    IMAGE_NT_HEADERS32 nt32;
    IMAGE_NT_HEADERS64 nt64;
  } header;

  IMAGE_DATA_DIRECTORY resourceDataDir;
  IMAGE_SECTION_HEADER resourceSectionHdr;
  IMAGE_RESOURCE_DATA_ENTRY dataEntry;
  IMAGE_RESOURCE_DIRECTORY rootResourceDir;
  VS_VERSIONINFO fileinfo;

  dword offset = 0, nSections = 0;
  word dosSignature = 0, nEntries = 0;
  int version = 0;

  QFile hsExe( fileName );
  QByteArray fileData;
  QBuffer fileBuffer;

  // it will only work on little endian
  Q_ASSERT( Q_BYTE_ORDER == Q_LITTLE_ENDIAN );

  // read all data to buffer
  if ( hsExe.open( QIODevice::ReadOnly ) ) {
    fileData = hsExe.readAll();
    fileBuffer.setBuffer( &fileData );
    hsExe.close();
  } else {
    ERR( "PeVersionExtractor: Unable to open file %s!", fileName.toStdString().c_str() );
    return version;
  }

  // let's check dos signature
  readValue( fileBuffer, dosSignature, offset );
  if ( dosSignature != IMAGE_DOS_SIGNATURE ) {
    ERR( "Incorrect IMAGE_DOS_SIGNATURE!: %s", dosSignature );
    return version;
  }

  // get offset to IMAGE_NT_HEADER
  readValue( fileBuffer, offset, IMAGE_DOS_EXTENDER_OFFSET );

  // read header 32 (  we only need header and signature and they are the same
  // for 32 and 64 bit )
  readValue( fileBuffer, header.nt32, offset );

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
    resourceDataDir = header.nt32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];
    offset += sizeof( header.nt32 );
    break;
  }
    // 64 bit
  case IMAGE_SIZEOF_OPTIONAL64_HEADER: {
    resourceDataDir = header.nt64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];
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
    readValue( fileBuffer, resourceSectionHdr, offset );
    offset += IMAGE_SIZEOF_SECTION_HEADER;
    if ( ( resourceDataDir.VirtualAddress >= resourceSectionHdr.VirtualAddress ) &&
         ( resourceDataDir.VirtualAddress < resourceSectionHdr.VirtualAddress +
           resourceSectionHdr.SizeOfRawData ) ) {
      break;
    }
  }
  // get offset to resources
  offset = resourceSectionHdr.PointerToRawData;

  readValue( fileBuffer, rootResourceDir, offset );

  // nEntries = rootResourceDir.NumberOfIdEntries +
  // rootResourceDir.NumberOfNamedEntries;
  nEntries = rootResourceDir.NumberOfIdEntries;
  // root directory entries are located right after the section header
  // first are named entries then id entries (  we want id entries )
  QVarLengthArray<IMAGE_RESOURCE_DIRECTORY_ENTRY> dirEntries( nEntries );
  readValue( fileBuffer, *( dirEntries.data() ),
             nEntries * sizeof( IMAGE_RESOURCE_DATA_ENTRY ),
             // we must also add number of named entries
             offset + sizeof( rootResourceDir ) +
             rootResourceDir.NumberOfNamedEntries *
             sizeof( rootResourceDir.NumberOfNamedEntries ) );

  // find entry containing version info
  // entry points to image resource dir and that points to entry ( or resource
  // data - last level of the tree: this is what we are looking for )
  for ( const auto &dirEntry : dirEntries ) {
    if ( !dirEntry.DUMMYUNIONNAME.DUMMYSTRUCTNAME.NameIsString ) {
      if ( dirEntry.DUMMYUNIONNAME.Id == RT_VERSION ) {
        // find our resource data ( dig deeper into tree )
        bool found = false;
        dword resOffset = offset + dirEntry.DUMMYUNIONNAME2.DUMMYSTRUCTNAME2.OffsetToDirectory;
        IMAGE_RESOURCE_DIRECTORY_ENTRY entry;

        while ( !found ) {
          IMAGE_RESOURCE_DIRECTORY resDir;
          readValue( fileBuffer, resDir, resOffset );
          // at this level resDir should contain only 1 id entry
          Q_ASSERT( resDir.NumberOfIdEntries == 1 &&
                    resDir.NumberOfNamedEntries == 0 );
          resOffset += sizeof( IMAGE_RESOURCE_DIRECTORY );
          readValue( fileBuffer, entry, resOffset );
          // if is dir then dig deeper

          if ( entry.DUMMYUNIONNAME2.DUMMYSTRUCTNAME2.DataIsDirectory ) {
            resOffset = offset + entry.DUMMYUNIONNAME2.DUMMYSTRUCTNAME2.OffsetToDirectory;
            continue;
          }

          // if not then its our resource containing version info struct
          else {
            resOffset = offset + entry.DUMMYUNIONNAME2.OffsetToData;
            readValue( fileBuffer, dataEntry, resOffset );
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
  // more info here:
  // http://microsoft.public.win32.programmer.kernel.narkive.com/C60Zmgca/handling-win32-pe-exe-files
  offset = resourceSectionHdr.PointerToRawData + dataEntry.OffsetToData - resourceSectionHdr.VirtualAddress;
  readValue( fileBuffer, fileinfo, offset );

  if ( fileinfo.wValueLength <= 0 ) {
    ERR( "VS_FIXEDFILEINFO size is 0! No version info available" );
  } else if ( fileinfo.Value.dwSignature == VS_FIXEDFILEINFO_SIGNATURE ) {
    version = ( fileinfo.Value.dwFileVersionLS >> 0 ) & 0xffff;
    DBG( "Detected build: %d", version );
  } else {
    ERR( "Wrong VS_FIXEDFILEINFO signature!: %d", fileinfo.Value.dwSignature );
  }
  return version;
}
};
