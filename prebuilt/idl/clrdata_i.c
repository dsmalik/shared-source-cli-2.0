// ==++==
//
//   
//    Copyright (c) 2006 Microsoft Corporation.  All rights reserved.
//   
//    The use and distribution terms for this software are contained in the file
//    named license.txt, which can be found in the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by the
//    terms of this license.
//   
//    You must not remove this notice, or any other, from this software.
//   
//
// ==--==


/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0366 */
//@@MIDL_FILE_HEADING(  )

#ifdef _MSC_VER
#pragma warning( disable: 4049 )  /* more than 64k source lines */
#endif


#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif // !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, IID_ICLRDataTarget,0x3E11CCEE,0xD08B,0x43e5,0xAF,0x01,0x32,0x71,0x7A,0x64,0xDA,0x03);


MIDL_DEFINE_GUID(IID, IID_ICLRDataTarget2,0x6d05fae3,0x189c,0x4630,0xa6,0xdc,0x1c,0x25,0x1e,0x1c,0x01,0xab);


MIDL_DEFINE_GUID(IID, IID_ICLRMetadataLocator,0xaa8fa804,0xbc05,0x4642,0xb2,0xc5,0xc3,0x53,0xed,0x22,0xfc,0x63);


MIDL_DEFINE_GUID(IID, IID_ICLRDataEnumMemoryRegionsCallback,0xBCDD6908,0xBA2D,0x4ec5,0x96,0xCF,0xDF,0x4D,0x5C,0xDC,0xB4,0xA4);


MIDL_DEFINE_GUID(IID, IID_ICLRDataEnumMemoryRegions,0x471c35b4,0x7c2f,0x4ef0,0xa9,0x45,0x00,0xf8,0xc3,0x80,0x56,0xf1);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



