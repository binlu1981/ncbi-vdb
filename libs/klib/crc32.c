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

#include <klib/extern.h>
#include <klib/checksum.h>
#include <byteswap.h>

#include <sysalloc.h>

#define SLOW_CRC 0

#if SLOW_CRC == 1
/*--------------------------------------------------------------------------
 * CRC32
 */
static
uint32_t sCRC32_tbl [ 256 ];

/* CRC32Init
 *  initializes table
 *  IDEMPOTENT
 */
LIB_EXPORT void CC CRC32Init ( void )
{
    static int beenHere = 0;
    if ( ! beenHere )
    {
        int i, j;
        int32_t kPoly32 = 0x04C11DB7;
        
        for ( i = 0; i < 256; ++ i )
        {
            int32_t byteCRC = i << 24;
            for ( j = 0; j < 8; ++ j )
            {
                if ( byteCRC < 0 )
                    byteCRC = ( byteCRC << 1 ) ^ kPoly32;
                else
                    byteCRC <<= 1;
            }
            sCRC32_tbl [ i ] = byteCRC;
        }

        beenHere = 1;
    }
}

/* CRC32
 *  runs checksum on arbitrary data, returning result
 *  initial checksum to be passed in is 0
 *  subsequent checksums should be return from prior invocation
 */
LIB_EXPORT uint32_t CC CRC32 ( uint32_t checksum, const void *data, size_t size )
{
    size_t j;

#define str ( ( const unsigned char* ) data )

    if ( sCRC32_tbl [ 0 ] == sCRC32_tbl [ 1 ] )
        CRC32Init();

    for ( j = 0; j < size; ++ j )
    {
        uint32_t i = ( checksum >> 24 ) ^ str [ j ];
        checksum <<= 8;
        checksum ^= sCRC32_tbl [ i ];
    }
    return checksum;
    
#undef str
}

#else /* SLOW_CRC != 1*/
/* -------  experimenting with slicing ------------*/

uint32_t const sCRC32_tbl_[8][256] =
{
	{
		0x00000000,0x04C11DB7,0x09823B6E,0x0D4326D9,0x130476DC,0x17C56B6B,0x1A864DB2,0x1E475005,
		0x2608EDB8,0x22C9F00F,0x2F8AD6D6,0x2B4BCB61,0x350C9B64,0x31CD86D3,0x3C8EA00A,0x384FBDBD,
		0x4C11DB70,0x48D0C6C7,0x4593E01E,0x4152FDA9,0x5F15ADAC,0x5BD4B01B,0x569796C2,0x52568B75,
		0x6A1936C8,0x6ED82B7F,0x639B0DA6,0x675A1011,0x791D4014,0x7DDC5DA3,0x709F7B7A,0x745E66CD,
		0x9823B6E0,0x9CE2AB57,0x91A18D8E,0x95609039,0x8B27C03C,0x8FE6DD8B,0x82A5FB52,0x8664E6E5,
		0xBE2B5B58,0xBAEA46EF,0xB7A96036,0xB3687D81,0xAD2F2D84,0xA9EE3033,0xA4AD16EA,0xA06C0B5D,
		0xD4326D90,0xD0F37027,0xDDB056FE,0xD9714B49,0xC7361B4C,0xC3F706FB,0xCEB42022,0xCA753D95,
		0xF23A8028,0xF6FB9D9F,0xFBB8BB46,0xFF79A6F1,0xE13EF6F4,0xE5FFEB43,0xE8BCCD9A,0xEC7DD02D,
		0x34867077,0x30476DC0,0x3D044B19,0x39C556AE,0x278206AB,0x23431B1C,0x2E003DC5,0x2AC12072,
		0x128E9DCF,0x164F8078,0x1B0CA6A1,0x1FCDBB16,0x018AEB13,0x054BF6A4,0x0808D07D,0x0CC9CDCA,
		0x7897AB07,0x7C56B6B0,0x71159069,0x75D48DDE,0x6B93DDDB,0x6F52C06C,0x6211E6B5,0x66D0FB02,
		0x5E9F46BF,0x5A5E5B08,0x571D7DD1,0x53DC6066,0x4D9B3063,0x495A2DD4,0x44190B0D,0x40D816BA,
		0xACA5C697,0xA864DB20,0xA527FDF9,0xA1E6E04E,0xBFA1B04B,0xBB60ADFC,0xB6238B25,0xB2E29692,
		0x8AAD2B2F,0x8E6C3698,0x832F1041,0x87EE0DF6,0x99A95DF3,0x9D684044,0x902B669D,0x94EA7B2A,
		0xE0B41DE7,0xE4750050,0xE9362689,0xEDF73B3E,0xF3B06B3B,0xF771768C,0xFA325055,0xFEF34DE2,
		0xC6BCF05F,0xC27DEDE8,0xCF3ECB31,0xCBFFD686,0xD5B88683,0xD1799B34,0xDC3ABDED,0xD8FBA05A,
		0x690CE0EE,0x6DCDFD59,0x608EDB80,0x644FC637,0x7A089632,0x7EC98B85,0x738AAD5C,0x774BB0EB,
		0x4F040D56,0x4BC510E1,0x46863638,0x42472B8F,0x5C007B8A,0x58C1663D,0x558240E4,0x51435D53,
		0x251D3B9E,0x21DC2629,0x2C9F00F0,0x285E1D47,0x36194D42,0x32D850F5,0x3F9B762C,0x3B5A6B9B,
		0x0315D626,0x07D4CB91,0x0A97ED48,0x0E56F0FF,0x1011A0FA,0x14D0BD4D,0x19939B94,0x1D528623,
		0xF12F560E,0xF5EE4BB9,0xF8AD6D60,0xFC6C70D7,0xE22B20D2,0xE6EA3D65,0xEBA91BBC,0xEF68060B,
		0xD727BBB6,0xD3E6A601,0xDEA580D8,0xDA649D6F,0xC423CD6A,0xC0E2D0DD,0xCDA1F604,0xC960EBB3,
		0xBD3E8D7E,0xB9FF90C9,0xB4BCB610,0xB07DABA7,0xAE3AFBA2,0xAAFBE615,0xA7B8C0CC,0xA379DD7B,
		0x9B3660C6,0x9FF77D71,0x92B45BA8,0x9675461F,0x8832161A,0x8CF30BAD,0x81B02D74,0x857130C3,
		0x5D8A9099,0x594B8D2E,0x5408ABF7,0x50C9B640,0x4E8EE645,0x4A4FFBF2,0x470CDD2B,0x43CDC09C,
		0x7B827D21,0x7F436096,0x7200464F,0x76C15BF8,0x68860BFD,0x6C47164A,0x61043093,0x65C52D24,
		0x119B4BE9,0x155A565E,0x18197087,0x1CD86D30,0x029F3D35,0x065E2082,0x0B1D065B,0x0FDC1BEC,
		0x3793A651,0x3352BBE6,0x3E119D3F,0x3AD08088,0x2497D08D,0x2056CD3A,0x2D15EBE3,0x29D4F654,
		0xC5A92679,0xC1683BCE,0xCC2B1D17,0xC8EA00A0,0xD6AD50A5,0xD26C4D12,0xDF2F6BCB,0xDBEE767C,
		0xE3A1CBC1,0xE760D676,0xEA23F0AF,0xEEE2ED18,0xF0A5BD1D,0xF464A0AA,0xF9278673,0xFDE69BC4,
		0x89B8FD09,0x8D79E0BE,0x803AC667,0x84FBDBD0,0x9ABC8BD5,0x9E7D9662,0x933EB0BB,0x97FFAD0C,
		0xAFB010B1,0xAB710D06,0xA6322BDF,0xA2F33668,0xBCB4666D,0xB8757BDA,0xB5365D03,0xB1F740B4
	},
	{
		0x00000000,0xD219C1DC,0xA0F29E0F,0x72EB5FD3,0x452421A9,0x973DE075,0xE5D6BFA6,0x37CF7E7A,
		0x8A484352,0x5851828E,0x2ABADD5D,0xF8A31C81,0xCF6C62FB,0x1D75A327,0x6F9EFCF4,0xBD873D28,
		0x10519B13,0xC2485ACF,0xB0A3051C,0x62BAC4C0,0x5575BABA,0x876C7B66,0xF58724B5,0x279EE569,
		0x9A19D841,0x4800199D,0x3AEB464E,0xE8F28792,0xDF3DF9E8,0x0D243834,0x7FCF67E7,0xADD6A63B,
		0x20A33626,0xF2BAF7FA,0x8051A829,0x524869F5,0x6587178F,0xB79ED653,0xC5758980,0x176C485C,
		0xAAEB7574,0x78F2B4A8,0x0A19EB7B,0xD8002AA7,0xEFCF54DD,0x3DD69501,0x4F3DCAD2,0x9D240B0E,
		0x30F2AD35,0xE2EB6CE9,0x9000333A,0x4219F2E6,0x75D68C9C,0xA7CF4D40,0xD5241293,0x073DD34F,
		0xBABAEE67,0x68A32FBB,0x1A487068,0xC851B1B4,0xFF9ECFCE,0x2D870E12,0x5F6C51C1,0x8D75901D,
		0x41466C4C,0x935FAD90,0xE1B4F243,0x33AD339F,0x04624DE5,0xD67B8C39,0xA490D3EA,0x76891236,
		0xCB0E2F1E,0x1917EEC2,0x6BFCB111,0xB9E570CD,0x8E2A0EB7,0x5C33CF6B,0x2ED890B8,0xFCC15164,
		0x5117F75F,0x830E3683,0xF1E56950,0x23FCA88C,0x1433D6F6,0xC62A172A,0xB4C148F9,0x66D88925,
		0xDB5FB40D,0x094675D1,0x7BAD2A02,0xA9B4EBDE,0x9E7B95A4,0x4C625478,0x3E890BAB,0xEC90CA77,
		0x61E55A6A,0xB3FC9BB6,0xC117C465,0x130E05B9,0x24C17BC3,0xF6D8BA1F,0x8433E5CC,0x562A2410,
		0xEBAD1938,0x39B4D8E4,0x4B5F8737,0x994646EB,0xAE893891,0x7C90F94D,0x0E7BA69E,0xDC626742,
		0x71B4C179,0xA3AD00A5,0xD1465F76,0x035F9EAA,0x3490E0D0,0xE689210C,0x94627EDF,0x467BBF03,
		0xFBFC822B,0x29E543F7,0x5B0E1C24,0x8917DDF8,0xBED8A382,0x6CC1625E,0x1E2A3D8D,0xCC33FC51,
		0x828CD898,0x50951944,0x227E4697,0xF067874B,0xC7A8F931,0x15B138ED,0x675A673E,0xB543A6E2,
		0x08C49BCA,0xDADD5A16,0xA83605C5,0x7A2FC419,0x4DE0BA63,0x9FF97BBF,0xED12246C,0x3F0BE5B0,
		0x92DD438B,0x40C48257,0x322FDD84,0xE0361C58,0xD7F96222,0x05E0A3FE,0x770BFC2D,0xA5123DF1,
		0x189500D9,0xCA8CC105,0xB8679ED6,0x6A7E5F0A,0x5DB12170,0x8FA8E0AC,0xFD43BF7F,0x2F5A7EA3,
		0xA22FEEBE,0x70362F62,0x02DD70B1,0xD0C4B16D,0xE70BCF17,0x35120ECB,0x47F95118,0x95E090C4,
		0x2867ADEC,0xFA7E6C30,0x889533E3,0x5A8CF23F,0x6D438C45,0xBF5A4D99,0xCDB1124A,0x1FA8D396,
		0xB27E75AD,0x6067B471,0x128CEBA2,0xC0952A7E,0xF75A5404,0x254395D8,0x57A8CA0B,0x85B10BD7,
		0x383636FF,0xEA2FF723,0x98C4A8F0,0x4ADD692C,0x7D121756,0xAF0BD68A,0xDDE08959,0x0FF94885,
		0xC3CAB4D4,0x11D37508,0x63382ADB,0xB121EB07,0x86EE957D,0x54F754A1,0x261C0B72,0xF405CAAE,
		0x4982F786,0x9B9B365A,0xE9706989,0x3B69A855,0x0CA6D62F,0xDEBF17F3,0xAC544820,0x7E4D89FC,
		0xD39B2FC7,0x0182EE1B,0x7369B1C8,0xA1707014,0x96BF0E6E,0x44A6CFB2,0x364D9061,0xE45451BD,
		0x59D36C95,0x8BCAAD49,0xF921F29A,0x2B383346,0x1CF74D3C,0xCEEE8CE0,0xBC05D333,0x6E1C12EF,
		0xE36982F2,0x3170432E,0x439B1CFD,0x9182DD21,0xA64DA35B,0x74546287,0x06BF3D54,0xD4A6FC88,
		0x6921C1A0,0xBB38007C,0xC9D35FAF,0x1BCA9E73,0x2C05E009,0xFE1C21D5,0x8CF77E06,0x5EEEBFDA,
		0xF33819E1,0x2121D83D,0x53CA87EE,0x81D34632,0xB61C3848,0x6405F994,0x16EEA647,0xC4F7679B,
		0x79705AB3,0xAB699B6F,0xD982C4BC,0x0B9B0560,0x3C547B1A,0xEE4DBAC6,0x9CA6E515,0x4EBF24C9
	},
	{
		0x00000000,0x01D8AC87,0x03B1590E,0x0269F589,0x0762B21C,0x06BA1E9B,0x04D3EB12,0x050B4795,
		0x0EC56438,0x0F1DC8BF,0x0D743D36,0x0CAC91B1,0x09A7D624,0x087F7AA3,0x0A168F2A,0x0BCE23AD,
		0x1D8AC870,0x1C5264F7,0x1E3B917E,0x1FE33DF9,0x1AE87A6C,0x1B30D6EB,0x19592362,0x18818FE5,
		0x134FAC48,0x129700CF,0x10FEF546,0x112659C1,0x142D1E54,0x15F5B2D3,0x179C475A,0x1644EBDD,
		0x3B1590E0,0x3ACD3C67,0x38A4C9EE,0x397C6569,0x3C7722FC,0x3DAF8E7B,0x3FC67BF2,0x3E1ED775,
		0x35D0F4D8,0x3408585F,0x3661ADD6,0x37B90151,0x32B246C4,0x336AEA43,0x31031FCA,0x30DBB34D,
		0x269F5890,0x2747F417,0x252E019E,0x24F6AD19,0x21FDEA8C,0x2025460B,0x224CB382,0x23941F05,
		0x285A3CA8,0x2982902F,0x2BEB65A6,0x2A33C921,0x2F388EB4,0x2EE02233,0x2C89D7BA,0x2D517B3D,
		0x762B21C0,0x77F38D47,0x759A78CE,0x7442D449,0x714993DC,0x70913F5B,0x72F8CAD2,0x73206655,
		0x78EE45F8,0x7936E97F,0x7B5F1CF6,0x7A87B071,0x7F8CF7E4,0x7E545B63,0x7C3DAEEA,0x7DE5026D,
		0x6BA1E9B0,0x6A794537,0x6810B0BE,0x69C81C39,0x6CC35BAC,0x6D1BF72B,0x6F7202A2,0x6EAAAE25,
		0x65648D88,0x64BC210F,0x66D5D486,0x670D7801,0x62063F94,0x63DE9313,0x61B7669A,0x606FCA1D,
		0x4D3EB120,0x4CE61DA7,0x4E8FE82E,0x4F5744A9,0x4A5C033C,0x4B84AFBB,0x49ED5A32,0x4835F6B5,
		0x43FBD518,0x4223799F,0x404A8C16,0x41922091,0x44996704,0x4541CB83,0x47283E0A,0x46F0928D,
		0x50B47950,0x516CD5D7,0x5305205E,0x52DD8CD9,0x57D6CB4C,0x560E67CB,0x54679242,0x55BF3EC5,
		0x5E711D68,0x5FA9B1EF,0x5DC04466,0x5C18E8E1,0x5913AF74,0x58CB03F3,0x5AA2F67A,0x5B7A5AFD,
		0xEC564380,0xED8EEF07,0xEFE71A8E,0xEE3FB609,0xEB34F19C,0xEAEC5D1B,0xE885A892,0xE95D0415,
		0xE29327B8,0xE34B8B3F,0xE1227EB6,0xE0FAD231,0xE5F195A4,0xE4293923,0xE640CCAA,0xE798602D,
		0xF1DC8BF0,0xF0042777,0xF26DD2FE,0xF3B57E79,0xF6BE39EC,0xF766956B,0xF50F60E2,0xF4D7CC65,
		0xFF19EFC8,0xFEC1434F,0xFCA8B6C6,0xFD701A41,0xF87B5DD4,0xF9A3F153,0xFBCA04DA,0xFA12A85D,
		0xD743D360,0xD69B7FE7,0xD4F28A6E,0xD52A26E9,0xD021617C,0xD1F9CDFB,0xD3903872,0xD24894F5,
		0xD986B758,0xD85E1BDF,0xDA37EE56,0xDBEF42D1,0xDEE40544,0xDF3CA9C3,0xDD555C4A,0xDC8DF0CD,
		0xCAC91B10,0xCB11B797,0xC978421E,0xC8A0EE99,0xCDABA90C,0xCC73058B,0xCE1AF002,0xCFC25C85,
		0xC40C7F28,0xC5D4D3AF,0xC7BD2626,0xC6658AA1,0xC36ECD34,0xC2B661B3,0xC0DF943A,0xC10738BD,
		0x9A7D6240,0x9BA5CEC7,0x99CC3B4E,0x981497C9,0x9D1FD05C,0x9CC77CDB,0x9EAE8952,0x9F7625D5,
		0x94B80678,0x9560AAFF,0x97095F76,0x96D1F3F1,0x93DAB464,0x920218E3,0x906BED6A,0x91B341ED,
		0x87F7AA30,0x862F06B7,0x8446F33E,0x859E5FB9,0x8095182C,0x814DB4AB,0x83244122,0x82FCEDA5,
		0x8932CE08,0x88EA628F,0x8A839706,0x8B5B3B81,0x8E507C14,0x8F88D093,0x8DE1251A,0x8C39899D,
		0xA168F2A0,0xA0B05E27,0xA2D9ABAE,0xA3010729,0xA60A40BC,0xA7D2EC3B,0xA5BB19B2,0xA463B535,
		0xAFAD9698,0xAE753A1F,0xAC1CCF96,0xADC46311,0xA8CF2484,0xA9178803,0xAB7E7D8A,0xAAA6D10D,
		0xBCE23AD0,0xBD3A9657,0xBF5363DE,0xBE8BCF59,0xBB8088CC,0xBA58244B,0xB831D1C2,0xB9E97D45,
		0xB2275EE8,0xB3FFF26F,0xB19607E6,0xB04EAB61,0xB545ECF4,0xB49D4073,0xB6F4B5FA,0xB72C197D
	},
	{
		0x00000000,0xDC6D9AB7,0xBC1A28D9,0x6077B26E,0x7CF54C05,0xA098D6B2,0xC0EF64DC,0x1C82FE6B,
		0xF9EA980A,0x258702BD,0x45F0B0D3,0x999D2A64,0x851FD40F,0x59724EB8,0x3905FCD6,0xE5686661,
		0xF7142DA3,0x2B79B714,0x4B0E057A,0x97639FCD,0x8BE161A6,0x578CFB11,0x37FB497F,0xEB96D3C8,
		0x0EFEB5A9,0xD2932F1E,0xB2E49D70,0x6E8907C7,0x720BF9AC,0xAE66631B,0xCE11D175,0x127C4BC2,
		0xEAE946F1,0x3684DC46,0x56F36E28,0x8A9EF49F,0x961C0AF4,0x4A719043,0x2A06222D,0xF66BB89A,
		0x1303DEFB,0xCF6E444C,0xAF19F622,0x73746C95,0x6FF692FE,0xB39B0849,0xD3ECBA27,0x0F812090,
		0x1DFD6B52,0xC190F1E5,0xA1E7438B,0x7D8AD93C,0x61082757,0xBD65BDE0,0xDD120F8E,0x017F9539,
		0xE417F358,0x387A69EF,0x580DDB81,0x84604136,0x98E2BF5D,0x448F25EA,0x24F89784,0xF8950D33,
		0xD1139055,0x0D7E0AE2,0x6D09B88C,0xB164223B,0xADE6DC50,0x718B46E7,0x11FCF489,0xCD916E3E,
		0x28F9085F,0xF49492E8,0x94E32086,0x488EBA31,0x540C445A,0x8861DEED,0xE8166C83,0x347BF634,
		0x2607BDF6,0xFA6A2741,0x9A1D952F,0x46700F98,0x5AF2F1F3,0x869F6B44,0xE6E8D92A,0x3A85439D,
		0xDFED25FC,0x0380BF4B,0x63F70D25,0xBF9A9792,0xA31869F9,0x7F75F34E,0x1F024120,0xC36FDB97,
		0x3BFAD6A4,0xE7974C13,0x87E0FE7D,0x5B8D64CA,0x470F9AA1,0x9B620016,0xFB15B278,0x277828CF,
		0xC2104EAE,0x1E7DD419,0x7E0A6677,0xA267FCC0,0xBEE502AB,0x6288981C,0x02FF2A72,0xDE92B0C5,
		0xCCEEFB07,0x108361B0,0x70F4D3DE,0xAC994969,0xB01BB702,0x6C762DB5,0x0C019FDB,0xD06C056C,
		0x3504630D,0xE969F9BA,0x891E4BD4,0x5573D163,0x49F12F08,0x959CB5BF,0xF5EB07D1,0x29869D66,
		0xA6E63D1D,0x7A8BA7AA,0x1AFC15C4,0xC6918F73,0xDA137118,0x067EEBAF,0x660959C1,0xBA64C376,
		0x5F0CA517,0x83613FA0,0xE3168DCE,0x3F7B1779,0x23F9E912,0xFF9473A5,0x9FE3C1CB,0x438E5B7C,
		0x51F210BE,0x8D9F8A09,0xEDE83867,0x3185A2D0,0x2D075CBB,0xF16AC60C,0x911D7462,0x4D70EED5,
		0xA81888B4,0x74751203,0x1402A06D,0xC86F3ADA,0xD4EDC4B1,0x08805E06,0x68F7EC68,0xB49A76DF,
		0x4C0F7BEC,0x9062E15B,0xF0155335,0x2C78C982,0x30FA37E9,0xEC97AD5E,0x8CE01F30,0x508D8587,
		0xB5E5E3E6,0x69887951,0x09FFCB3F,0xD5925188,0xC910AFE3,0x157D3554,0x750A873A,0xA9671D8D,
		0xBB1B564F,0x6776CCF8,0x07017E96,0xDB6CE421,0xC7EE1A4A,0x1B8380FD,0x7BF43293,0xA799A824,
		0x42F1CE45,0x9E9C54F2,0xFEEBE69C,0x22867C2B,0x3E048240,0xE26918F7,0x821EAA99,0x5E73302E,
		0x77F5AD48,0xAB9837FF,0xCBEF8591,0x17821F26,0x0B00E14D,0xD76D7BFA,0xB71AC994,0x6B775323,
		0x8E1F3542,0x5272AFF5,0x32051D9B,0xEE68872C,0xF2EA7947,0x2E87E3F0,0x4EF0519E,0x929DCB29,
		0x80E180EB,0x5C8C1A5C,0x3CFBA832,0xE0963285,0xFC14CCEE,0x20795659,0x400EE437,0x9C637E80,
		0x790B18E1,0xA5668256,0xC5113038,0x197CAA8F,0x05FE54E4,0xD993CE53,0xB9E47C3D,0x6589E68A,
		0x9D1CEBB9,0x4171710E,0x2106C360,0xFD6B59D7,0xE1E9A7BC,0x3D843D0B,0x5DF38F65,0x819E15D2,
		0x64F673B3,0xB89BE904,0xD8EC5B6A,0x0481C1DD,0x18033FB6,0xC46EA501,0xA419176F,0x78748DD8,
		0x6A08C61A,0xB6655CAD,0xD612EEC3,0x0A7F7474,0x16FD8A1F,0xCA9010A8,0xAAE7A2C6,0x768A3871,
		0x93E25E10,0x4F8FC4A7,0x2FF876C9,0xF395EC7E,0xEF171215,0x337A88A2,0x530D3ACC,0x8F60A07B
	},
	{
		0x00000000,0x490D678D,0x921ACF1A,0xDB17A897,0x20F48383,0x69F9E40E,0xB2EE4C99,0xFBE32B14,
		0x41E90706,0x08E4608B,0xD3F3C81C,0x9AFEAF91,0x611D8485,0x2810E308,0xF3074B9F,0xBA0A2C12,
		0x83D20E0C,0xCADF6981,0x11C8C116,0x58C5A69B,0xA3268D8F,0xEA2BEA02,0x313C4295,0x78312518,
		0xC23B090A,0x8B366E87,0x5021C610,0x192CA19D,0xE2CF8A89,0xABC2ED04,0x70D54593,0x39D8221E,
		0x036501AF,0x4A686622,0x917FCEB5,0xD872A938,0x2391822C,0x6A9CE5A1,0xB18B4D36,0xF8862ABB,
		0x428C06A9,0x0B816124,0xD096C9B3,0x999BAE3E,0x6278852A,0x2B75E2A7,0xF0624A30,0xB96F2DBD,
		0x80B70FA3,0xC9BA682E,0x12ADC0B9,0x5BA0A734,0xA0438C20,0xE94EEBAD,0x3259433A,0x7B5424B7,
		0xC15E08A5,0x88536F28,0x5344C7BF,0x1A49A032,0xE1AA8B26,0xA8A7ECAB,0x73B0443C,0x3ABD23B1,
		0x06CA035E,0x4FC764D3,0x94D0CC44,0xDDDDABC9,0x263E80DD,0x6F33E750,0xB4244FC7,0xFD29284A,
		0x47230458,0x0E2E63D5,0xD539CB42,0x9C34ACCF,0x67D787DB,0x2EDAE056,0xF5CD48C1,0xBCC02F4C,
		0x85180D52,0xCC156ADF,0x1702C248,0x5E0FA5C5,0xA5EC8ED1,0xECE1E95C,0x37F641CB,0x7EFB2646,
		0xC4F10A54,0x8DFC6DD9,0x56EBC54E,0x1FE6A2C3,0xE40589D7,0xAD08EE5A,0x761F46CD,0x3F122140,
		0x05AF02F1,0x4CA2657C,0x97B5CDEB,0xDEB8AA66,0x255B8172,0x6C56E6FF,0xB7414E68,0xFE4C29E5,
		0x444605F7,0x0D4B627A,0xD65CCAED,0x9F51AD60,0x64B28674,0x2DBFE1F9,0xF6A8496E,0xBFA52EE3,
		0x867D0CFD,0xCF706B70,0x1467C3E7,0x5D6AA46A,0xA6898F7E,0xEF84E8F3,0x34934064,0x7D9E27E9,
		0xC7940BFB,0x8E996C76,0x558EC4E1,0x1C83A36C,0xE7608878,0xAE6DEFF5,0x757A4762,0x3C7720EF,
		0x0D9406BC,0x44996131,0x9F8EC9A6,0xD683AE2B,0x2D60853F,0x646DE2B2,0xBF7A4A25,0xF6772DA8,
		0x4C7D01BA,0x05706637,0xDE67CEA0,0x976AA92D,0x6C898239,0x2584E5B4,0xFE934D23,0xB79E2AAE,
		0x8E4608B0,0xC74B6F3D,0x1C5CC7AA,0x5551A027,0xAEB28B33,0xE7BFECBE,0x3CA84429,0x75A523A4,
		0xCFAF0FB6,0x86A2683B,0x5DB5C0AC,0x14B8A721,0xEF5B8C35,0xA656EBB8,0x7D41432F,0x344C24A2,
		0x0EF10713,0x47FC609E,0x9CEBC809,0xD5E6AF84,0x2E058490,0x6708E31D,0xBC1F4B8A,0xF5122C07,
		0x4F180015,0x06156798,0xDD02CF0F,0x940FA882,0x6FEC8396,0x26E1E41B,0xFDF64C8C,0xB4FB2B01,
		0x8D23091F,0xC42E6E92,0x1F39C605,0x5634A188,0xADD78A9C,0xE4DAED11,0x3FCD4586,0x76C0220B,
		0xCCCA0E19,0x85C76994,0x5ED0C103,0x17DDA68E,0xEC3E8D9A,0xA533EA17,0x7E244280,0x3729250D,
		0x0B5E05E2,0x4253626F,0x9944CAF8,0xD049AD75,0x2BAA8661,0x62A7E1EC,0xB9B0497B,0xF0BD2EF6,
		0x4AB702E4,0x03BA6569,0xD8ADCDFE,0x91A0AA73,0x6A438167,0x234EE6EA,0xF8594E7D,0xB15429F0,
		0x888C0BEE,0xC1816C63,0x1A96C4F4,0x539BA379,0xA878886D,0xE175EFE0,0x3A624777,0x736F20FA,
		0xC9650CE8,0x80686B65,0x5B7FC3F2,0x1272A47F,0xE9918F6B,0xA09CE8E6,0x7B8B4071,0x328627FC,
		0x083B044D,0x413663C0,0x9A21CB57,0xD32CACDA,0x28CF87CE,0x61C2E043,0xBAD548D4,0xF3D82F59,
		0x49D2034B,0x00DF64C6,0xDBC8CC51,0x92C5ABDC,0x692680C8,0x202BE745,0xFB3C4FD2,0xB231285F,
		0x8BE90A41,0xC2E46DCC,0x19F3C55B,0x50FEA2D6,0xAB1D89C2,0xE210EE4F,0x390746D8,0x700A2155,
		0xCA000D47,0x830D6ACA,0x581AC25D,0x1117A5D0,0xEAF48EC4,0xA3F9E949,0x78EE41DE,0x31E32653
	},
	{
		0x00000000,0x1B280D78,0x36501AF0,0x2D781788,0x6CA035E0,0x77883898,0x5AF02F10,0x41D82268,
		0xD9406BC0,0xC26866B8,0xEF107130,0xF4387C48,0xB5E05E20,0xAEC85358,0x83B044D0,0x989849A8,
		0xB641CA37,0xAD69C74F,0x8011D0C7,0x9B39DDBF,0xDAE1FFD7,0xC1C9F2AF,0xECB1E527,0xF799E85F,
		0x6F01A1F7,0x7429AC8F,0x5951BB07,0x4279B67F,0x03A19417,0x1889996F,0x35F18EE7,0x2ED9839F,
		0x684289D9,0x736A84A1,0x5E129329,0x453A9E51,0x04E2BC39,0x1FCAB141,0x32B2A6C9,0x299AABB1,
		0xB102E219,0xAA2AEF61,0x8752F8E9,0x9C7AF591,0xDDA2D7F9,0xC68ADA81,0xEBF2CD09,0xF0DAC071,
		0xDE0343EE,0xC52B4E96,0xE853591E,0xF37B5466,0xB2A3760E,0xA98B7B76,0x84F36CFE,0x9FDB6186,
		0x0743282E,0x1C6B2556,0x311332DE,0x2A3B3FA6,0x6BE31DCE,0x70CB10B6,0x5DB3073E,0x469B0A46,
		0xD08513B2,0xCBAD1ECA,0xE6D50942,0xFDFD043A,0xBC252652,0xA70D2B2A,0x8A753CA2,0x915D31DA,
		0x09C57872,0x12ED750A,0x3F956282,0x24BD6FFA,0x65654D92,0x7E4D40EA,0x53355762,0x481D5A1A,
		0x66C4D985,0x7DECD4FD,0x5094C375,0x4BBCCE0D,0x0A64EC65,0x114CE11D,0x3C34F695,0x271CFBED,
		0xBF84B245,0xA4ACBF3D,0x89D4A8B5,0x92FCA5CD,0xD32487A5,0xC80C8ADD,0xE5749D55,0xFE5C902D,
		0xB8C79A6B,0xA3EF9713,0x8E97809B,0x95BF8DE3,0xD467AF8B,0xCF4FA2F3,0xE237B57B,0xF91FB803,
		0x6187F1AB,0x7AAFFCD3,0x57D7EB5B,0x4CFFE623,0x0D27C44B,0x160FC933,0x3B77DEBB,0x205FD3C3,
		0x0E86505C,0x15AE5D24,0x38D64AAC,0x23FE47D4,0x622665BC,0x790E68C4,0x54767F4C,0x4F5E7234,
		0xD7C63B9C,0xCCEE36E4,0xE196216C,0xFABE2C14,0xBB660E7C,0xA04E0304,0x8D36148C,0x961E19F4,
		0xA5CB3AD3,0xBEE337AB,0x939B2023,0x88B32D5B,0xC96B0F33,0xD243024B,0xFF3B15C3,0xE41318BB,
		0x7C8B5113,0x67A35C6B,0x4ADB4BE3,0x51F3469B,0x102B64F3,0x0B03698B,0x267B7E03,0x3D53737B,
		0x138AF0E4,0x08A2FD9C,0x25DAEA14,0x3EF2E76C,0x7F2AC504,0x6402C87C,0x497ADFF4,0x5252D28C,
		0xCACA9B24,0xD1E2965C,0xFC9A81D4,0xE7B28CAC,0xA66AAEC4,0xBD42A3BC,0x903AB434,0x8B12B94C,
		0xCD89B30A,0xD6A1BE72,0xFBD9A9FA,0xE0F1A482,0xA12986EA,0xBA018B92,0x97799C1A,0x8C519162,
		0x14C9D8CA,0x0FE1D5B2,0x2299C23A,0x39B1CF42,0x7869ED2A,0x6341E052,0x4E39F7DA,0x5511FAA2,
		0x7BC8793D,0x60E07445,0x4D9863CD,0x56B06EB5,0x17684CDD,0x0C4041A5,0x2138562D,0x3A105B55,
		0xA28812FD,0xB9A01F85,0x94D8080D,0x8FF00575,0xCE28271D,0xD5002A65,0xF8783DED,0xE3503095,
		0x754E2961,0x6E662419,0x431E3391,0x58363EE9,0x19EE1C81,0x02C611F9,0x2FBE0671,0x34960B09,
		0xAC0E42A1,0xB7264FD9,0x9A5E5851,0x81765529,0xC0AE7741,0xDB867A39,0xF6FE6DB1,0xEDD660C9,
		0xC30FE356,0xD827EE2E,0xF55FF9A6,0xEE77F4DE,0xAFAFD6B6,0xB487DBCE,0x99FFCC46,0x82D7C13E,
		0x1A4F8896,0x016785EE,0x2C1F9266,0x37379F1E,0x76EFBD76,0x6DC7B00E,0x40BFA786,0x5B97AAFE,
		0x1D0CA0B8,0x0624ADC0,0x2B5CBA48,0x3074B730,0x71AC9558,0x6A849820,0x47FC8FA8,0x5CD482D0,
		0xC44CCB78,0xDF64C600,0xF21CD188,0xE934DCF0,0xA8ECFE98,0xB3C4F3E0,0x9EBCE468,0x8594E910,
		0xAB4D6A8F,0xB06567F7,0x9D1D707F,0x86357D07,0xC7ED5F6F,0xDCC55217,0xF1BD459F,0xEA9548E7,
		0x720D014F,0x69250C37,0x445D1BBF,0x5F7516C7,0x1EAD34AF,0x058539D7,0x28FD2E5F,0x33D52327
	},
	{
		0x00000000,0x4F576811,0x9EAED022,0xD1F9B833,0x399CBDF3,0x76CBD5E2,0xA7326DD1,0xE86505C0,
		0x73397BE6,0x3C6E13F7,0xED97ABC4,0xA2C0C3D5,0x4AA5C615,0x05F2AE04,0xD40B1637,0x9B5C7E26,
		0xE672F7CC,0xA9259FDD,0x78DC27EE,0x378B4FFF,0xDFEE4A3F,0x90B9222E,0x41409A1D,0x0E17F20C,
		0x954B8C2A,0xDA1CE43B,0x0BE55C08,0x44B23419,0xACD731D9,0xE38059C8,0x3279E1FB,0x7D2E89EA,
		0xC824F22F,0x87739A3E,0x568A220D,0x19DD4A1C,0xF1B84FDC,0xBEEF27CD,0x6F169FFE,0x2041F7EF,
		0xBB1D89C9,0xF44AE1D8,0x25B359EB,0x6AE431FA,0x8281343A,0xCDD65C2B,0x1C2FE418,0x53788C09,
		0x2E5605E3,0x61016DF2,0xB0F8D5C1,0xFFAFBDD0,0x17CAB810,0x589DD001,0x89646832,0xC6330023,
		0x5D6F7E05,0x12381614,0xC3C1AE27,0x8C96C636,0x64F3C3F6,0x2BA4ABE7,0xFA5D13D4,0xB50A7BC5,
		0x9488F9E9,0xDBDF91F8,0x0A2629CB,0x457141DA,0xAD14441A,0xE2432C0B,0x33BA9438,0x7CEDFC29,
		0xE7B1820F,0xA8E6EA1E,0x791F522D,0x36483A3C,0xDE2D3FFC,0x917A57ED,0x4083EFDE,0x0FD487CF,
		0x72FA0E25,0x3DAD6634,0xEC54DE07,0xA303B616,0x4B66B3D6,0x0431DBC7,0xD5C863F4,0x9A9F0BE5,
		0x01C375C3,0x4E941DD2,0x9F6DA5E1,0xD03ACDF0,0x385FC830,0x7708A021,0xA6F11812,0xE9A67003,
		0x5CAC0BC6,0x13FB63D7,0xC202DBE4,0x8D55B3F5,0x6530B635,0x2A67DE24,0xFB9E6617,0xB4C90E06,
		0x2F957020,0x60C21831,0xB13BA002,0xFE6CC813,0x1609CDD3,0x595EA5C2,0x88A71DF1,0xC7F075E0,
		0xBADEFC0A,0xF589941B,0x24702C28,0x6B274439,0x834241F9,0xCC1529E8,0x1DEC91DB,0x52BBF9CA,
		0xC9E787EC,0x86B0EFFD,0x574957CE,0x181E3FDF,0xF07B3A1F,0xBF2C520E,0x6ED5EA3D,0x2182822C,
		0x2DD0EE65,0x62878674,0xB37E3E47,0xFC295656,0x144C5396,0x5B1B3B87,0x8AE283B4,0xC5B5EBA5,
		0x5EE99583,0x11BEFD92,0xC04745A1,0x8F102DB0,0x67752870,0x28224061,0xF9DBF852,0xB68C9043,
		0xCBA219A9,0x84F571B8,0x550CC98B,0x1A5BA19A,0xF23EA45A,0xBD69CC4B,0x6C907478,0x23C71C69,
		0xB89B624F,0xF7CC0A5E,0x2635B26D,0x6962DA7C,0x8107DFBC,0xCE50B7AD,0x1FA90F9E,0x50FE678F,
		0xE5F41C4A,0xAAA3745B,0x7B5ACC68,0x340DA479,0xDC68A1B9,0x933FC9A8,0x42C6719B,0x0D91198A,
		0x96CD67AC,0xD99A0FBD,0x0863B78E,0x4734DF9F,0xAF51DA5F,0xE006B24E,0x31FF0A7D,0x7EA8626C,
		0x0386EB86,0x4CD18397,0x9D283BA4,0xD27F53B5,0x3A1A5675,0x754D3E64,0xA4B48657,0xEBE3EE46,
		0x70BF9060,0x3FE8F871,0xEE114042,0xA1462853,0x49232D93,0x06744582,0xD78DFDB1,0x98DA95A0,
		0xB958178C,0xF60F7F9D,0x27F6C7AE,0x68A1AFBF,0x80C4AA7F,0xCF93C26E,0x1E6A7A5D,0x513D124C,
		0xCA616C6A,0x8536047B,0x54CFBC48,0x1B98D459,0xF3FDD199,0xBCAAB988,0x6D5301BB,0x220469AA,
		0x5F2AE040,0x107D8851,0xC1843062,0x8ED35873,0x66B65DB3,0x29E135A2,0xF8188D91,0xB74FE580,
		0x2C139BA6,0x6344F3B7,0xB2BD4B84,0xFDEA2395,0x158F2655,0x5AD84E44,0x8B21F677,0xC4769E66,
		0x717CE5A3,0x3E2B8DB2,0xEFD23581,0xA0855D90,0x48E05850,0x07B73041,0xD64E8872,0x9919E063,
		0x02459E45,0x4D12F654,0x9CEB4E67,0xD3BC2676,0x3BD923B6,0x748E4BA7,0xA577F394,0xEA209B85,
		0x970E126F,0xD8597A7E,0x09A0C24D,0x46F7AA5C,0xAE92AF9C,0xE1C5C78D,0x303C7FBE,0x7F6B17AF,
		0xE4376989,0xAB600198,0x7A99B9AB,0x35CED1BA,0xDDABD47A,0x92FCBC6B,0x43050458,0x0C526C49
	},
	{
		0x00000000,0x5BA1DCCA,0xB743B994,0xECE2655E,0x6A466E9F,0x31E7B255,0xDD05D70B,0x86A40BC1,
		0xD48CDD3E,0x8F2D01F4,0x63CF64AA,0x386EB860,0xBECAB3A1,0xE56B6F6B,0x09890A35,0x5228D6FF,
		0xADD8A7CB,0xF6797B01,0x1A9B1E5F,0x413AC295,0xC79EC954,0x9C3F159E,0x70DD70C0,0x2B7CAC0A,
		0x79547AF5,0x22F5A63F,0xCE17C361,0x95B61FAB,0x1312146A,0x48B3C8A0,0xA451ADFE,0xFFF07134,
		0x5F705221,0x04D18EEB,0xE833EBB5,0xB392377F,0x35363CBE,0x6E97E074,0x8275852A,0xD9D459E0,
		0x8BFC8F1F,0xD05D53D5,0x3CBF368B,0x671EEA41,0xE1BAE180,0xBA1B3D4A,0x56F95814,0x0D5884DE,
		0xF2A8F5EA,0xA9092920,0x45EB4C7E,0x1E4A90B4,0x98EE9B75,0xC34F47BF,0x2FAD22E1,0x740CFE2B,
		0x262428D4,0x7D85F41E,0x91679140,0xCAC64D8A,0x4C62464B,0x17C39A81,0xFB21FFDF,0xA0802315,
		0xBEE0A442,0xE5417888,0x09A31DD6,0x5202C11C,0xD4A6CADD,0x8F071617,0x63E57349,0x3844AF83,
		0x6A6C797C,0x31CDA5B6,0xDD2FC0E8,0x868E1C22,0x002A17E3,0x5B8BCB29,0xB769AE77,0xECC872BD,
		0x13380389,0x4899DF43,0xA47BBA1D,0xFFDA66D7,0x797E6D16,0x22DFB1DC,0xCE3DD482,0x959C0848,
		0xC7B4DEB7,0x9C15027D,0x70F76723,0x2B56BBE9,0xADF2B028,0xF6536CE2,0x1AB109BC,0x4110D576,
		0xE190F663,0xBA312AA9,0x56D34FF7,0x0D72933D,0x8BD698FC,0xD0774436,0x3C952168,0x6734FDA2,
		0x351C2B5D,0x6EBDF797,0x825F92C9,0xD9FE4E03,0x5F5A45C2,0x04FB9908,0xE819FC56,0xB3B8209C,
		0x4C4851A8,0x17E98D62,0xFB0BE83C,0xA0AA34F6,0x260E3F37,0x7DAFE3FD,0x914D86A3,0xCAEC5A69,
		0x98C48C96,0xC365505C,0x2F873502,0x7426E9C8,0xF282E209,0xA9233EC3,0x45C15B9D,0x1E608757,
		0x79005533,0x22A189F9,0xCE43ECA7,0x95E2306D,0x13463BAC,0x48E7E766,0xA4058238,0xFFA45EF2,
		0xAD8C880D,0xF62D54C7,0x1ACF3199,0x416EED53,0xC7CAE692,0x9C6B3A58,0x70895F06,0x2B2883CC,
		0xD4D8F2F8,0x8F792E32,0x639B4B6C,0x383A97A6,0xBE9E9C67,0xE53F40AD,0x09DD25F3,0x527CF939,
		0x00542FC6,0x5BF5F30C,0xB7179652,0xECB64A98,0x6A124159,0x31B39D93,0xDD51F8CD,0x86F02407,
		0x26700712,0x7DD1DBD8,0x9133BE86,0xCA92624C,0x4C36698D,0x1797B547,0xFB75D019,0xA0D40CD3,
		0xF2FCDA2C,0xA95D06E6,0x45BF63B8,0x1E1EBF72,0x98BAB4B3,0xC31B6879,0x2FF90D27,0x7458D1ED,
		0x8BA8A0D9,0xD0097C13,0x3CEB194D,0x674AC587,0xE1EECE46,0xBA4F128C,0x56AD77D2,0x0D0CAB18,
		0x5F247DE7,0x0485A12D,0xE867C473,0xB3C618B9,0x35621378,0x6EC3CFB2,0x8221AAEC,0xD9807626,
		0xC7E0F171,0x9C412DBB,0x70A348E5,0x2B02942F,0xADA69FEE,0xF6074324,0x1AE5267A,0x4144FAB0,
		0x136C2C4F,0x48CDF085,0xA42F95DB,0xFF8E4911,0x792A42D0,0x228B9E1A,0xCE69FB44,0x95C8278E,
		0x6A3856BA,0x31998A70,0xDD7BEF2E,0x86DA33E4,0x007E3825,0x5BDFE4EF,0xB73D81B1,0xEC9C5D7B,
		0xBEB48B84,0xE515574E,0x09F73210,0x5256EEDA,0xD4F2E51B,0x8F5339D1,0x63B15C8F,0x38108045,
		0x9890A350,0xC3317F9A,0x2FD31AC4,0x7472C60E,0xF2D6CDCF,0xA9771105,0x4595745B,0x1E34A891,
		0x4C1C7E6E,0x17BDA2A4,0xFB5FC7FA,0xA0FE1B30,0x265A10F1,0x7DFBCC3B,0x9119A965,0xCAB875AF,
		0x3548049B,0x6EE9D851,0x820BBD0F,0xD9AA61C5,0x5F0E6A04,0x04AFB6CE,0xE84DD390,0xB3EC0F5A,
		0xE1C4D9A5,0xBA65056F,0x56876031,0x0D26BCFB,0x8B82B73A,0xD0236BF0,0x3CC10EAE,0x6760D264
	}
};


#if 0 /* precomputing table */
void CRC32Init_slicing8()
{
    static int beenHere = 0;
    if ( ! beenHere )
    {
        int32_t kPoly32 = 0x04C11DB7;
        size_t i;
        for (i = 0; i < 256; ++i)
        {
            int32_t crc = i << 24;
            size_t j;
            for (j = 0; j < 8; ++j)
            {
                /*if (crc < 0)
                    crc = (crc << 1) ^ kPoly32;
                else
                    crc <<= 1;*/
                crc = (crc << 1) ^ ((crc >> 31) & kPoly32);
            }
            sCRC32_tbl_[0][i] = crc;
        }
        for (int i = 0; i < 256; ++i)
        {
            sCRC32_tbl_[1][i] = (sCRC32_tbl_[0][i] << 8) ^ sCRC32_tbl_[0][sCRC32_tbl_[0][i] >> 24];
            sCRC32_tbl_[2][i] = (sCRC32_tbl_[1][i] << 8) ^ sCRC32_tbl_[0][sCRC32_tbl_[1][i] >> 24];
            sCRC32_tbl_[3][i] = (sCRC32_tbl_[2][i] << 8) ^ sCRC32_tbl_[0][sCRC32_tbl_[2][i] >> 24];

            sCRC32_tbl_[4][i] = (sCRC32_tbl_[3][i] << 8) ^ sCRC32_tbl_[0][sCRC32_tbl_[3][i] >> 24];
            sCRC32_tbl_[5][i] = (sCRC32_tbl_[4][i] << 8) ^ sCRC32_tbl_[0][sCRC32_tbl_[4][i] >> 24];
            sCRC32_tbl_[6][i] = (sCRC32_tbl_[5][i] << 8) ^ sCRC32_tbl_[0][sCRC32_tbl_[5][i] >> 24];
            sCRC32_tbl_[7][i] = (sCRC32_tbl_[6][i] << 8) ^ sCRC32_tbl_[0][sCRC32_tbl_[6][i] >> 24];
        }
        beenHere = 1;
    }
}
#endif
LIB_EXPORT void CC CRC32Init ( void ) {} /* TODO: some other files call this function. Need to delete those calls and then to delete this empty function */

static uint32_t CRC32_one_byte_lookup(uint32_t previousCrc32, const void *data, size_t length)
{
    uint32_t crc = previousCrc32;
    const uint8_t* currentChar = (const uint8_t*) data;
    while (length-- > 0)
    {
        uint32_t i = ( crc >> 24 ) ^ *currentChar;
        ++currentChar;
        crc <<= 8;
        crc ^= sCRC32_tbl_[0][i];
    }
    return crc;
}

#define QWORD_READ 0
#define INVERT_PREVIOUS_CRC 0

LIB_EXPORT uint32_t CC CRC32(uint32_t previousCrc32, const void *data, size_t length)
{
#if INVERT_PREVIOUS_CRC
    uint32_t crc = ~previousCrc32; /* same as previousCrc32 ^ 0xFFFFFFFF*/
#else
    uint32_t crc = previousCrc32;
#endif

#if QWORD_READ == 1
    const uint64_t* current = (const uint64_t*) data;
    size_t const ALIGN_BYTES = 8;
#else
    const uint32_t* current = (const uint32_t*) data;
    size_t const ALIGN_BYTES = 4;
#endif

    /* if 'data' is unaligned, process first unaligned bytes with simple algorithm,
    then apply slicing to the aligned remainder */
    size_t nFisrtUnalignedBytes = ((size_t)data % ALIGN_BYTES);
    if (nFisrtUnalignedBytes)
    {
        nFisrtUnalignedBytes = ALIGN_BYTES - nFisrtUnalignedBytes;
        crc = CRC32_one_byte_lookup(crc, data, nFisrtUnalignedBytes);
        length -= nFisrtUnalignedBytes;
        current = (const uint32_t*) ((char*)data + nFisrtUnalignedBytes);
    }

    /* process aligned data with slicing-by-8 algorithm */
    while (length >= 8)
    {
#if QWORD_READ == 1 /* this is slower than 2x4-bytes */
        uint64_t qword = *current++ ^ bswap_32(crc); /* in theory it should be no bswap for little-endian here */
        crc =
            sCRC32_tbl_[0][(uint8_t)(qword>>56)] ^
            sCRC32_tbl_[1][(uint8_t)(qword>>48)] ^
            sCRC32_tbl_[2][(uint8_t)(qword>>40)] ^
            sCRC32_tbl_[3][(uint8_t)(qword>>32)] ^
            sCRC32_tbl_[4][(uint8_t)(qword>>24)] ^
            sCRC32_tbl_[5][(uint8_t)(qword>>16)] ^
            sCRC32_tbl_[6][(uint8_t)(qword>> 8)] ^
            sCRC32_tbl_[7][(uint8_t)qword];
#else
        uint32_t one = *current++ ^ bswap_32(crc); /* in theory it should be no bswap for little-endian here */
        uint32_t two = *current++;
        crc =
            sCRC32_tbl_[0][(uint8_t)(two>>24)] ^
            sCRC32_tbl_[1][(uint8_t)(two>>16)] ^
            sCRC32_tbl_[2][(uint8_t)(two>> 8)] ^
            sCRC32_tbl_[3][(uint8_t)two] ^
            sCRC32_tbl_[4][(uint8_t)(one>>24)] ^
            sCRC32_tbl_[5][(uint8_t)(one>>16)] ^
            sCRC32_tbl_[6][(uint8_t)(one>> 8)] ^
            sCRC32_tbl_[7][(uint8_t)one];
#endif

        length -= 8;
    }

    /* remaining 1 to 7 bytes (standard algorithm) */
    crc = CRC32_one_byte_lookup(crc, current, length);

#if INVERT_PREVIOUS_CRC
    return ~crc; // same as crc ^ 0xFFFFFFFF
#else
    return crc;
#endif
}

#endif /* SLOW_CRC */
