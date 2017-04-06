#ifndef PEVERSIONEXTRACTOR_H
#define PEVERSIONEXTRACTOR_H

#include <iostream>	// for typedefs
#include <QString>
#include <QBuffer>

// typedefs
typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint64_t ULONGLONG;

#define IMAGE_DOS_SIGNATURE                 0x5A4D     /* MZ   */
#define IMAGE_NT_SIGNATURE                  0x00004550 /* PE00 */
#define IMAGE_SIZEOF_NT_SIGNATURE           4				/* dword */
#define IMAGE_DOS_EXTENDER_OFFSET           0x3c    /* offset to IMAGE_NT_HEADER*/

#define IMAGE_SIZEOF_FILE_HEADER            20
#define IMAGE_SIZEOF_OPTIONAL32_HEADER      224
#define IMAGE_SIZEOF_OPTIONAL64_HEADER      240
#define IMAGE_SIZEOF_SECTION_HEADER         40

#define IMAGE_OPTIONAL32_HEADER_MAGIC       0x10b
#define IMAGE_OPTIONAL64_HEADER_MAGIC       0x20b

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 		16
#define IMAGE_SIZEOF_SHORT_NAME             8

#define IMAGE_DIRECTORY_ENTRY_RESOURCE      2 // this directory we need
#define RT_VERSION													16 // this id has image_resource_directory_entry containing version information
#define VS_FIXEDFILEINFO_SIGNATURE					0xFEEF04BD

struct VS_FIXEDFILEINFO {
  DWORD dwSignature;
  DWORD dwStrucVersion;
  DWORD dwFileVersionMS; // this is what we are looking for 2 bytes 1st nbr next 2 bytes 2nd nbr
  DWORD dwFileVersionLS; // same here
  DWORD dwProductVersionMS;
  DWORD dwProductVersionLS;
  DWORD dwFileFlagsMask;
  DWORD dwFileFlags;
  DWORD dwFileOS;
  DWORD dwFileType;
  DWORD dwFileSubtype;
  DWORD dwFileDateMS;
  DWORD dwFileDateLS;
};

struct VS_VERSIONINFO {
  WORD             wLength;
  WORD             wValueLength;
  WORD             wType;
  WORD             szKey[17]; // lets just simplify this by making it constant size to align it to 32bits
  //word             Padding1; /* aligned to 32 bit */
  VS_FIXEDFILEINFO Value;
  WORD             Padding2; /* aligned to 32 bit */
  WORD             Children;
};

struct IMAGE_RESOURCE_DIRECTORY_ENTRY {
  union {
    struct {
      unsigned NameOffset:31;
      unsigned NameIsString:1;
    } DUMMYSTRUCTNAME;
    DWORD   Name;
    WORD    Id;
    WORD    __pad;
  } DUMMYUNIONNAME;
  union {
    DWORD   OffsetToData;
    struct {
      unsigned OffsetToDirectory:31;
      unsigned DataIsDirectory:1;
    } DUMMYSTRUCTNAME2;
  } DUMMYUNIONNAME2;
};

struct IMAGE_RESOURCE_DIRECTORY {
  DWORD   Characteristics;
  DWORD   TimeDateStamp;
  WORD    MajorVersion;
  WORD    MinorVersion;
  WORD    NumberOfNamedEntries;
  WORD    NumberOfIdEntries;
  /* right after this structure there is a array of IMAGE_RESOURCE_DIRECTORY_ENTRY DirectoryEntries[]; */
};

struct IMAGE_SECTION_HEADER {
  BYTE  Name[ IMAGE_SIZEOF_SHORT_NAME ];
  union {
    DWORD PhysicalAddress;
    DWORD VirtualSize;
  } Misc;
  DWORD VirtualAddress;
  DWORD SizeOfRawData;
  DWORD PointerToRawData;
  DWORD PointerToRelocations;
  DWORD PointerToLinenumbers;
  WORD  NumberOfRelocations;
  WORD  NumberOfLinenumbers;
  DWORD Characteristics;
};

struct IMAGE_DATA_DIRECTORY {
  DWORD VirtualAddress;
  DWORD Size;
};

struct IMAGE_FILE_HEADER {
  WORD  Machine;
  WORD  NumberOfSections;
  DWORD TimeDateStamp;
  DWORD PointerToSymbolTable;
  DWORD NumberOfSymbols;
  WORD  SizeOfOptionalHeader;
  WORD  Characteristics;
};

struct IMAGE_OPTIONAL_HEADER32 {
  /* Standard fields */
  WORD  Magic;                      /* 0x10b or 0x107 */
  BYTE  MajorLinkerVersion;
  BYTE  MinorLinkerVersion;
  DWORD SizeOfCode;
  DWORD SizeOfInitializedData;
  DWORD SizeOfUninitializedData;
  DWORD AddressOfEntryPoint;
  DWORD BaseOfCode;
  DWORD BaseOfData;
  /* NT additional fields */
  DWORD ImageBase;
  DWORD SectionAlignment;
  DWORD FileAlignment;
  WORD  MajorOperatingSystemVersion;
  WORD  MinorOperatingSystemVersion;
  WORD  MajorImageVersion;
  WORD  MinorImageVersion;
  WORD  MajorSubsystemVersion;
  WORD  MinorSubsystemVersion;
  DWORD Win32VersionValue;
  DWORD SizeOfImage;
  DWORD SizeOfHeaders;
  DWORD CheckSum;
  WORD  Subsystem;
  WORD  DllCharacteristics;
  DWORD SizeOfStackReserve;
  DWORD SizeOfStackCommit;
  DWORD SizeOfHeapReserve;
  DWORD SizeOfHeapCommit;
  DWORD LoaderFlags;
  DWORD NumberOfRvaAndSizes;
  IMAGE_DATA_DIRECTORY DataDirectory[ IMAGE_NUMBEROF_DIRECTORY_ENTRIES ];
};
struct IMAGE_OPTIONAL_HEADER64 {
  WORD  Magic; /* 0x20b */
  BYTE MajorLinkerVersion;
  BYTE MinorLinkerVersion;
  DWORD SizeOfCode;
  DWORD SizeOfInitializedData;
  DWORD SizeOfUninitializedData;
  DWORD AddressOfEntryPoint;
  DWORD BaseOfCode;
  ULONGLONG ImageBase;
  DWORD SectionAlignment;
  DWORD FileAlignment;
  WORD MajorOperatingSystemVersion;
  WORD MinorOperatingSystemVersion;
  WORD MajorImageVersion;
  WORD MinorImageVersion;
  WORD MajorSubsystemVersion;
  WORD MinorSubsystemVersion;
  DWORD Win32VersionValue;
  DWORD SizeOfImage;
  DWORD SizeOfHeaders;
  DWORD CheckSum;
  WORD Subsystem;
  WORD DllCharacteristics;
  ULONGLONG SizeOfStackReserve;
  ULONGLONG SizeOfStackCommit;
  ULONGLONG SizeOfHeapReserve;
  ULONGLONG SizeOfHeapCommit;
  DWORD LoaderFlags;
  DWORD NumberOfRvaAndSizes;
  IMAGE_DATA_DIRECTORY DataDirectory[ IMAGE_NUMBEROF_DIRECTORY_ENTRIES ];
};

struct IMAGE_NT_HEADERS32 {
  DWORD Signature;                          /* PE00 */
  IMAGE_FILE_HEADER FileHeader;
  IMAGE_OPTIONAL_HEADER32 OptionalHeader;
};

struct IMAGE_NT_HEADERS64 {
  DWORD Signature;                          /* "PE00" */
  IMAGE_FILE_HEADER FileHeader;
  IMAGE_OPTIONAL_HEADER64 OptionalHeader;
};

struct IMAGE_RESOURCE_DATA_ENTRY {
  DWORD	OffsetToData;
  DWORD	Size;
  DWORD	CodePage;
  DWORD	Reserved;
};

class PeVersionExtractor
{
public:
  explicit PeVersionExtractor( const QString& filename );
  /**
   * @brief extracts version from exe
   * @return 0 on failure or version on success
   */
  int extract();
private:
  QByteArray mFileData;
  QBuffer mBuffer;

  /**
   * @brief reads data from buffer and casts to T
   * @param data T&	- data to cast to
   * @param size DWORD ( uint32_t ) - size of data to read
   * @param offset DWORD ( uint32_t ) - buffer ofset
   */
  template<typename T>
  inline void readValue( T& data, DWORD size, DWORD offset ) {
    mBuffer.open( QIODevice::ReadOnly );
    mBuffer.seek( offset );
    mBuffer.read( reinterpret_cast< char * >( &data ), size );
    mBuffer.close();
  }

  /**
   * @brief reads data from buffer and casts to T
   * @param data T&	- data to cast to
   * @param offset DWORD ( uint32_t ) - buffer ofset
   */
  template<typename T>
  inline void readValue( T& data, DWORD offset ) {
    mBuffer.open( QIODevice::ReadOnly );
    mBuffer.seek( offset );
    mBuffer.read( reinterpret_cast< char * >( &data ), sizeof( data));
    mBuffer.close();
  }
};

#endif // PEVERSIONEXTRACTOR_H
