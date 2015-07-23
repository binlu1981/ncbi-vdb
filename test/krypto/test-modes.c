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

#include <kapp/main.h>
#include <kapp/args.h>

#include <krypto/ciphermgr.h>
#include <krypto/cipher.h>

#include <klib/log.h>
#include <klib/out.h>
#include <klib/status.h>
#include <klib/rc.h>
#include <klib/defs.h>


#include <string.h>

/*
  http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf

  http://www.inconteam.com/software-development/41-encryption/55-aes-test-vectors
*/

uint8_t AES_ECB_128_key[16] =
{
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

uint8_t  AES_ECB_128_test[4][16] =
{
    {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
    },
    {
        0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
        0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51
    },
    {
        0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
        0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef
    },
    {
        0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
        0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
    }
};

uint8_t  AES_ECB_128_cipher[4][16] =
{
    {
        0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60,
        0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97
    },
    {
        0xf5, 0xd3, 0xd5, 0x85, 0x03, 0xb9, 0x69, 0x9d,
        0xe7, 0x85, 0x89, 0x5a, 0x96, 0xfd, 0xba, 0xaf
    },
    {
        0x43, 0xb1, 0xcd, 0x7f, 0x59, 0x8e, 0xce, 0x23,
        0x88, 0x1b, 0x00, 0xe3, 0xed, 0x03, 0x06, 0x88
    },
    {
        0x7b, 0x0c, 0x78, 0x5e, 0x27, 0xe8, 0xad, 0x3f,
        0x82, 0x23, 0x20, 0x71, 0x04, 0x72, 0x5d, 0xd4
    }
};

uint8_t AES_ECB_192_key[24] =
{
    0x8e, 0x73, 0xb0, 0xf7, 0xda, 0x0e, 0x64, 0x52,
    0xc8, 0x10, 0xf3, 0x2b, 0x80, 0x90, 0x79, 0xe5,
    0x62, 0xf8, 0xea, 0xd2, 0x52, 0x2c, 0x6b, 0x7b
};

uint8_t  AES_ECB_192_test[4][16] =
{
    {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
    },
    {
        0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
        0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51
    },
    {
        0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
        0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef
    },
    {
        0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
        0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
    }
};

uint8_t  AES_ECB_192_cipher[4][16] =
{
    {
        0xbd, 0x33, 0x4f, 0x1d, 0x6e, 0x45, 0xf2, 0x5f,
        0xf7, 0x12, 0xa2, 0x14, 0x57, 0x1f, 0xa5, 0xcc
    },
    {
        0x97, 0x41, 0x04, 0x84, 0x6d, 0x0a, 0xd3, 0xad,
        0x77, 0x34, 0xec, 0xb3, 0xec, 0xee, 0x4e, 0xef
    },
    {
        0xef, 0x7a, 0xfd, 0x22, 0x70, 0xe2, 0xe6, 0x0a,
        0xdc, 0xe0, 0xba, 0x2f, 0xac, 0xe6, 0x44, 0x4e
    },
    {
        0x9a, 0x4b, 0x41, 0xba, 0x73, 0x8d, 0x6c, 0x72,
        0xfb, 0x16, 0x69, 0x16, 0x03, 0xc1, 0x8e, 0x0e
    }
};
 
uint8_t AES_ECB_256_key[32] = 
{
    0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
    0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
    0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
    0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
};

uint8_t AES_ECB_256_test[4][16] = 
{
    {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
    },
    {
        0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
        0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51
    },
    {   
        0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
        0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef
    },
    {
        0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
        0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
    }
};

uint8_t AES_ECB_256_cipher[4][16] = 
{
    {
        0xf3, 0xee, 0xd1, 0xbd, 0xb5, 0xd2, 0xa0, 0x3c,
        0x06, 0x4b, 0x5a, 0x7e, 0x3d, 0xb1, 0x81, 0xf8
    },
    {
        0x59, 0x1c, 0xcb, 0x10, 0xd4, 0x10, 0xed, 0x26,
        0xdc, 0x5b, 0xa7, 0x4a, 0x31, 0x36, 0x28, 0x70
    },
    {
        0xb6, 0xed, 0x21, 0xb9, 0x9c, 0xa6, 0xf4, 0xf9,
        0xf1, 0x53, 0xe7, 0xb1, 0xbe, 0xaf, 0xed, 0x1d
    },
    {
        0x23, 0x30, 0x4b, 0x7a, 0x39, 0xf9, 0xf3, 0xff,
        0x06, 0x7d, 0x8d, 0x8f, 0x9e, 0x24, 0xec, 0xc7
    }
};

uint8_t AES_CBC_128_key[16] =
{
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

uint8_t AES_CBC_128_ivec[4][16] =
{
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    },
    {
        0x76, 0x49, 0xAB, 0xAC, 0x81, 0x19, 0xB2, 0x46,
        0xCE, 0xE9, 0x8E, 0x9B, 0x12, 0xE9, 0x19, 0x7D
    },
    {
        0x50, 0x86, 0xCB, 0x9B, 0x50, 0x72, 0x19, 0xEE,
        0x95, 0xDB, 0x11, 0x3A, 0x91, 0x76, 0x78, 0xB2
    },
    {
        0x73, 0xBE, 0xD6, 0xB8, 0xE3, 0xC1, 0x74, 0x3B,
        0x71, 0x16, 0xE6, 0x9E, 0x22, 0x22, 0x95, 0x16
    }
};
uint8_t AES_CBC_128_test[4][16] =
{
    {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
    },
    {
        0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
        0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51
    },
    {
        0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
        0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef
    },
    {
        0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
        0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
    }
};
uint8_t AES_CBC_128_cipher[4][16] =
{
    {
        0x76, 0x49, 0xab, 0xac, 0x81, 0x19, 0xb2, 0x46,
        0xce, 0xe9, 0x8e, 0x9b, 0x12, 0xe9, 0x19, 0x7d
    },
    {
        0x50, 0x86, 0xcb, 0x9b, 0x50, 0x72, 0x19, 0xee,
        0x95, 0xdb, 0x11, 0x3a, 0x91, 0x76, 0x78, 0xb2
    },
    {
        0x73, 0xbe, 0xd6, 0xb8, 0xe3, 0xc1, 0x74, 0x3b,
        0x71, 0x16, 0xe6, 0x9e, 0x22, 0x22, 0x95, 0x16
    },
    {
        0x3f, 0xf1, 0xca, 0xa1, 0x68, 0x1f, 0xac, 0x09,
        0x12, 0x0e, 0xca, 0x30, 0x75, 0x86, 0xe1, 0xa7
    }
};

uint8_t AES_CBC_192_key[24] =
{
    0x8e, 0x73, 0xb0, 0xf7, 0xda, 0x0e, 0x64, 0x52,
    0xc8, 0x10, 0xf3, 0x2b, 0x80, 0x90, 0x79, 0xe5,
    0x62, 0xf8, 0xea, 0xd2, 0x52, 0x2c, 0x6b, 0x7b
};

uint8_t AES_CBC_192_ivec[4][16] =
{
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    },
    {
        0x4F, 0x02, 0x1D, 0xB2, 0x43, 0xBC, 0x63, 0x3D,
        0x71, 0x78, 0x18, 0x3A, 0x9F, 0xA0, 0x71, 0xE8
    },
    {
        0xB4, 0xD9, 0xAD, 0xA9, 0xAD, 0x7D, 0xED, 0xF4,
        0xE5, 0xE7, 0x38, 0x76, 0x3F, 0x69, 0x14, 0x5A
    },
    {
        0x57, 0x1B, 0x24, 0x20, 0x12, 0xFB, 0x7A, 0xE0,
        0x7F, 0xA9, 0xBA, 0xAC, 0x3D, 0xF1, 0x02, 0xE0
    }
};
uint8_t AES_CBC_192_test[4][16] =
{
    {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
    },
    {
        0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
        0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51
    },
    {
        0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
        0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef
    },
    {
        0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
        0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
    }
};
uint8_t AES_CBC_192_cipher[4][16] =
{
    {
        0x4f, 0x02, 0x1d, 0xb2, 0x43, 0xbc, 0x63, 0x3d,
        0x71, 0x78, 0x18, 0x3a, 0x9f, 0xa0, 0x71, 0xe8
    },
    {
        0xb4, 0xd9, 0xad, 0xa9, 0xad, 0x7d, 0xed, 0xf4, 
        0xe5, 0xe7, 0x38, 0x76, 0x3f, 0x69, 0x14, 0x5a
    },
    {
        0x57, 0x1b, 0x24, 0x20, 0x12, 0xfb, 0x7a, 0xe0,
        0x7f, 0xa9, 0xba, 0xac, 0x3d, 0xf1, 0x02, 0xe0
    },
    {
        0x08, 0xb0, 0xe2, 0x79, 0x88, 0x59, 0x88, 0x81,
        0xd9, 0x20, 0xa9, 0xe6, 0x4f, 0x56, 0x15, 0xcd
    }
};

uint8_t AES_CBC_256_key[32] =
{
    0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
    0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
    0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
    0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
};

uint8_t AES_CBC_256_ivec[4][16] =
{
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    },
    {
        0xF5, 0x8C, 0x4C, 0x04, 0xD6, 0xE5, 0xF1, 0xBA,
        0x77, 0x9E, 0xAB, 0xFB, 0x5F, 0x7B, 0xFB, 0xD6
    },
    {
        0x9C, 0xFC, 0x4E, 0x96, 0x7E, 0xDB, 0x80, 0x8D,
        0x67, 0x9F, 0x77, 0x7B, 0xC6, 0x70, 0x2C, 0x7D
    },
    {
        0x39, 0xF2, 0x33, 0x69, 0xA9, 0xD9, 0xBA, 0xCF,
        0xA5, 0x30, 0xE2, 0x63, 0x04, 0x23, 0x14, 0x61
    }
};
uint8_t AES_CBC_256_test[4][16] =
{
    {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
    },
    {
        0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
        0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51
    },
    {
        0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
        0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef
    },
    {
        0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
        0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
    }
};
uint8_t AES_CBC_256_cipher[4][16] =
{
    {
        0xf5, 0x8c, 0x4c, 0x04, 0xd6, 0xe5, 0xf1, 0xba,
        0x77, 0x9e, 0xab, 0xfb, 0x5f, 0x7b, 0xfb, 0xd6
    },
    {
        0x9c, 0xfc, 0x4e, 0x96, 0x7e, 0xdb, 0x80, 0x8d,
        0x67, 0x9f, 0x77, 0x7b, 0xc6, 0x70, 0x2c, 0x7d
    },
    {
        0x39, 0xf2, 0x33, 0x69, 0xa9, 0xd9, 0xba, 0xcf,
        0xa5, 0x30, 0xe2, 0x63, 0x04, 0x23, 0x14, 0x61
    },
    {
        0xb2, 0xeb, 0x05, 0xe2, 0xc3, 0x9b, 0xe9, 0xfc,
        0xda, 0x6c, 0x19, 0x07, 0x8c, 0x6a, 0x9d, 0x1b
    }
};

/* ==================================================== */
uint8_t AES_CFB_128_key[16] =
{
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

uint8_t AES_CFB_128_ivec[4][16] =
{
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    },
    {
        0x3B, 0x3F, 0xD9, 0x2E, 0xB7, 0x2D, 0xAD, 0x20,
        0x33, 0x34, 0x49, 0xF8, 0xE8, 0x3C, 0xFB, 0x4A
    },
    {
        0xC8, 0xA6, 0x45, 0x37, 0xA0, 0xB3, 0xA9, 0x3F,
        0xCD, 0xE3, 0xCD, 0xAD, 0x9F, 0x1C, 0xE5, 0x8B
    },
    {
        0x26, 0x75, 0x1F, 0x67, 0xA3, 0xCB, 0xB1, 0x40,
        0xB1, 0x80, 0x8C, 0xF1, 0x87, 0xA4, 0xF4, 0xDF
    }
};
uint8_t AES_CFB_128_test[4][16] =
{
    {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
    },
    {
        0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
        0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51
    },
    {
        0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
        0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef
    },
    {
        0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
        0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
    }
};
uint8_t AES_CFB_128_cipher[4][16] =
{
    {
        0x3b, 0x3f, 0xd9, 0x2e, 0xb7, 0x2d, 0xad, 0x20,
        0x33, 0x34, 0x49, 0xf8, 0xe8, 0x3c, 0xfb, 0x4a
    },
    {
        0xc8, 0xa6, 0x45, 0x37, 0xa0, 0xb3, 0xa9, 0x3f,
        0xcd, 0xe3, 0xcd, 0xad, 0x9f, 0x1c, 0xe5, 0x8b
    },
    {
        0x26, 0x75, 0x1f, 0x67, 0xa3, 0xcb, 0xb1, 0x40,
        0xb1, 0x80, 0x8c, 0xf1, 0x87, 0xa4, 0xf4, 0xdf
    },
    {
        0xc0, 0x4b, 0x05, 0x35, 0x7c, 0x5d, 0x1c, 0x0e,
        0xea, 0xc4, 0xc6, 0x6f, 0x9f, 0xf7, 0xf2, 0xe6
    }
};

uint8_t AES_CFB_192_key[24] =
{
    0x8e, 0x73, 0xb0, 0xf7, 0xda, 0x0e, 0x64, 0x52,
    0xc8, 0x10, 0xf3, 0x2b, 0x80, 0x90, 0x79, 0xe5,
    0x62, 0xf8, 0xea, 0xd2, 0x52, 0x2c, 0x6b, 0x7b
};

uint8_t AES_CFB_192_ivec[4][16] =
{
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    },
    {
        0xCD, 0xC8, 0x0D, 0x6F, 0xDD, 0xF1, 0x8C, 0xAB,
        0x34, 0xC2, 0x59, 0x09, 0xC9, 0x9A, 0x41, 0x74
    },
    {
        0x67, 0xCE, 0x7F, 0x7F, 0x81, 0x17, 0x36, 0x21,
        0x96, 0x1A, 0x2B, 0x70, 0x17, 0x1D, 0x3D, 0x7A
    },
    {
        0x2E, 0x1E, 0x8A, 0x1D, 0xD5, 0x9B, 0x88, 0xB1,
        0xC8, 0xE6, 0x0F, 0xED, 0x1E, 0xFA, 0xC4, 0xC9
    }
};
uint8_t AES_CFB_192_test[4][16] =
{
    {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
    },
    {
        0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
        0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51
    },
    {
        0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11, 
        0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef
    },
    {
        0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
        0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
    }
};
uint8_t AES_CFB_192_cipher[4][16] =
{
    {
        0xcd, 0xc8, 0x0d, 0x6f, 0xdd, 0xf1, 0x8c, 0xab,
        0x34, 0xc2, 0x59, 0x09, 0xc9, 0x9a, 0x41, 0x74
    },
    {
        0x67, 0xce, 0x7f, 0x7f, 0x81, 0x17, 0x36, 0x21,
        0x96, 0x1a, 0x2b, 0x70, 0x17, 0x1d, 0x3d, 0x7a
    },
    {
        0x2e, 0x1e, 0x8a, 0x1d, 0xd5, 0x9b, 0x88, 0xb1,
        0xc8, 0xe6, 0x0f, 0xed, 0x1e, 0xfa, 0xc4, 0xc9
    },
    {
        0xc0, 0x5f, 0x9f, 0x9c, 0xa9, 0x83, 0x4f, 0xa0,
        0x42, 0xae, 0x8f, 0xba, 0x58, 0x4b, 0x09, 0xff
    }
};

uint8_t AES_CFB_256_key[32] =
{
    0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
    0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
    0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
    0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
};

uint8_t AES_CFB_256_ivec[4][16] =
{
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    },
    {
        0xDC, 0x7E, 0x84, 0xBF, 0xDA, 0x79, 0x16, 0x4B,
        0x7E, 0xCD, 0x84, 0x86, 0x98, 0x5D, 0x38, 0x60
    },
    {
        0x39, 0xFF, 0xED, 0x14, 0x3B, 0x28, 0xB1, 0xC8,
        0x32, 0x11, 0x3C, 0x63, 0x31, 0xE5, 0x40, 0x7B
    },
    {
        0xDF, 0x10, 0x13, 0x24, 0x15, 0xE5, 0x4B, 0x92,
        0xA1, 0x3E, 0xD0, 0xA8, 0x26, 0x7A, 0xE2, 0xF9
    }
};
uint8_t AES_CFB_256_test[4][16] =
{
    {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
    },
    {
        0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
        0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51
    },
    {
        0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
        0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef
    },
    {
        0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
        0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
    }
};
uint8_t AES_CFB_256_cipher[4][16] =
{
    {
        0xDC, 0x7E, 0x84, 0xBF, 0xDA, 0x79, 0x16, 0x4B,
        0x7E, 0xCD, 0x84, 0x86, 0x98, 0x5D, 0x38, 0x60
    },
    {
        0x39, 0xff, 0xed, 0x14, 0x3b, 0x28, 0xb1, 0xc8,
        0x32, 0x11, 0x3c, 0x63, 0x31, 0xe5, 0x40, 0x7b
    },
    {
        0xdf, 0x10, 0x13, 0x24, 0x15, 0xe5, 0x4b, 0x92,
        0xa1, 0x3e, 0xd0, 0xa8, 0x26, 0x7a, 0xe2, 0xf9
    },
    {
        0x75, 0xa3, 0x85, 0x74, 0x1a, 0xb9, 0xce, 0xf8,
        0x20, 0x31, 0x62, 0x3d, 0x55, 0xb1, 0xe4, 0x71
    }
};
/* ==================================================== */
uint8_t AES_OFB_128_key[16] =
{
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

uint8_t AES_OFB_128_ivec[4][16] =
{
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    },
    {
        0x50, 0xFE, 0x67, 0xCC, 0x99, 0x6D, 0x32, 0xB6,
        0xDA, 0x09, 0x37, 0xE9, 0x9B, 0xAF, 0xEC, 0x60
    },
    {
        0xD9, 0xA4, 0xDA, 0xDA, 0x08, 0x92, 0x23, 0x9F,
        0x6B, 0x8B, 0x3D, 0x76, 0x80, 0xE1, 0x56, 0x74
    },
    {
        0xA7, 0x88, 0x19, 0x58, 0x3F, 0x03, 0x08, 0xE7,
        0xA6, 0xBF, 0x36, 0xB1, 0x38, 0x6A, 0xBF, 0x23
    }
};
uint8_t AES_OFB_128_test[4][16] =
{
    {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
    },
    {
        0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
        0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51
    },
    {
        0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
        0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef
    },
    {
        0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
        0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
    }
};
uint8_t AES_OFB_128_cipher[4][16] =
{
    {
        0x3b, 0x3f, 0xd9, 0x2e, 0xb7, 0x2d, 0xad, 0x20,
        0x33, 0x34, 0x49, 0xf8, 0xe8, 0x3c, 0xfb, 0x4a
    },
    {
        0x77, 0x89, 0x50, 0x8d, 0x16, 0x91, 0x8f, 0x03,
        0xf5, 0x3c, 0x52, 0xda, 0xc5, 0x4e, 0xd8, 0x25
    },
    {
        0x97, 0x40, 0x05, 0x1e, 0x9c, 0x5f, 0xec, 0xf6,
        0x43, 0x44, 0xf7, 0xa8, 0x22, 0x60, 0xed, 0xcc
    },
    {
        0x30, 0x4c, 0x65, 0x28, 0xf6, 0x59, 0xc7, 0x78,
        0x66, 0xa5, 0x10, 0xd9, 0xc1, 0xd6, 0xae, 0x5e
    }
};

uint8_t AES_OFB_192_key[24] =
{
    0x8e, 0x73, 0xb0, 0xf7, 0xda, 0x0e, 0x64, 0x52,
    0xc8, 0x10, 0xf3, 0x2b, 0x80, 0x90, 0x79, 0xe5,
    0x62, 0xf8, 0xea, 0xd2, 0x52, 0x2c, 0x6b, 0x7b
};

uint8_t AES_OFB_192_ivec[4][16] =
{
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    },
    {
        0xA6, 0x09, 0xB3, 0x8D, 0xF3, 0xB1, 0x13, 0x3D,
        0xDD, 0xFF, 0x27, 0x18, 0xBA, 0x09, 0x56, 0x5E
    },
    {
        0x52, 0xEF, 0x01, 0xDA, 0x52, 0x60, 0x2F, 0xE0,
        0x97, 0x5F, 0x78, 0xAC, 0x84, 0xBF, 0x8A, 0x50
    },
    {
        0xBD, 0x52, 0x86, 0xAC, 0x63, 0xAA, 0xBD, 0x7E,
        0xB0, 0x67, 0xAC, 0x54, 0xB5, 0x53, 0xF7, 0x1D
    }
};
uint8_t AES_OFB_192_test[4][16] =
{
    {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
    },
    {
        0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
        0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51
    },
    {
        0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
        0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef
    },
    {
        0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
        0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
    }
};
uint8_t AES_OFB_192_cipher[4][16] =
{
    {
        0xcd, 0xc8, 0x0d, 0x6f, 0xdd, 0xf1, 0x8c, 0xab,
        0x34, 0xc2, 0x59, 0x09, 0xc9, 0x9a, 0x41, 0x74
    },
    {
        0xfc, 0xc2, 0x8b, 0x8d, 0x4c, 0x63, 0x83, 0x7c,
        0x09, 0xe8, 0x17, 0x00, 0xc1, 0x10, 0x04, 0x01
    },
    {
        0x8d, 0x9a, 0x9a, 0xea, 0xc0, 0xf6, 0x59, 0x6f,
        0x55, 0x9c, 0x6d, 0x4d, 0xaf, 0x59, 0xa5, 0xf2
    },
    {
        0x6d, 0x9f, 0x20, 0x08, 0x57, 0xca, 0x6c, 0x3e,
        0x9c, 0xac, 0x52, 0x4b, 0xd9, 0xac, 0xc9, 0x2a
    }
};

uint8_t AES_OFB_256_key[32] =
{
    0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
    0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
    0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
    0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
};

uint8_t AES_OFB_256_ivec[4][16] =
{
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    },
    {
        0xB7, 0xBF, 0x3A, 0x5D, 0xF4, 0x39, 0x89, 0xDD,
        0x97, 0xF0, 0xFA, 0x97, 0xEB, 0xCE, 0x2F, 0x4A
    },
    {
        0xE1, 0xC6, 0x56, 0x30, 0x5E, 0xD1, 0xA7, 0xA6,
        0x56, 0x38, 0x05, 0x74, 0x6F, 0xE0, 0x3E, 0xDC
    },
    {
        0x41, 0x63, 0x5B, 0xE6, 0x25, 0xB4, 0x8A, 0xFC,
        0x16, 0x66, 0xDD, 0x42, 0xA0, 0x9D, 0x96, 0xE7
    }
};
uint8_t AES_OFB_256_test[4][16] =
{
    {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
    },
    {
        0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
        0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51
    },
    {
        0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
        0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef
    },
    {
        0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
        0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
    }
};
uint8_t AES_OFB_256_cipher[4][16] =
{
    {
        0xdc, 0x7e, 0x84, 0xbf, 0xda, 0x79, 0x16, 0x4b,
        0x7e, 0xcd, 0x84, 0x86, 0x98, 0x5d, 0x38, 0x60
    },
    {
        0x4f, 0xeb, 0xdc, 0x67, 0x40, 0xd2, 0x0b, 0x3a,
        0xc8, 0x8f, 0x6a, 0xd8, 0x2a, 0x4f, 0xb0, 0x8d
    },
    {
        0x71, 0xab, 0x47, 0xa0, 0x86, 0xe8, 0x6e, 0xed,
        0xf3, 0x9d, 0x1c, 0x5b, 0xba, 0x97, 0xc4, 0x08
    },
    {
        0x01, 0x26, 0x14, 0x1d, 0x67, 0xf3, 0x7b, 0xe8,
        0x53, 0x8f, 0x5a, 0x8b, 0xe7, 0x40, 0xe4, 0x84
    }
};

/* ==================================================== */
uint8_t AES_CTR_128_key[16] =
{
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

uint8_t AES_CTR_128_ivec[16] =
{
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
uint8_t AES_CTR_128_test[4][16] =
{
    {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
    },
    {
        0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
        0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51
    },
    {
        0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
        0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef
    },
    {
        0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
        0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
    }
};
uint8_t AES_CTR_128_cipher[4][16] =
{
    {
        0x87, 0x4d, 0x61, 0x91, 0xb6, 0x20, 0xe3, 0x26,
        0x1b, 0xef, 0x68, 0x64, 0x99, 0x0d, 0xb6, 0xce
    },
    {
        0x98, 0x06, 0xf6, 0x6b, 0x79, 0x70, 0xfd, 0xff,
        0x86, 0x17, 0x18, 0x7b, 0xb9, 0xff, 0xfd, 0xff
    },
    {
        0x5a, 0xe4, 0xdf, 0x3e, 0xdb, 0xd5, 0xd3, 0x5e,
        0x5b, 0x4f, 0x09, 0x02, 0x0d, 0xb0, 0x3e, 0xab
    },
    {
        0x1e, 0x03, 0x1d, 0xda, 0x2f, 0xbe, 0x03, 0xd1,
        0x79, 0x21, 0x70, 0xa0, 0xf3, 0x00, 0x9c, 0xee
    }
};

uint8_t AES_CTR_192_key[24] =
{
    0x8e, 0x73, 0xb0, 0xf7, 0xda, 0x0e, 0x64, 0x52,
    0xc8, 0x10, 0xf3, 0x2b, 0x80, 0x90, 0x79, 0xe5,
    0x62, 0xf8, 0xea, 0xd2, 0x52, 0x2c, 0x6b, 0x7b
};

uint8_t AES_CTR_192_ivec[16] =
{
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
uint8_t AES_CTR_192_test[4][16] =
{
    {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
    },
    {
        0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
        0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51
    },
    {
        0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
        0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef
    },
    {
        0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
        0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
    }
};
uint8_t AES_CTR_192_cipher[4][16] =
{
    {
        0x1a, 0xbc, 0x93, 0x24, 0x17, 0x52, 0x1c, 0xa2,
        0x4f, 0x2b, 0x04, 0x59, 0xfe, 0x7e, 0x6e, 0x0b
    },
    {
        0x09, 0x03, 0x39, 0xec, 0x0a, 0xa6, 0xfa, 0xef,
        0xd5, 0xcc, 0xc2, 0xc6, 0xf4, 0xce, 0x8e, 0x94
    },
    {
        0x1e, 0x36, 0xb2, 0x6b, 0xd1, 0xeb, 0xc6, 0x70,
        0xd1, 0xbd, 0x1d, 0x66, 0x56, 0x20, 0xab, 0xf7
    },
    {
        0x4f, 0x78, 0xa7, 0xf6, 0xd2, 0x98, 0x09, 0x58,
        0x5a, 0x97, 0xda, 0xec, 0x58, 0xc6, 0xb0, 0x50
    }
};

uint8_t AES_CTR_256_key[32] =
{
    0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
    0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
    0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
    0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
};

uint8_t AES_CTR_256_ivec[16] =
{
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
uint8_t AES_CTR_256_test[4][16] =
{
    {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
    },
    {
        0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
        0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51
    },
    {
        0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
        0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef
    },
    {
        0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
        0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
    }
};
uint8_t AES_CTR_256_cipher[4][16] =
{
    {
        0x60, 0x1e, 0xc3, 0x13, 0x77, 0x57, 0x89, 0xa5,
        0xb7, 0xa7, 0xf5, 0x04, 0xbb, 0xf3, 0xd2, 0x28
    },
    {   
        0xf4, 0x43, 0xe3, 0xca, 0x4d, 0x62, 0xb5, 0x9a,
        0xca, 0x84, 0xe9, 0x90, 0xca, 0xca, 0xf5, 0xc5
    },
    {   
        0x2b, 0x09, 0x30, 0xda, 0xa2, 0x3d, 0xe9, 0x4c,
        0xe8, 0x70, 0x17, 0xba, 0x2d, 0x84, 0x98, 0x8d
    },
    {   
        0xdf, 0xc9, 0xc5, 0x8d, 0xb6, 0x7a, 0xad, 0xa6,
        0x13, 0xc2, 0xdd, 0x08, 0x45, 0x79, 0x41, 0xa6
    }
};
/* ==================================================== */
rc_t run ()
{
    KCipherManager * manager;
    rc_t rc;

    rc = KCipherManagerMake (&manager);
    if (rc == 0)
    {
        KCipher * cipher;

        rc = KCipherManagerMakeCipher (manager, &cipher, kcipher_AES);
        if (rc == 0)
        {
            /* AES ECB 128 */
            KOutMsg ("AES ECB 128\n");
            rc = KCipherSetEncryptKey (cipher, AES_ECB_128_key, sizeof AES_ECB_128_key);
            if (rc == 0)
            {
                rc = KCipherSetDecryptKey (cipher, AES_ECB_128_key, sizeof AES_ECB_128_key);
                if (rc == 0)
                {
                    unsigned int index;

                    for (index = 0; index < (sizeof AES_ECB_128_test / sizeof AES_ECB_128_test[0]); ++index)
                    {
                        uint8_t etemp [16];
                        uint8_t dtemp [16];

                        rc = KCipherEncryptECB (cipher, AES_ECB_128_test[index], etemp, sizeof AES_ECB_128_test[0] / sizeof etemp);
                        if (rc == 0)
                        {
                            rc = KCipherDecryptECB (cipher, AES_ECB_128_cipher[index], dtemp, sizeof AES_ECB_128_cipher[0] / sizeof dtemp);
                            if (rc == 0)
                            {
                                if (memcmp (AES_ECB_128_test[index], dtemp, sizeof dtemp) != 0)
                                    KOutMsg ("Failed AES ECB 128 encrypt #%u\n", index);
                                if (memcmp (AES_ECB_128_cipher[index], etemp, sizeof etemp) != 0)
                                    KOutMsg ("Failed AES ECB 128 decrypt test #%u\n", index);
                            }
                        }
                    }
                }
            }           

            /* AES ECB 192 */
            KOutMsg ("AES ECB 192\n");
            rc = KCipherSetEncryptKey (cipher, AES_ECB_192_key, sizeof AES_ECB_192_key);
            if (rc == 0)
            {
                rc = KCipherSetDecryptKey (cipher, AES_ECB_192_key, sizeof AES_ECB_192_key);
                if (rc == 0)
                {
                    unsigned int index;

                    for (index = 0; index < (sizeof AES_ECB_192_test / sizeof AES_ECB_192_test[0]); ++index)
                    {
                        uint8_t etemp [16];
                        uint8_t dtemp [16];

                        rc = KCipherEncryptECB (cipher, AES_ECB_192_test[index], etemp, sizeof AES_ECB_192_test[0] / sizeof etemp);
                        if (rc == 0)
                        {
                            rc = KCipherDecryptECB (cipher, AES_ECB_192_cipher[index], dtemp, sizeof AES_ECB_192_cipher[0] / sizeof dtemp);
                            if (rc == 0)
                            {
                                if (memcmp (AES_ECB_192_test[index], dtemp, sizeof dtemp) != 0)
                                    KOutMsg ("Failed AES ECB 192 encrypt #%u\n", index);
                                if (memcmp (AES_ECB_192_cipher[index], etemp, sizeof etemp) != 0)
                                    KOutMsg ("Failed AES ECB 192 decrypt test #%u\n", index);
                            }
                        }
                    }
                }
            }           

            /* AES ECB 256 */
            KOutMsg ("AES ECB 256\n");
            rc = KCipherSetEncryptKey (cipher, AES_ECB_256_key, sizeof AES_ECB_256_key);
            if (rc == 0)
            {
                rc = KCipherSetDecryptKey (cipher, AES_ECB_256_key, sizeof AES_ECB_256_key);
                if (rc == 0)
                {
                    unsigned int index;

                    for (index = 0; index < (sizeof AES_ECB_256_test / sizeof AES_ECB_256_test[0]); ++index)
                    {
                        uint8_t etemp [16];
                        uint8_t dtemp [16];

                        rc = KCipherEncryptECB (cipher, AES_ECB_256_test[index], etemp, sizeof AES_ECB_256_test[0] / sizeof etemp);
                        if (rc == 0)
                        {
                            rc = KCipherDecryptECB (cipher, AES_ECB_256_cipher[index], dtemp, sizeof AES_ECB_256_cipher[0] / sizeof dtemp);
                            if (rc == 0)
                            {
                                if (memcmp (AES_ECB_256_test[index], dtemp, sizeof dtemp) != 0)
                                    KOutMsg ("Failed AES ECB 256 encrypt #%u\n", index);
                                if (memcmp (AES_ECB_256_cipher[index], etemp, sizeof etemp) != 0)
                                    KOutMsg ("Failed AES ECB 256 decrypt test #%u\n", index);
                            }
                        }
                    }
                }
            }           


            /* AES CBC 128 */
            KOutMsg ("AES CBC 128\n");
            rc = KCipherSetEncryptKey (cipher, AES_CBC_128_key, sizeof AES_CBC_128_key);
            if (rc == 0)
            {
                rc = KCipherSetDecryptKey (cipher, AES_CBC_128_key, sizeof AES_CBC_128_key);
                if (rc == 0)
                {
                    rc = KCipherSetEncryptIVec (cipher, AES_CBC_128_ivec);
                    if (rc == 0)
                    {
                        rc = KCipherSetDecryptIVec (cipher, AES_CBC_128_ivec);
                        if (rc == 0)
                        {
                            unsigned int index;

                            for (index = 0; index < (sizeof AES_CBC_128_test / sizeof AES_CBC_128_test[0]); ++index)
                            {
                                uint8_t etemp [16];
                                uint8_t dtemp [16];

                                rc = KCipherEncryptCBC (cipher, AES_CBC_128_test[index], etemp, sizeof AES_CBC_128_test[0] / sizeof etemp);
                                if (rc == 0)
                                {
                                    rc = KCipherDecryptCBC (cipher, AES_CBC_128_cipher[index], dtemp, sizeof AES_CBC_128_cipher[0] / sizeof dtemp);
                                    if (rc == 0)
                                    {
                                        if (memcmp (AES_CBC_128_test[index], dtemp, sizeof dtemp) != 0)
                                            KOutMsg ("Failed AES CBC 128 encrypt #%u\n", index);
                                        if (memcmp (AES_CBC_128_cipher[index], etemp, sizeof etemp) != 0)
                                            KOutMsg ("Failed AES CBC 128 decrypt test #%u\n", index);
                                    }
                                }
                            }
                        }
                    }
                }
            }           

            /* AES CBC 192 */
            KOutMsg ("AES CBC 192\n");
            rc = KCipherSetEncryptKey (cipher, AES_CBC_192_key, sizeof AES_CBC_192_key);
            if (rc == 0)
            {
                rc = KCipherSetDecryptKey (cipher, AES_CBC_192_key, sizeof AES_CBC_192_key);
                if (rc == 0)
                {
                    rc = KCipherSetEncryptIVec (cipher, AES_CBC_128_ivec);
                    if (rc == 0)
                    {
                        rc = KCipherSetDecryptIVec (cipher, AES_CBC_128_ivec);
                        if (rc == 0)
                        {
                            unsigned int index;

                            for (index = 0; index < (sizeof AES_CBC_192_test / sizeof AES_CBC_192_test[0]); ++index)
                            {
                                uint8_t etemp [16];
                                uint8_t dtemp [16];

                                rc = KCipherEncryptCBC (cipher, AES_CBC_192_test[index], etemp, sizeof AES_CBC_192_test[0] / sizeof etemp );
                                if (rc == 0)
                                {
                                    rc = KCipherDecryptCBC (cipher, AES_CBC_192_cipher[index], dtemp, sizeof AES_CBC_192_cipher[0] / sizeof dtemp);
                                    if (rc == 0)
                                    {
                                        if (memcmp (AES_CBC_192_test[index], dtemp, sizeof dtemp) != 0)
                                            KOutMsg ("Failed AES CBC 192 encrypt #%u\n", index);
                                        if (memcmp (AES_CBC_192_cipher[index], etemp, sizeof etemp) != 0)
                                            KOutMsg ("Failed AES CBC 192 decrypt test #%u\n", index);
                                    }
                                }
                            }
                        }
                    }
                }
            }           

            /* AES CBC 256 */
            KOutMsg ("AES CBC 256\n");
            rc = KCipherSetEncryptKey (cipher, AES_CBC_256_key, sizeof AES_CBC_256_key);
            if (rc == 0)
            {
                rc = KCipherSetDecryptKey (cipher, AES_CBC_256_key, sizeof AES_CBC_256_key);
                if (rc == 0)
                {
                    rc = KCipherSetEncryptIVec (cipher, AES_CBC_256_ivec);
                    if (rc == 0)
                    {
                        rc = KCipherSetDecryptIVec (cipher, AES_CBC_256_ivec);
                        if (rc == 0)
                        {
                            unsigned int index;

                            for (index = 0; index < (sizeof AES_CBC_256_test / sizeof AES_CBC_256_test[0]); ++index)
                            {
                                uint8_t etemp [16];
                                uint8_t dtemp [16];

                                rc = KCipherEncryptCBC (cipher, AES_CBC_256_test[index], etemp, sizeof AES_CBC_256_test[0] / sizeof etemp);
                                if (rc == 0)
                                {
                                    rc = KCipherDecryptCBC (cipher, AES_CBC_256_cipher[index], dtemp, sizeof AES_CBC_256_cipher[0] / sizeof dtemp);
                                    if (rc == 0)
                                    {
                                        if (memcmp (AES_CBC_256_test[index], dtemp, sizeof dtemp) != 0)
                                            KOutMsg ("Failed AES CBC 256 encrypt #%u\n", index);
                                        if (memcmp (AES_CBC_256_cipher[index], etemp, sizeof etemp) != 0)
                                            KOutMsg ("Failed AES CBC 256 decrypt test #%u\n", index);
                                    }
                                }
                            }
                        }
                    }
                }
            }           

            KCipherRelease (cipher);
        }
        KCipherManagerRelease (manager);
    }
    return rc;
}


rc_t CC UsageSummary  (const char * progname)
{
    return KOutMsg ("\n"
                    "Usage:\n"
                    "  %s [OPTIONS]\n"
                    "\n"
                    "Summary:\n"
                    "  Test the KXTocDir type.\n",
                    progname);
}
const char UsageDefaultName[] = "test-modes";
rc_t CC Usage (const Args * args)
{
    return 0;
}
/* { */
/*     const char * progname = UsageDefaultName; */
/*     const char * fullpath = UsageDefaultName; */
/*     rc_t rc = 0; */

/*     rc = ArgsProgram (args, &fullpath, &progname); */
/*     if (rc == 0) */
/*     { */
/*         assert (args); */
/*         summary (UsageDefaultName); */
/*         HelpOptionsStandard (); */
/*     } */
/*     return rc; */
/* } */

/* MINIUSAGE(def_name) */


ver_t CC KAppVersion (void)
{
    return 0;
}
rc_t CC KMain ( int argc, char *argv [] )
{
    Args * args;
    rc_t rc;

    rc = ArgsMakeAndHandle (&args, argc, argv, 0);
    if (rc == 0)
    {

        rc = run();
        if (rc)
            LOGERR (klogErr, rc, "Exiting failure");
        else
            STSMSG (0, ("Exiting okay\n"));
    }

    if (rc)
        LOGERR (klogErr, rc, "Exiting status");
    else
        STSMSG (0, ("Existing status (%R)\n", rc));
    return rc;
}


