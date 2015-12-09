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

#include <klib/rc.h>
#include <klib/out.h>
#include <klib/text.h>
#include <kproc/thread.h>
#include <xfs/xfs.h>

#include "xfs-priv.h"
#include "xlog.h"
#include "platform.h"

#include <sysalloc.h>
#include <stdlib.h>

/*  Some platform dependent headers
 */

#include "operations.h"


/*  Some useless pranks
 */

XFS_EXTERN rc_t CC XFSSecurityInit ();
XFS_EXTERN rc_t CC XFSSecurityDeinit ();

/*
 *  Virtuhai table and it's methods
 */
static rc_t XFS_DOKAN_init_v1( struct XFSControl * self );
static rc_t XFS_DOKAN_destroy_v1( struct XFSControl * self );
static rc_t XFS_DOKAN_mount_v1( struct XFSControl * self );
static rc_t XFS_DOKAN_loop_v1( struct XFSControl * self);
static rc_t XFS_DOKAN_unmount_v1( struct XFSControl * self);

static struct XFSControl_vt_v1 XFSControl_VT_V1 = {
    1,
    1,
    XFS_DOKAN_init_v1,
    XFS_DOKAN_destroy_v1,
    XFS_DOKAN_mount_v1,
    XFS_DOKAN_loop_v1,
    XFS_DOKAN_unmount_v1
};

/*  Control init.
 */
LIB_EXPORT
rc_t CC
XFSControlPlatformInit ( struct XFSControl * self )
{
    if ( self == NULL ) {
        return XFS_RC ( rcNull );
    }

    self -> vt = ( union XFSControl_vt * ) & XFSControl_VT_V1;

    return 0;
}   /* XFSControlInit () */

/*  Overloadable versions
 */
rc_t
XFS_DOKAN_init_v1( struct XFSControl * self )
{
    rc_t RCt;

    RCt = 0;

    XFSLogDbg ( "XFS_DOKAN_init()\n" );

        /*) Standard checks
         (*/
    if ( self -> Control != NULL ) {
        XFSLogDbg ( "XFS_DOKAN_init(): control is not empty\n" );
        return XFS_RC ( rcUnexpected );
    }

    if ( self -> Arguments == NULL ) {
        XFSLogDbg ( "XFS_DOKAN_init(): arguments are empty\n" );
        return XFS_RC ( rcUnexpected );
    }

    if ( XFSControlGetLabel ( self ) == NULL ) {
        RCt = XFSControlSetLabel ( self, "DOKAN" );
    }

    return RCt;
}   /* XFS_DOKAN_init() */

rc_t
XFS_DOKAN_destroy_v1( struct XFSControl * self )
{
    PDOKAN_OPTIONS Options;

    Options = NULL;

    XFSLogDbg ( "XFS_DOKAN_destroy()\n" );

    if ( self == NULL ) { 
        XFSLogDbg ( "XFS_DOKAN_destroy(): NULL self passed" );

        return XFS_RC ( rcNull );
    }

    Options = ( PDOKAN_OPTIONS ) self -> Control;

    if ( Options == NULL ) {
        XFSLogDbg ( "XFS_DOKAN_destroy(): options are empty\n" );
    }
    else {
        if ( Options -> MountPoint != NULL ) {
            free ( ( char * ) Options -> MountPoint );
            Options -> MountPoint = NULL;
        }

        free ( Options );
        self -> Control = NULL;
    }

    return 0;
}   /* XFS_DOKAN_destroy() */

static
rc_t CC
_InitDOKAN_OPERATIONS ( DOKAN_OPERATIONS ** Operations )
{
    rc_t RCt;
    DOKAN_OPERATIONS * RetOp;

    RCt = 0;
    RetOp = NULL;

    XFS_CSAN ( Operations )
    XFS_CAN ( Operations )

    RetOp = calloc ( 1, sizeof ( DOKAN_OPERATIONS ) );
    if ( RetOp == NULL ) {
        RCt = XFS_RC ( rcExhausted );
    }
    else {
        RCt = XFS_Private_InitOperations ( RetOp );
        if ( RCt == 0 ) {
            * Operations = RetOp;
        }
    }

    if ( RCt != 0 ) {
        * Operations = NULL;

        if ( RetOp != NULL ) {
            free ( RetOp );
        }
    }

    return RCt;
}   /* _InitDOKAN_OPERATIONS () */

XFS_EXTERN rc_t CC XFSPathInnerToNative (
                                WCHAR * NativePathBuffer,
                                size_t NativePathBufferSize,
                                const char * InnerPath,
                                ...
                                );

static
rc_t CC
_MakeMountPath ( const char * Inner, const WCHAR ** MountPath )
{
    rc_t RCt;
    WCHAR BF [ XFS_SIZE_64 ];
    WCHAR * Path;
    size_t SZ;

    RCt = 0;
    * BF = 0;
    Path = NULL;
    SZ = 0;

    XFS_CSAN ( MountPath )
    XFS_CAN ( Inner )
    XFS_CAN ( MountPath )

    RCt = XFSPathInnerToNative ( BF, sizeof ( BF ), Inner );
    if ( RCt == 0 ) {
        SZ = wcslen ( BF );
        if ( BF [ SZ - 1 ] == L'\\' ) {
            BF [ SZ - 1 ] = 0;
            SZ --;
        }

        Path = calloc ( SZ + 1, sizeof ( WCHAR ) );
        if ( Path == NULL ) {
            RCt = XFS_RC ( rcExhausted );
        }
        else {
            wcscpy ( Path, BF );

            * MountPath = Path;
        }
    }

    if ( RCt != 0 ) {
        * MountPath = NULL;

        if ( Path != NULL ) {
            free ( Path );
        }
    }

    return RCt;
}   /* _MakeMountPath () */

rc_t
XFS_DOKAN_mount_v1( struct XFSControl * self )
{
    rc_t RCt;
    DOKAN_OPTIONS * Options;

    RCt = 0;
    Options = NULL;

    XFSLogDbg ( "XFS_DOKAN_mount()\n" );

    if ( self == NULL ) {
        XFSLogDbg ( "ZERO self passed\n" );
        return XFS_RC ( rcNull );
    }

    if ( ( RCt = XFSSecurityInit () ) != 0 ) {
        XFSLogDbg ( "Can not initialize DOKAN security\n" );
        return RCt;
    }

        /*) Here we are allocating DOKAN options and it's global context
         (*/
    Options = calloc ( 1, sizeof ( DOKAN_OPTIONS ) );
    if ( Options == NULL ) {
        RCt = XFS_RC ( rcNull );
    }
    else {

        Options -> Version = DOKAN_VERSION;
        Options -> ThreadCount = 0; /* Default Value */
        Options -> Options = 0L;
        Options -> Options |= DOKAN_OPTION_KEEP_ALIVE;
        Options -> Options |= DOKAN_OPTION_DEBUG;
            /*) using Peer as GlobalContext as for FUSE implementation
             (*/
        Options -> GlobalContext = ( ULONG64 )( self -> TreeDepot );

        RCt = _MakeMountPath (
                            XFSControlGetMountPoint ( self ),
                            & ( Options -> MountPoint )
                            );
        if ( RCt == 0 ) {
            self -> Control = Options;
        }
    }

    if ( RCt != 0 ) {
        self -> Control = NULL;

        if ( Options != NULL ) {
            if ( Options -> MountPoint != NULL ) {\
                free ( ( char * ) Options -> MountPoint );
                Options -> MountPoint = NULL;
            }
            free ( Options );
        }
    }

    return RCt;
}   /* XFS_DOKAN_mount() */

rc_t
XFS_DOKAN_loop_v1( struct XFSControl * self )
{
    rc_t RCt;
    DOKAN_OPTIONS * Options;
    DOKAN_OPERATIONS * Operations;
    const struct XFSTree * Tree;

    RCt = 0;
    Operations = NULL;
    Options = NULL;
    Tree = NULL;

    XFSLogDbg ( "XFS_DOKAN_loop()\n" );

    if ( self == NULL ) {
        XFSLogDbg ( "XFSControl: ZERO self passed\n" );
        return XFS_RC ( rcNull );
    }

    if ( self -> TreeDepot == NULL ) {
        XFSLogDbg ( "XFSControl: ZERO passed\n" );
        return XFS_RC ( rcNull );
    }

    RCt = XFSControlGetTree ( self, & Tree );
    if ( RCt != 0 || Tree == NULL ) {
        XFSLogDbg ( "XFSControl: ZERO Tree DATA passed\n" );
        return XFS_RC ( rcNull );
    }

    Options = ( DOKAN_OPTIONS * ) self -> Control;
    if ( Options == NULL ) {
        XFSLogDbg ( "XFSControl: ZERO options passed\n" );
        return XFS_RC ( rcNull );
    }

XFSLogDbg ( "XFS_DOKAN_loop(): Tree [0x%p] Data [0x%p]\n",  self -> TreeDepot, Tree );


/*  We will split mount method for mount'n'loop later, so there is 
    usual routine stuff
*/

    RCt = _InitDOKAN_OPERATIONS ( & Operations );
    if ( RCt == 0 ) {
            /*)
             /  There we are running DokanMain
            (*/
        switch ( DokanMain ( Options, Operations ) ) {
            case DOKAN_SUCCESS :
                XFSLogDbg ( "DokanMain() : general success\n" );
                break;
            case DOKAN_ERROR :
                XFSLogDbg ( "DokanMain() : general error\n" );
                RCt = XFS_RC ( rcError );
                break;
            case DOKAN_DRIVE_LETTER_ERROR :
                XFSLogDbg ( "DokanMain() : bad drive letter\n" );
                RCt = XFS_RC ( rcError );
                break;
            case DOKAN_DRIVER_INSTALL_ERROR :
                XFSLogDbg ( "DokanMain() : can't install driver\n" );
                RCt = XFS_RC ( rcError );
                break;
            case DOKAN_START_ERROR :
                XFSLogDbg ( "DokanMain() : can't start, something wrong\n" );
                RCt = XFS_RC ( rcError );
                break;
            case DOKAN_MOUNT_ERROR :
                XFSLogDbg ( "DokanMain() : can't assigh a drive letter or mount point\n" );
                RCt = XFS_RC ( rcError );
                break;
            case DOKAN_MOUNT_POINT_ERROR :
                XFSLogDbg ( "DokanMain() : mount point is invalid\n" );
                RCt = XFS_RC ( rcError );
                break;
            default :
                XFSLogDbg ( "DokanMain() : something wrong happens\n" );
                RCt = XFS_RC ( rcError );
                break;
        }

        free ( Operations );
    }

XFSLogDbg ( "XFS_DOKAN_loop(): Exited Tree [0x%p]\n",  self -> TreeDepot );

    return RCt;
}   /* XFS_DOKAN_loop() */

rc_t
XFS_DOKAN_unmount_v1( struct XFSControl * self )
{
    rc_t RCt = 0;

    if ( self == NULL ) {
        XFSLogDbg ( "ZERO self passed\n" );
        /*
        return XFS_RC ( rcNull );
        */
        return 0;
    }

    if ( self -> Control == NULL ) {
        XFSLogDbg ( "ZERO self passed\n" );
        /*
        return XFS_RC ( rcNull );
        */
        return 0;
    }

    XFSSecurityDeinit ();

    return 0;
}   /* XFS_DOKAN_unmount() */

