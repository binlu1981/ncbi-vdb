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

#define KROWSET_IT KRowSetSimpleIterator

#include <kdb/rowset.h>
#include <kdb/rowset-impl.h>
#include <kfc/ctx.h>
#include <kfc/except.h>
#include <kfc/xc.h>
#include <klib/out.h>

#include <string.h>
#include <assert.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

/*--------------------------------------------------------------------------
 * forwards
 */
typedef struct KRowSetSimpleData KRowSetSimpleData;
typedef struct KRowSetSimpleIterator KRowSetSimpleIterator;

static
KRowSetSimpleIterator * CC KRowSetSimpleGetIterator ( const struct KRowSet * self, ctx_t ctx );

/*--------------------------------------------------------------------------
 * KRowSet
 */

struct KRowSetSimpleData
{
    size_t size;
    uint64_t num_rows;
    int64_t rows_array[1];
};

static
KRowSetSimpleData * KRowSetSimpleAllocateData ( ctx_t ctx, size_t size )
{
    FUNC_ENTRY ( ctx, rcDB, rcRowSet, rcAllocating );
    KRowSetSimpleData * data;

    if ( size <= 0 )
        INTERNAL_ERROR ( xcParamInvalid, "data size should be greater than zero" );
    else
    {
        data = malloc ( sizeof *data + (size - 1) * sizeof(int64_t) );
        if ( data == NULL )
            SYSTEM_ERROR ( xcNoMemory, "out of memory" );
        else
        {
            data -> size = size;
            data -> num_rows = 0;
            return data;
        }
    }

    return NULL;
}

static
KRowSetSimpleData * GetRowSetSimpleData ( KRowSet *self, ctx_t ctx )
{
    if ( self == NULL || self -> data == NULL )
    {
        FUNC_ENTRY ( ctx, rcDB, rcRowSet, rcAccessing );
        INTERNAL_ERROR ( xcSelfNull, "failed to get rowset data" );
        return NULL;
    }

    return self -> data;
}

static
void CC KRowSetSimpleDestroyData ( void *data, ctx_t ctx )
{
    free ( data );
}

static
bool CC KRowSetSimpleHasRowId ( const KRowSet * self, ctx_t ctx, int64_t row_id )
{
    FUNC_ENTRY ( ctx, rcDB, rcRowSet, rcSearching );
    const KRowSetSimpleData * data;

    TRY ( data = GetRowSetSimpleData ( (KRowSet *)self, ctx ) )
    {
        uint64_t i;
        for ( i = 0; i < data -> num_rows; ++i )
        {
            if ( data -> rows_array[i] == row_id )
                return true;
        }
    }
    return false;
}

static
void AppendRowId ( KRowSet *self, ctx_t ctx, int64_t row_id )
{
    FUNC_ENTRY ( ctx, rcDB, rcRowSet, rcInserting );
    KRowSetSimpleData * data;
    TRY ( data = GetRowSetSimpleData ( self, ctx ) )
    {
        if ( data -> size - data -> num_rows == 0 )
        {
            KRowSetSimpleData * old_data = data;
            size_t new_size;
            if ( data -> size == SIZE_MAX )
            {
                INTERNAL_ERROR ( xcIntegerOutOfBounds, "cannot allocate bigger simple container for row id insertion" );
                return;
            }
            if ( SIZE_MAX - data -> size > data -> size )
                new_size = data -> size * 2;
            else
                new_size = SIZE_MAX;

            TRY ( data = KRowSetSimpleAllocateData ( ctx, new_size ) )
            {
                memcpy ( &data -> rows_array, &old_data -> rows_array, old_data -> num_rows * sizeof(int64_t) );
                data -> num_rows = old_data -> num_rows;
                self -> data = data;
                ON_FAIL ( KRowSetSimpleDestroyData ( old_data, ctx ) )
                    return;
            }
            else
                return;
        }
        data -> rows_array[ data -> num_rows++ ] = row_id;
    }
}

static
void CC KRowSetSimpleAddRowIdRange ( KRowSet *self, ctx_t ctx, int64_t start_row_id, uint64_t count )
{
    FUNC_ENTRY ( ctx, rcDB, rcRowSet, rcInserting );
    uint64_t i;

    if ( (uint64_t)INT64_MAX - start_row_id < count )
    {
        USER_ERROR ( xcIntegerOutOfBounds, "bad row id range" );
        return;
    }


    for ( i = 0; i < count; ++i )
    {
        int64_t row_id = i + start_row_id;
        bool row_exists;
        TRY ( row_exists = KRowSetSimpleHasRowId ( self, ctx, row_id ) )
        {
            if ( row_exists )
                continue;

            ON_FAIL ( AppendRowId ( self, ctx, row_id ) )
                break;
        }
    }
}

static KRowSet_vt vtKRowSetSimple =
{
    1, 0,

    KRowSetSimpleDestroyData,
    KRowSetSimpleAddRowIdRange,
    KRowSetSimpleHasRowId,
    KRowSetSimpleGetIterator
};

KDB_EXTERN KRowSet * CC KTableMakeRowSetSimple ( struct KTable const * self, ctx_t ctx )
{
    FUNC_ENTRY ( ctx, rcDB, rcRowSet, rcConstructing );
    KRowSet *r;

    r = calloc ( 1, sizeof *r );
    if ( r == NULL )
        SYSTEM_ERROR ( xcNoMemory, "out of memory" );
    else
    {
        TRY ( KRowSetInit ( r, ctx, ( const KRowSet_vt * ) &vtKRowSetSimple, "KRowSetSimple", "KRowSetSimple" ) )
        {
            TRY ( r->data = KRowSetSimpleAllocateData ( ctx, 16 ) )
            {
                return r;
            }
        }
        free ( r );
    }

    return NULL;
}

/*--------------------------------------------------------------------------
 * KRowSetSimpleIterator
 */

struct KRowSetSimpleIterator
{
    KRowSetIterator dad;
    const KRowSet * rowset;
    const KRowSetSimpleData * rowset_data;
    int64_t array_index;
};

static
void CC KRowSetSimpleIteratorDestroy ( KRowSetSimpleIterator *self, ctx_t ctx )
{
    if ( self == NULL || self -> rowset == NULL )
    {
        FUNC_ENTRY ( ctx, rcDB, rcIterator, rcDestroying );
        INTERNAL_ERROR ( xcSelfNull, "failed to destroy rowset iterator" );
    }
    else
    {
        KRowSetRelease ( self -> rowset, ctx );
    }
}

static
void CC KRowSetSimpleIteratorNext ( struct KRowSetSimpleIterator * self, ctx_t ctx )
{
    if ( self == NULL )
    {
        FUNC_ENTRY ( ctx, rcDB, rcIterator, rcPositioning );
        INTERNAL_ERROR ( xcSelfNull, "failed to move rowset iterator" );
    }
    else
        ++self -> array_index;
}

static
bool CC KRowSetSimpleIteratorIsValid ( const struct KRowSetSimpleIterator * self )
{
    if (self == NULL || self -> rowset_data == NULL || self -> rowset == NULL || self -> rowset_data != self -> rowset -> data )
        return false;

    return self -> array_index >= 0 && self -> array_index < self -> rowset_data -> num_rows;
}

static
int64_t CC KRowSetSimpleIteratorGetRowId ( const struct KRowSetSimpleIterator * self, ctx_t ctx )
{
    if ( !KRowSetSimpleIteratorIsValid ( self ) )
    {
        FUNC_ENTRY ( ctx, rcDB, rcIterator, rcAccessing );
        if (self == NULL || self -> rowset_data == NULL || self -> rowset == NULL )
            INTERNAL_ERROR ( self == NULL ? xcSelfNull : xcSelfInvalid, "cannot get row id from iterator" );
        else if ( self -> rowset_data != self -> rowset -> data )
            USER_ERROR ( xcSelfInvalid, "usage of iterator after modifying rowset" );
        else
            USER_ERROR ( xcSelfInvalid, "usage of iterator beyond rowset range" );
        return -1;
    }
    return self -> rowset_data -> rows_array[self -> array_index];
}

static KRowSetIterator_vt vtKRowSetIteratorSimple =
{
    1, 0,

    KRowSetSimpleIteratorDestroy,
    KRowSetSimpleIteratorNext,
    KRowSetSimpleIteratorIsValid,
    KRowSetSimpleIteratorGetRowId
};

static
KRowSetSimpleIterator * CC KRowSetSimpleGetIterator ( const struct KRowSet * self, ctx_t ctx )
{
    FUNC_ENTRY ( ctx, rcDB, rcIterator, rcConstructing );

    if ( self == NULL || self -> data == NULL )
        INTERNAL_ERROR ( xcSelfNull, "failed to get rowset data" );
    else
    {
        KRowSetSimpleIterator *r;
        r = calloc ( 1, sizeof *r );
        if ( r == NULL )
            SYSTEM_ERROR ( xcNoMemory, "out of memory" );
        else
        {
            TRY ( KRowSetIteratorInit ( &r -> dad, ctx, ( const KRowSetIterator_vt * ) &vtKRowSetIteratorSimple, "KRowSetSimpleIterator", "KRowSetSimpleIterator" ) )
            {
                r -> rowset = self;
                r -> rowset_data = self -> data;
                r -> array_index = 0;
                TRY ( KRowSetAddRef ( r -> rowset, ctx ) )
                {
                    return r;
                }
            }
            free ( r );
        }
    }

    return NULL;
}
