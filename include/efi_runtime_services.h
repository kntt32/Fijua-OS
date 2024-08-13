#ifndef INCLUDED_EFI_RUNTIME_SERVICES_H
#define INCLUDED_EFI_RUNTIME_SERVICES_H

#define EFI_RUNTIME_SERVICES_SIGNATURE 0x56524553544e5552
#define EFI_RUNTIME_SERVICES_REVISION EFI_SPECIFICATION_VERSION

//******************************************************
// Variable Attributes
//******************************************************
#define EFI_VARIABLE_NON_VOLATILE                           0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS                     0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS                         0x00000004
#define EFI_VARIABLE_HARDWARE_ERROR_RECORD                  0x00000008 \
//This attribute is identified by the mnemonic 'HR' elsewhere
//in this specification.
#define EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS             0x00000010
//NOTE: EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS is deprecated
//and should be considered reserved.
#define EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS \0x00000020
#define EFI_VARIABLE_APPEND_WRITE                           0x00000040
#define EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS          0x00000080
//This attribute indicates that the variable payload begins
//with an EFI_VARIABLE_AUTHENTICATION_3 structure, and
//potentially more structures as indicated by fields of this
//structure. See definition below and in SetVariable().

//
// EFI_VARIABLE_AUTHENTICATION_3_CERT_ID descriptor
//
// An extensible structure to identify a unique x509 cert
// associated with a given variable
//
#define EFI_VARIABLE_AUTHENTICATION_3_CERT_ID_SHA256 1

//************************************************
//EFI_TIME
//************************************************
// This represents the current time information
typedef struct {
   UINT16    Year;              // 1900 - 9999
   UINT8     Month;             // 1 - 12
   UINT8     Day;               // 1 - 31
   UINT8     Hour;              // 0 - 23
   UINT8     Minute;            // 0 - 59
   UINT8     Second;            // 0 - 59
   UINT8     Pad1;
   UINT32    Nanosecond;        // 0 - 999,999,999
   INT16     TimeZone;          // —1440 to 1440 or 2047
   UINT8     Daylight;
   UINT8     Pad2;
 }   EFI_TIME;

  typedef struct {
   UINT32    dwLength;
   UINT16    wRevision;
   UINT16    wCertificateType;
 } WIN_CERTIFICATE;

 typedef struct {
   WIN_CERTIFICATE    Hdr;
   EFI_GUID           CertType;
   UINT8              CertData[1];
 } WIN_CERTIFICATE_UEFI_GUID;

typedef struct {
   UINT8          Type;
   UINT32         IdSize;
   // UINT8       Id[IdSize];
}   EFI_VARIABLE_AUTHENTICATION_3_CERT_ID;

//******************************************************
// Variable Attributes
//******************************************************

// NOTE: This interface is deprecated and should no longer be used!
//
// EFI_VARIABLE_AUTHENTICATION descriptor
//
// A counter-based authentication method descriptor template
//
typedef struct {
  UINT64                       MonotonicCount;
  WIN_CERTIFICATE_UEFI_GUID    AuthInfo;
} EFI_VARIABLE_AUTHENTICATION;

//
// EFI_VARIABLE_AUTHENTICATION_2 descriptor
//
// A time-based authentication method descriptor template
//
typedef struct {
  EFI_TIME                      TimeStamp;
  WIN_CERTIFICATE_UEFI_GUID     AuthInfo;
} EFI_VARIABLE_AUTHENTICATION_2;

//
// EFI_VARIABLE_AUTHENTICATION_3 descriptor
//
// An extensible implementation of the Variable Authentication
// structure.
//
#define EFI_VARIABLE_AUTHENTICATION_3_TIMESTAMP_TYPE 1
#define EFI_VARIABLE_AUTHENTICATION_3_NONCE_TYPE 2

typedef struct {
  UINT8 Version;
  UINT8 Type;
  UINT32 MetadataSize;
  UINT32 Flags;
} EFI_VARIABLE_AUTHENTICATION_3;

//
// EFI_VARIABLE_AUTHENTICATION_3_NONCE descriptor
//
// A nonce-based authentication method descriptor template. This
// structure will always be followed by a
// WIN_CERTIFICATE_UEFI_GUID structure.
//
typedef struct {
  UINT32 NonceSize;
  // UINT8 Nonce[NonceSize];
}  EFI_VARIABLE_AUTHENTICATION_3_NONCE;

#define EFI_HARDWARE_ERROR_VARIABLE \
{0x414E6BDD,0xE47B,0x47cc,{0xB2,0x44,0xBB,0x61,0x02,0x0C,0xF5,0x16}}

//***************************************************
// Bit Definitions for EFI_TIME.Daylight. See below.
//***************************************************
#define EFI_TIME_ADJUST_DAYLIGHT   0x01
#define EFI_TIME_IN_DAYLIGHT       0x02

//***************************************************
// Value Definition for EFI_TIME.TimeZone. See below.
//***************************************************
#define EFI_UNSPECIFIED_TIMEZONE   0x07FF

 //******************************************************
// EFI_TIME_CAPABILITIES
//******************************************************
// This provides the capabilities of the
// real time clock device as exposed through the EFI interfaces.
typedef struct {
   UINT32                  Resolution;
   UINT32                  Accuracy;
   BOOLEAN                 SetsToZero;
}   EFI_TIME_CAPABILITIES;

//******************************************************
// EFI_OPTIONAL_PTR
//******************************************************
#define EFI_OPTIONAL_PTR          0x00000001

//******************************************************
// EFI_RESET_TYPE
//******************************************************
typedef enum {
   EfiResetCold,
   EfiResetWarm,
   EfiResetShutdown,
   EfiResetPlatformSpecific
}   EFI_RESET_TYPE;


#define EFI_OS_INDICATIONS_BOOT_TO_FW_UI                           0x0000000000000001

#define EFI_OS_INDICATIONS_TIMESTAMP_REVOCATION \                  0x0000000000000002

#define EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED         0x0000000000000004

#define EFI_OS_INDICATIONS_FMP_CAPSULE_SUPPORTED \                 0x0000000000000008

#define EFI_OS_INDICATIONS_CAPSULE_RESULT_VAR_SUPPORTED            0x0000000000000010

#define EFI_OS_INDICATIONS_START_OS_RECOVERY                       0x0000000000000020

#define EFI_OS_INDICATIONS_START_PLATFORM_RECOVERY \               0x0000000000000040   #define EFI_OS_INDICATIONS_JSON_CONFIG_DATA_REFRESH \              0x0000000000000080

typedef struct {
   UINT32               VariableTotalSize;
   UINT32               Reserved; //for alignment
   EFI_GUID             CapsuleGuid;
   EFI_TIME             CapsuleProcessed;
   EFI_STATUS           CapsuleStatus;
}   EFI_CAPSULE_RESULT_VARIABLE_HEADER;

typedef struct {
   UINT16               Version;
   UINT8                PayloadIndex;
   UINT8                UpdateImageIndex;

   EFI_GUID             UpdateImageTypeId;
   // CHAR16            CapsuleFileName [];
   // CHAR16            CapsuleTarget [];
}   EFI_CAPSULE_RESULT_VARIABLE_FMP;

typedef struct {
   UINT32 Version;
   UINT32 CapsuleId;
   UINT32 RespLength;
   UINT8 Resp[];
 } EFI_CAPSULE_RESULT_VARIABLE_JSON;



typedef struct {
UINT64                           Length;
union {
   EFI_PHYSICAL_ADDRESS          DataBlock;
   EFI_PHYSICAL_ADDRESS          ContinuationPointer;
 }Union;
} EFI_CAPSULE_BLOCK_DESCRIPTOR;

typedef struct {
   UINT32                  CapsuleArrayNumber;
   VOID*                   CapsulePtr[1];
 } EFI_CAPSULE_TABLE;

typedef struct {
   EFI_GUID             CapsuleGuid;
   UINT32               HeaderSize;
   UINT32               Flags;
   UINT32               CapsuleImageSize;
 } EFI_CAPSULE_HEADER;

// {0DE9F0EC-88B6-428F-977A-258F1D0E5E72}
#define EFI_MEMORY_RANGE_CAPSULE_GUID \
   { 0xde9f0ec, 0x88b6, 0x428f, \
    { 0x97, 0x7a, 0x25, 0x8f, 0x1d, 0xe, 0x5e, 0x72 } }

typedef struct {
   EFI_PHYSICAL_ADDRESS          Address;
   UINT64                        Length;
} EFI_MEMORY_RANGE;

typedef struct {
   EFI_CAPSULE_HEADER            Header;
   UINT32                        OsRequestedMemoryType;
   UINT64                        NumberOfMemoryRanges;
   EFI_MEMORY_RANGE              MemoryRanges[];
 } EFI_MEMORY_RANGE_CAPSULE;

 typedef struct {
   UINT64               FirmwareMemoryRequirement;
   UINT64               NumberOfMemoryRanges;
} EFI_MEMORY_RANGE_CAPSULE_RESULT;














typedef
EFI_STATUS
(*EFI_GET_VARIABLE) (
  IN CHAR16           *VariableName,
  IN EFI_GUID         *VendorGuid,
  OUT UINT32          *Attributes OPTIONAL,
  IN OUT UINTN        *DataSize,
  OUT VOID            *Data OPTIONAL
 );

typedef
EFI_STATUS
(*EFI_GET_NEXT_VARIABLE_NAME) (
  IN OUT UINTN           *VariableNameSize,
  IN OUT CHAR16          *VariableName,
  IN OUT EFI_GUID        *VendorGuid
 );

 typedef
 EFI_STATUS
(*EFI_SET_VARIABLE) (
   IN CHAR16            *VariableName,
   IN EFI_GUID          *VendorGuid,
   IN UINT32            Attributes,
   IN UINTN             DataSize,
   IN VOID              *Data
);

 typedef
EFI_STATUS
(*EFI_QUERY_VARIABLE_INFO) (
   IN UINT32             Attributes,
   OUT UINT64            *MaximumVariableStorageSize,
   OUT UINT64            *RemainingVariableStorageSize,
   OUT UINT64            *MaximumVariableSize
  );



typedef
EFI_STATUS
(*EFI_GET_TIME) (
   OUT EFI_TIME                  *Time,
   OUT EFI_TIME_CAPABILITIES     *Capabilities OPTIONAL
  );

typedef
EFI_STATUS
(*EFI_SET_TIME) (
  IN EFI_TIME       *Time
 );

typedef
EFI_STATUS
(*EFI_GET_WAKEUP_TIME) (
   OUT BOOLEAN            *Enabled,
   OUT BOOLEAN            *Pending,
   OUT EFI_TIME           *Time
  );

typedef
EFI_STATUS
(*EFI_SET_WAKEUP_TIME) (
   IN BOOLEAN         Enable,
   IN EFI_TIME        *Time OPTIONAL
  );

typedef
EFI_STATUS
(*EFI_SET_VIRTUAL_ADDRESS_MAP) (
   IN UINTN                 MemoryMapSize,
   IN UINTN                 DescriptorSize,
   IN UINT32                DescriptorVersion,
   IN EFI_MEMORY_DESCRIPTOR *VirtualMap
  );

typedef
EFI_STATUS
(*EFI_CONVERT_POINTER) (
   IN UINTN             DebugDisposition,
   IN VOID              **Address
  );

typedef
VOID
(EFIAPI *EFI_RESET_SYSTEM) (
   IN EFI_RESET_TYPE          ResetType,
   IN EFI_STATUS              ResetStatus,
   IN UINTN                   DataSize,
   IN VOID                    *ResetData OPTIONAL
 );

typedef
EFI_STATUS
(*EFI_GET_NEXT_HIGH_MONO_COUNT) (
  OUT UINT32               *HighCount
 );

typedef
EFI_STATUS
(*EFI_UPDATE_CAPSULE) (
   IN EFI_CAPSULE_HEADER      **CapsuleHeaderArray,
   IN UINTN                   CapsuleCount,
   IN EFI_PHYSICAL_ADDRESS    ScatterGatherList OPTIONAL
  );


typedef
EFI_STATUS
(*EFI_QUERY_CAPSULE_CAPABILITIES) (
   IN EFI_CAPSULE_HEADER         **CapsuleHeaderArray,
   IN UINTN                      CapsuleCount,
   OUT UINT64                    *MaximumCapsuleSize,
   OUT EFI_RESET_TYPE            *ResetType
  );



typedef struct {
    EFI_TABLE_HEADER                 Hdr;

    //
    // Time Services
    //
    EFI_GET_TIME                     GetTime;
    EFI_SET_TIME                     SetTime;
    EFI_GET_WAKEUP_TIME              GetWakeupTime;
    EFI_SET_WAKEUP_TIME              SetWakeupTime;

    //
    // Virtual Memory Services
    //
    EFI_SET_VIRTUAL_ADDRESS_MAP      SetVirtualAddressMap;
  EFI_CONVERT_POINTER                ConvertPointer;

    //
    // Variable Services
    //
    EFI_GET_VARIABLE                 GetVariable;
    EFI_GET_NEXT_VARIABLE_NAME       GetNextVariableName;
    EFI_SET_VARIABLE                 SetVariable;


    //
    // Miscellaneous Services
    //
    EFI_GET_NEXT_HIGH_MONO_COUNT     GetNextHighMonotonicCount;
    EFI_RESET_SYSTEM                 ResetSystem;

   //
   // UEFI 2.0 Capsule Services
   //
   EFI_UPDATE_CAPSULE               UpdateCapsule;
   EFI_QUERY_CAPSULE_CAPABILITIES   QueryCapsuleCapabilities;


 //
 // Miscellaneous UEFI 2.0 Service
 //
  EFI_QUERY_VARIABLE_INFO          QueryVariableInfo;
} EFI_RUNTIME_SERVICES;

#endif
