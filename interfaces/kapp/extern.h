/*===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
*/

#ifndef _h_kapp_extern_
#define _h_kapp_extern_

#if ! defined EXPORT_LATCH && defined _LIBRARY

#define KAPP_EXTERN LIB_EXPORT
#define KAPP_EXTERN_DATA extern LIB_EXPORT
#define EXPORT_LATCH 1

#else

#define KAPP_EXTERN LIB_IMPORT
#define KAPP_EXTERN_DATA LIB_IMPORT

#endif

#ifndef _h_klib_extern_
#include <klib/extern.h>
#endif

#endif /* _h_kapp_extern_ */