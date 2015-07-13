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

#include "blast-mgr.h" /* BTableType */
#include "reader.h" /* Data2na */
#include "run-set.h" /* Core4na */

#include <klib/log.h> /* PLOGMSG */
#include <klib/rc.h> /* SILENT_RC */
#include <klib/status.h> /* STSMSG */

#include <vdb/blob.h> /* VBlob */
#include <vdb/cursor.h> /* VCursor */
#include <vdb/table.h> /* VTable */

#include <string.h> /* strcmp */

/******************************************************************************/

#define MAX_SEQ_LEN 5000

/******************************************************************************/

#define MAX_BIT64 (~((uint64_t)-1 >> 1))

static bool _is_set_read_id_reference_bit(uint64_t read_id) {
    return read_id & MAX_BIT64;
}

static
uint64_t _clear_read_id_reference_bit(uint64_t read_id, bool *bad)
{
    assert(bad);

    *bad = false;

    if (! _is_set_read_id_reference_bit(read_id)) {
        *bad = true;
        S
        return read_id;
    }

    return read_id & ~MAX_BIT64;
}

static uint64_t _set_read_id_reference_bit
    (uint64_t read_id, VdbBlastStatus *status)
{
    assert(status);

    if (_is_set_read_id_reference_bit(read_id)) {
        *status = eVdbBlastErr;
        S
        return read_id;
    }

    return read_id | MAX_BIT64;
}


/******************************************************************************/

typedef struct {
    BSTNode n;

    const char *acc;
} RunNode;
static int64_t CC RunNodeCmpByAcc(const void *item, const BSTNode *n) {
    const char *c = item;
    const RunNode *rn = (const RunNode*)n;

    if (c == NULL || rn == NULL || rn->acc == NULL) {
        return 1;
    }

    return strcmp(c, rn->acc);
}

static int64_t CC RunBstSortByAcc(const BSTNode *item, const BSTNode *n) {
    const RunNode *rn = (const RunNode*)item;
    assert(rn);
    return RunNodeCmpByAcc(rn->acc, n);
}

static void CC RunNodeWhack(BSTNode *n, void *ignore) {
    RunNode *rn = (RunNode*)n;
    assert(rn);
    memset(n, 0, sizeof *n);
    free(n);
}

/******************************************************************************/

struct VdbBlastRef {
    uint32_t iRun;  /* in run table */
    char *SEQ_ID;
    uint64_t first; /* spot  in REFERENCE table */
    uint64_t count; /* spots in REFERENCE table */
    bool external;  /* reference */
    bool circular;  /* reference */
    size_t base_count; /* is set just for circular references */
};
static void _VdbBlastRefWhack(VdbBlastRef *self) {
    assert(self);

    free(self->SEQ_ID);

    memset(self, 0, sizeof *self);
}

/******************************************************************************/

void _RefSetFini(RefSet *self) {
    size_t i = 0;

    if (self == NULL) {
        return;
    }

    for (i = 0; i < self->rfdk; ++i) {
        _VdbBlastRefWhack(&self->rfd[i]);
    }

    free(self->rfd);

    BSTreeWhack(&self->runs, RunNodeWhack, NULL);

    memset(self, 0, sizeof *self);
}

/******************************************************************************/

typedef struct References {
    const RunSet *rs;     /* table of runs */

    RefSet       *refs;

    size_t        rfdi; /* refs member being read */
    size_t        spot; /* next spot to be read in refs member being read */
    bool      circular; /* for circular references
                         - if true than the spot is provided the second time */

    const VCursor *curs; /* to REFERENCE table of current refs member ([rfdi])*/
    uint32_t   idxREAD; /* index of READ column in VCursor */
    uint64_t   read_id;
    bool           eos; /* end if set: no more sequences to read */
} References;
void _ReferencesWhack(const References *cself) {
    References *self = (References *)cself;

    if (self == NULL) {
        return;
    }

    VCursorRelease(self->curs);

    memset(self, 0, sizeof *self);

    free(self);
}

const References* _RunSetMakeReferences
    (RunSet *self, VdbBlastStatus *status)
{
    rc_t rc = 0;
    uint32_t i = 0;
    References *r = NULL;
    RefSet *refs = NULL;
    const VCursor *c = NULL;
    uint32_t iREAD_LEN = 0;
    assert(self && status);
    refs = &self->refs;
    r = calloc(1, sizeof *r);
    if (r == NULL) {
        *status = eVdbBlastMemErr;
        return NULL;
    }
    assert(!refs->rfd);
    refs->rfdn = 1;
    refs->rfd = calloc(1, refs->rfdn * sizeof *refs->rfd);
    if (refs->rfd == NULL) {
        *status = eVdbBlastMemErr;
        return NULL;
    }
    BSTreeInit(&refs->runs);
    r->rs = self;
    r->refs = refs;
    for (i = 0; i < self->krun; ++i) {
        const void *value = NULL;
        uint32_t iCIRCULAR = 0; 
        uint32_t iCMP_READ = 0;
        uint32_t iSEQ_ID   = 0;
        int64_t first = 0;
        uint64_t count = 0;
        uint64_t cur_row = 0;
        const VdbBlastRun *run = &self->run[i];
        if (run->obj == NULL || run->obj->db == NULL) {
            continue;
        }
        {
            RunNode *n = NULL;
            n = (RunNode*)BSTreeFind(&refs->runs, run->acc, RunNodeCmpByAcc);
            if (n != NULL) {
                continue; /* ignore repeated runs */
            }
            else {
                n = calloc(1, sizeof *n);
                if (n == NULL) {
                    *status = eVdbBlastMemErr;
                    return NULL;
                }
            }
            n->acc = run->acc;
            BSTreeInsert(&refs->runs, (BSTNode*)n, RunBstSortByAcc);
        }
        if (run->obj->refTbl == NULL) {
            rc = VDatabaseOpenTableRead(run->obj->db,
                &run->obj->refTbl, "REFERENCE");
            if (rc != 0) {
                S
                continue;
            }
        }
        rc = VTableCreateCursorRead(run->obj->refTbl, &c);
        if (rc != 0) {
            S
            continue;
        }
        if (rc == 0) {
            rc = VCursorAddColumn(c, &iCIRCULAR, "CIRCULAR");
            if (rc != 0) {
                S
            }
        }
        if (rc == 0) {
            rc = VCursorAddColumn(c, &iCMP_READ, "CMP_READ");
            if (rc != 0) {
                S
            }
        }
        if (rc == 0) {
            rc = VCursorAddColumn(c, &iREAD_LEN, "READ_LEN");
            if (rc != 0) {
                S
            }
        }
        if (rc == 0) {
            rc = VCursorAddColumn(c, &iSEQ_ID, "SEQ_ID");
            if (rc != 0) {
                S
            }
        }
        if (rc == 0) {
            rc = VCursorOpen(c);
            if (rc != 0) {
                S
            }
        }
        if (rc == 0) {
            rc = VCursorIdRange(c, iSEQ_ID, &first, &count);
            if (rc != 0) {
                S
            }
        }

        for (cur_row = first;
            cur_row < first + count && rc == 0; ++cur_row)
        {
            bool next = false;

            const void *base = NULL;
            uint32_t elem_bits = 0, boff = 0, row_len = 0;
            char *SEQ_ID = NULL;

            rc = VCursorCellDataDirect(c, cur_row, iSEQ_ID,
                &elem_bits, &base, &boff, &row_len);
            if (rc != 0 || elem_bits != 8 || boff != 0) {
                S
                break;
            }
            if (value == NULL) {
                next = true;
            }
            else if (value != base) {
                next = true;
            }

            if (next) {
                value = base;
                SEQ_ID = string_dup(base, row_len);
                if (SEQ_ID == NULL) {
                    S
                    return NULL;
                }
                if (refs->rfdk > 0 && /* there are previous references */
                    cur_row != first) /* not the first reference row in a run */
                {
                    const VdbBlastRef *rfd1 = &refs->rfd[refs->rfdk - 1];
                    if (string_cmp(rfd1->SEQ_ID, string_size(rfd1->SEQ_ID),
                        SEQ_ID, string_size(SEQ_ID), string_size(SEQ_ID)) == 0)
                    {
                     /* a SEQ_ID with a different pointer but the same value:
                       (e.g. SRR520124/REFERENCE) */
                        free((void*)SEQ_ID);
                        SEQ_ID = NULL;
                        next = false;
                    }
                }
            }

            if (next) {
                bool CIRCULAR = false;
                bool external = false;
                VdbBlastRef *rfd  = NULL;
                VdbBlastRef *rfd1 = NULL;
                rc = VCursorCellDataDirect(c, cur_row, iCIRCULAR,
                    &elem_bits, &base, &boff, &row_len);
                if (rc != 0 ||
                    base == NULL || elem_bits != 8 || boff != 0)
                {
                    S
                    break;
                }
                CIRCULAR = *(bool*)base;

                rc = VCursorCellDataDirect(c, cur_row, iCMP_READ,
                    &elem_bits, &base, &boff, &row_len);
                if (rc != 0) {
                    S
                    break;
                }
                else if (base == NULL
                    || elem_bits == 0 || row_len == 0)
                {
                    external = true;
                }

                if (refs->rfdk >= refs->rfdn) {
                    void *tmp = NULL;
                    refs->rfdn *= 2;
                    tmp = realloc(refs->rfd, refs->rfdn * sizeof *refs->rfd);
                    if (tmp == NULL) {
                        *status = eVdbBlastMemErr;
                        return NULL;
                    }
                    refs->rfd = tmp;
                }

                rfd = &refs->rfd[refs->rfdk];

                if (refs->rfdk != 0) {
                    rfd1 = &refs->rfd[refs->rfdk - 1];
                    rfd1->count = cur_row - rfd1->first;
                }
                rfd->iRun     = i;
                rfd->SEQ_ID   = SEQ_ID;
                rfd->first    = cur_row;
                rfd->circular = CIRCULAR;
                rfd->external = external;

                ++refs->rfdk;
            }
        }
        refs->rfd[refs->rfdk - 1].count
            = cur_row - refs->rfd[refs->rfdk - 1].first;
    }
    *status = eVdbBlastNoErr;
    for (i = 0; i < refs->rfdk; ++i) {
        VdbBlastRef *rfd = &refs->rfd[i];
        assert(rfd);
        if (rfd->circular) {
            uint32_t read_len = 0;
            uint32_t row_len = 0;
            rc = VCursorReadDirect(c, rfd->first + rfd->count - 1,
                iREAD_LEN, 8, &read_len, 4, &row_len);
            if (rc != 0) {
                PLOGERR(klogInt, (klogInt, rc,
                    "Error in VCursorReadDirect(READ_LEN, spot=$(spot))",
                    "spot=%ld", rfd->first + rfd->count - 1));
                *status = eVdbBlastErr;
            }
            else if (row_len != 4) {
                PLOGERR(klogInt, (klogInt, rc,
                    "Bad row_len in VCursorReadDirect(READ_LEN, spot=$(spot))",
                    "spot=%ld", rfd->first + rfd->count - 1));
                *status = eVdbBlastErr;
            }
            else {
                rfd->base_count = (rfd->count - 1) * MAX_SEQ_LEN + read_len;
            }
        }
        STSMSG(1, ("%i) '%s'[%i-%i(%i)]", i, rfd->SEQ_ID,
            rfd->first, rfd->first + rfd->count - 1, rfd->count));
    }
    RELEASE(VCursor, c);
    return r;
}

/******************************************************************************/

static uint64_t _ReferencesRead2na(References *self,
    VdbBlastStatus *status, uint64_t *read_id,
    size_t *starting_base, uint8_t *buffer, size_t buffer_size)
{
    rc_t rc = 0;
    uint64_t total = 0;
    const VdbBlastRef *rfd = NULL;
    uint8_t *begin = buffer;
    assert(status && self && self->rs && read_id && starting_base);
    *status = eVdbBlastNoErr;
    assert(self->refs);
    rfd = &self->refs->rfd[self->rfdi];
    while (total < buffer_size * 4) {
        uint32_t start = 0;
        uint32_t to_read = 0;
        uint32_t num_read = 0;
        uint32_t remaining = 0;
        if (self->spot == 0 ||
           /* the very first call: open the first spot of the first reference */

            self->rfdi != self->read_id) /* should switch to a next reference */
        {
            const VdbBlastRef *rfd1 = NULL;
            const VTable *t = NULL;
            assert(!total);
            if (self->rfdi != self->read_id) {/* switching to a next reference*/
                if (self->rfdi + 1 != self->read_id) { /* should never happen */
                    *status = eVdbBlastErr;
                    S
                    return 0;
                }
                *starting_base = 0;
                ++self->rfdi;
                if (self->rfdi >= self->refs->rfdk) {
                    self->eos = true;
                    *status = eVdbBlastNoErr; /* end of set */
                    S
                    return 0;
                }
                rfd1 = rfd;
                rfd = &self->refs->rfd[self->rfdi];
            }
            if (rfd->iRun >= self->rs->krun) {
                S
                return 0;
            }
            if (self->rs->run == NULL || self->rs->run[rfd->iRun].obj == NULL ||
                self->rs->run[rfd->iRun].obj->refTbl == NULL)
            {
                S
                return 0;
            }
            if (self->rfdi == 0 || rfd1->iRun != rfd->iRun) {
                t = self->rs->run[rfd->iRun].obj->refTbl;
                RELEASE(VCursor, self->curs);
                rc = VTableCreateCursorRead(t, &self->curs);
                if (rc != 0) {
                    S
                    return 0;
                }
                rc = VCursorAddColumn(self->curs,
                    &self->idxREAD, "(INSDC:2na:packed)READ");
                if (rc != 0) {
                    S
                    return 0;
                }
                rc = VCursorOpen(self->curs);
                if (rc != 0) {
                    S
                    return 0;
                }
            }
            else {
                if (self->curs == NULL || self->idxREAD == 0) {
                    *status = eVdbBlastErr;
                    S /* should never happen */
                    return 0;
                }
            }
            if (self->spot == 0) {
                self->read_id = 0;
            }
            self->spot = rfd->first;
        }
        start = (uint32_t)*starting_base;
        to_read = (uint32_t)(buffer_size * 4 - total);
        rc = VCursorReadBitsDirect(self->curs, self->spot, self->idxREAD,
            2, start, begin, 0, to_read, &num_read, &remaining);
        total += num_read;
        *status = eVdbBlastNoErr;
        *read_id = _set_read_id_reference_bit(self->read_id, status);
        if (*status != eVdbBlastNoErr) {
            break;
        }
        if (rc != 0) {
            if (rc == SILENT_RC
                (rcVDB, rcCursor, rcReading, rcBuffer, rcInsufficient))
            {
                S
                if (num_read == 0) {
                    *status = eVdbBlastErr;
                    S /* should never happen */
                }
                else {
                    rc = 0;
                }
                *starting_base += num_read;
                break;
            }
            else {
                PLOGERR(klogInt, (klogInt, rc,
                  "Error in VCursorReadBitsDirect($(path), READ, spot=$(spot))",
                  "path=%s,spot=%ld",
                  self->rs->run[rfd->iRun].path, self->spot));
                *status = eVdbBlastErr;
                return 0;
            }
        }
        else {
            if (remaining != 0) { /* The buffer is filled. */
                S     /* There remains more data to read in the current spot. */
                *starting_base += num_read;
                break;
            }
            ++self->spot;
            if (self->spot >= rfd->first + rfd->count) {
                if (rfd->circular && ! self->circular) {
                       /* end of the first repeat of a circular sequence */
                    *status = eVdbBlastCircularSequence;
                    self->circular = true;
                    self->spot = rfd->first;
                }
                else { /* end of sequence */
                    ++self->read_id;
                }
                break;
            }
            begin += num_read / 4;
            if ((num_read % 4) != 0) {
                S
                *status = eVdbBlastErr;
                break; /* should never happen */
            }
            *starting_base = 0;
        }
    }
    return total;
}

static uint32_t _ReferencesData(References *self,
    Data2na *data, VdbBlastStatus *status,
    Packed2naRead *buffer, uint32_t buffer_length)
{
    uint32_t num_read = 0;
    assert(data && status && self && self->rs);
    *status = eVdbBlastNoErr;
    assert(self->refs);
    for (num_read = 0; num_read < buffer_length; ) {
        Packed2naRead *out = NULL;
        rc_t rc = 0;
        const VdbBlastRef *rfd = &self->refs->rfd[self->rfdi];
        int64_t first = 0;
        uint64_t count = 0;
        uint64_t last = 0;
        uint64_t i = 0;
        uint32_t elem_bits = 0;
        uint32_t row_len = 0;
        *status = eVdbBlastErr;
        assert(buffer);
        RELEASE(VBlob, data->blob);
        out = &buffer[num_read];
        assert(self->refs);
        if (self->spot == 0 ||
         /* the very first call: open the first spot of the first reference */

            self->rfdi != self->read_id) /* should switch to a next reference */
        {
            const VdbBlastRef *rfd1 = NULL;
            const VTable *t = NULL;
            if (self->rfdi != self->read_id) {/* switching to a next reference*/
                if (self->rfdi + 1 != self->read_id) { /* should never happen */
                    S
                    return 0;
                }
                ++self->rfdi;
                if (self->rfdi >= self->refs->rfdk) {
                    self->eos = true;
                    *status = eVdbBlastNoErr; /* end of set */
                    S
                    return 0;
                }
                rfd1 = rfd;
                rfd = &self->refs->rfd[self->rfdi];
            }
            if (rfd->iRun >= self->rs->krun) {
                S
                return 0;
            }
            if (self->rs->run == NULL || self->rs->run[rfd->iRun].obj == NULL ||
                self->rs->run[rfd->iRun].obj->refTbl == NULL)
            {
                S
                return 0;
            }
            if (self->rfdi == 0 || rfd1->iRun != rfd->iRun) {
                t = self->rs->run[rfd->iRun].obj->refTbl;
                RELEASE(VCursor, self->curs);
                rc = VTableCreateCursorRead(t, &self->curs);
                if (rc != 0) {
                    S
                    return 0;
                }
                rc = VCursorAddColumn(self->curs,
                    &self->idxREAD, "(INSDC:2na:packed)READ");
                if (rc != 0) {
                    S
                    return 0;
                }
                rc = VCursorOpen(self->curs);
                if (rc != 0) {
                    S
                    return 0;
                }
            }
            else {
                if (self->curs == NULL || self->idxREAD == 0) {
                    S /* should never happen */
                    return 0;
                }
            }
            if (self->spot == 0) {
                self->read_id = 0;
            }
            self->spot = rfd->first;
            data->irun = self->rfdi;
        }
        rc = VCursorGetBlobDirect(self->curs,
            &data->blob, self->spot, self->idxREAD);
        if (rc != 0) {
            S /* PLOGERR */
            return 0;
        }
        if (data->blob == NULL) {
            S
            return 0;
        }
        rc = VBlobIdRange(data->blob, &first, &count);
        if (rc != 0) {
            S /* PLOGERR */
            return 0;
        }
        if (self->spot < first || self->spot >= first + count) {
            /* requested blob b(spot) but spot < b.first || spot > b.last:
               should never happen */
            S /* PLOGERR */
            return 0;
        }
        if (first > rfd->first + rfd->count) { /* should never happen */
            S /* PLOGERR */
            return 0;
        }
        last = first + count;
        if (rfd->first + rfd->count < last) {
            last = rfd->first + rfd->count;
        }
        for (i = 0; self->spot < last; ++i, ++self->spot) {
            if (i == 0) {
                rc = VBlobCellData(data->blob, self->spot, &elem_bits,
                    (const void **)&out->starting_byte,
                    &out->offset_to_first_bit, &row_len);
                if (rc != 0 || elem_bits != 2) {
                    S /* PLOGERR */
                    return 0;
                }
                else {
                    out->length_in_bases = row_len;
                }
            }
            else if (self->spot != last - 1) {
                out->length_in_bases += row_len;
            }
            else {
                const void *base = NULL;
                rc = VBlobCellData(data->blob, self->spot,
                    &elem_bits, &base, NULL, &row_len);
                if (rc != 0 || elem_bits != 2) {
                    S /* PLOGERR */
                    return 0;
                }
                else {
                    out->length_in_bases += row_len;
                }
            }
        }
        *status = eVdbBlastNoErr;
        out->read_id = _set_read_id_reference_bit(self->read_id, status);
        if (*status != eVdbBlastNoErr) {
            break;
        }
        if (self->spot < rfd->first + rfd->count) {
            *status = eVdbBlastChunkedSequence;
        }
        else if (rfd->circular && ! self->circular) {
               /* end of the first repeat of a circular sequence */
            *status = eVdbBlastCircularSequence;
            self->circular = true;
            self->spot = rfd->first;
        }
        else { /* end of sequence */
            *status = eVdbBlastNoErr;
            ++self->read_id;
        }
        ++num_read;
        break;
    }
    return num_read;
}

/******************************************************************************/

uint64_t _Core2naReadRef(Core2na *self, VdbBlastStatus *status,
    uint64_t *read_id, uint8_t *buffer, size_t buffer_size)
{
    uint64_t num_read = 0;

    References *r = NULL;

    assert(status);

    *status = eVdbBlastNoErr;

    if (self == NULL) {
        *status = eVdbBlastErr;
        return 0;
    }

    if (self->reader.refs == NULL) { /* do not have any reference */
        self->eos = true;
        return 0;
    }

    r = (References*)(self->reader.refs);

    assert(r->refs);

    if (r->rfdi > r->refs->rfdk) {
        self->eos = true;
    }

    if (self->eos) {
        return 0;
    }

    num_read = _ReferencesRead2na(r, status, read_id,
        &self->reader.starting_base, buffer, buffer_size);

    if (num_read == 0 && *status == eVdbBlastNoErr && r->eos) {
        self->eos = true;
    }

    return num_read;
}

uint32_t _Core2naDataRef(struct Core2na *self,
    Data2na *data, VdbBlastStatus *status,
    Packed2naRead *buffer, uint32_t buffer_length)
{
    uint32_t num_read = 0;

    References *r = NULL;

    assert(status);

    *status = eVdbBlastNoErr;

    if (self == NULL) {
        *status = eVdbBlastErr;
        return 0;
    }

    if (self->reader.refs == NULL) { /* do not have any reference */
        self->eos = true;
        return 0;
    }

    r = (References*)(self->reader.refs);

    assert(r->refs);

    if (r->rfdi > r->refs->rfdk) {
        self->eos = true;
    }

    if (self->eos) {
        return 0;
    }

    num_read = _ReferencesData(r, data, status, buffer, buffer_length);
    if (num_read == 0 && *status == eVdbBlastNoErr && r->eos) {
        self->eos = true;
    }

    return num_read;
}

/******************************************************************************/

size_t _Core4naReadRef(Core4na *self, const RunSet *runs,
    uint32_t *status, uint64_t read_id, size_t starting_base,
    uint8_t *buffer, size_t buffer_length)
{
    size_t total = 0;
    uint8_t *begin = buffer;
    const VdbBlastRef *rfd = NULL;
    const VdbBlastRun *run = NULL;
    uint64_t spot = 0;

    bool circular = false;
                 /* true when returning a circular reference the second time */

    uint32_t start = 0;
    assert(status);
    if (self == NULL || runs == NULL ||
        runs->refs.rfd == NULL || runs->refs.rfdk == 0)
    {
        *status = eVdbBlastErr;
        return 0;
    }
    {
        bool bad = false;
        read_id = _clear_read_id_reference_bit(read_id, &bad);
        if (bad) {
            *status  = eVdbBlastInvalidId;
            return 0;
        }
    }
    if (read_id >= runs->refs.rfdk) {
        *status  = eVdbBlastInvalidId;
        return 0;
    }
    rfd = &runs->refs.rfd[read_id];
    *status = eVdbBlastErr;
    if (runs->run == NULL) {
        return 0;
    }
    run = &runs->run[rfd->iRun];
    if (self->curs != NULL) {
        if (self->desc.tableId != read_id) {
            VCursorRelease(self->curs);
            self->curs = NULL;
        }
    }
    if (self->curs == NULL) {
        rc_t rc = 0;
        if (rfd->iRun >= runs->krun) {
            return 0;
        }
        if (run->obj == NULL || run->obj->refTbl == NULL) {
            return 0;
        }
        rc = VTableCreateCursorRead(run->obj->refTbl, &self->curs);
        if (rc != 0) {
            S
            return 0;
        }
        rc = VCursorAddColumn(self->curs,
            &self->col_READ, "(INSDC:4na:bin)READ");
        if (rc == 0) {
            rc = VCursorOpen(self->curs);
        }
        if (rc != 0) {
            RELEASE(VCursor, self->curs);
            S
            return 0;
        }
        self->desc.tableId = read_id;
        self->desc.spot = 0;
    }
    *status = eVdbBlastNoErr;

    if (rfd->circular) {
        assert(rfd->base_count);
        if (starting_base >= rfd->base_count) {
            starting_base -= rfd->base_count;
            circular = true;
        }
    }

    spot = rfd->first + starting_base / MAX_SEQ_LEN;
    if (spot >= rfd->first + rfd->count) {
        return 0;
    }
    start = starting_base % MAX_SEQ_LEN;
    while (total < buffer_length) {
        rc_t rc = 0;
        uint32_t num_read = 0;
        uint32_t remaining = 0;
        uint32_t to_read = (uint32_t)(buffer_length - total);
        if (to_read == 0) {
            S
            break;
        }
        rc = VCursorReadBitsDirect(self->curs, spot, self->col_READ,
            8, start, begin, 0, to_read, &num_read, &remaining);
        if (rc != 0) {
            PLOGERR(klogInt, (klogInt, rc,
                "Error in VCursorReadBitsDirect($(path), READ, spot=$(spot))",
                "path=%s,spot=%ld", run->path, spot));
            *status = eVdbBlastErr;
            break;
        }
        else {
            total += num_read;
            if (total > buffer_length) {
                total = buffer_length;
            }
            if (total == buffer_length) {
                break;
            }
            begin += num_read;
            if (remaining > 0) {
            }
            else if (++spot >= rfd->first + rfd->count) {
                if (rfd->circular && ! circular) {
                    *status = eVdbBlastCircularSequence;
                }
                break; /* end of reference */
            }
            else {     /* next spot */
                start = 0;
            }
        }
    }
    return total;
}

const uint8_t* _Core4naDataRef(Core4na *self, const RunSet *runs,
    uint32_t *status, uint64_t read_id, size_t *length)
{
    const void *out = NULL;
    rc_t rc = 0;
    uint64_t i = 0;
    const VdbBlastRef *rfd = NULL;
    const VdbBlastRun *run = NULL;
    int64_t first = 0;
    uint64_t count = 0;
    uint64_t last = 0;
    assert(status);
    *status = eVdbBlastErr;
    if (length == NULL || self == NULL || runs == NULL ||
        runs->refs.rfd == NULL || runs->refs.rfdk == 0)
    {
        return NULL;
    }
    *length = 0;
    {
        bool bad = false;
        read_id = _clear_read_id_reference_bit(read_id, &bad);
        if (bad) {
            *status  = eVdbBlastInvalidId;
            return NULL;
        }
    }
    if (read_id >= runs->refs.rfdk) {
        *status  = eVdbBlastInvalidId;
        return NULL;
    }
    rfd = &runs->refs.rfd[read_id];
    if (runs->run == NULL) {
        return NULL;
    }
    run = &runs->run[rfd->iRun];
    if (self->curs != NULL) {
        if (self->desc.tableId != read_id) {
            RELEASE(VCursor, self->curs);
        }
    }
    if (self->curs == NULL) {
        rc_t rc = 0;
        if (rfd->iRun >= runs->krun) {
            return NULL;
        }
        if (run->obj == NULL || run->obj->refTbl == NULL) {
            return NULL;
        }
        rc = VTableCreateCursorRead(run->obj->refTbl, &self->curs);
        if (rc != 0) {
            S
            return NULL;
        }
        rc = VCursorAddColumn(self->curs,
            &self->col_READ, "(INSDC:4na:bin)READ");
        if (rc == 0) {
            rc = VCursorOpen(self->curs);
        }
        if (rc != 0) {
            RELEASE(VCursor, self->curs);
            S
            return NULL;
        }
        self->desc.tableId = read_id;
        self->desc.spot = 0;
    }
    if (self->blob) {
        RELEASE(VBlob, self->blob);
    }
    if (self->desc.spot == 0) {
        self->desc.spot = rfd->first;
    }
    else {
        if (self->desc.spot >= rfd->first + rfd->count) {
            *status = eVdbBlastNoErr;  /* end of reference */
            return NULL;
        }
    }
    rc = VCursorGetBlobDirect(self->curs,
        &self->blob, self->desc.spot, self->col_READ);
    if (rc != 0) {
        S /* PLOGERR */
        return 0;
    }
    if (self->blob == NULL) {
        S
        return 0;
    }
    rc = VBlobIdRange(self->blob, &first, &count);
    if (rc != 0) {
        S /* PLOGERR */
        return 0;
    }
    if (self->desc.spot < first || self->desc.spot >= first + count) {
        /* requested blob b(spot) but spot < b.first || spot > b.last:
           should never happen */
        S /* PLOGERR */
        return 0;
    }
    if (first > rfd->first + rfd->count) { /* should never happen */
        S /* PLOGERR */
        return 0;
    }
    last = first + count;
    if (rfd->first + rfd->count < last) {
        last = rfd->first + rfd->count;
    }
    {
        uint32_t row_len = 0;
        for (i = 0; self->desc.spot < last; ++i, ++self->desc.spot) {
            uint32_t elem_bits = 0;
            uint32_t offset_to_first_bit = 0;
            if (i == 0) {
                rc = VBlobCellData(self->blob, self->desc.spot, &elem_bits,
                    &out, &offset_to_first_bit, &row_len);
                if (rc != 0 || elem_bits != 8 || offset_to_first_bit != 0) {
                    S /* PLOGERR */
                    return NULL;
                }
                else {
                    *length = row_len;
                }
            }
            else if (self->desc.spot != last - 1) {
                *length += row_len;
            }
            else {
                const void *base = NULL;
                rc = VBlobCellData(self->blob, self->desc.spot,
                    &elem_bits, &base, NULL, &row_len);
                if (rc != 0 || elem_bits != 8 || offset_to_first_bit != 0) {
                    S /* PLOGERR */
                    return NULL;
                }
                else {
                    *length += row_len;
                }
            }
        }
    }
    if (self->desc.spot < rfd->first + rfd->count) {
        *status = eVdbBlastChunkedSequence;
    }
    else if (rfd->circular && ! self->desc.circular) {
        *status = eVdbBlastCircularSequence;
        self->desc.circular = true;
        self->desc.spot = rfd->first;
    }
    else {
        *status = eVdbBlastNoErr;
    }
    return out;
}