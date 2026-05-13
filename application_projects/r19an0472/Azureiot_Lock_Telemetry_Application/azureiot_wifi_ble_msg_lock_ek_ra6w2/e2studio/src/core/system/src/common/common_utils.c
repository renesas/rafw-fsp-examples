/**
 ****************************************************************************************
 *
 * @file common_utils.c
 *
 * @brief Common APIs and functions for DA162xx
 *
 * Copyright (c) 2026 Renesas Electronics. All rights reserved.
 *
 * This software ("Software") is owned by Renesas Electronics.
 *
 * By using this Software you agree that Renesas Electronics retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Renesas Electronics is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Renesas Electronics products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */


//#include "sdk_type.h"
#include "custom_config_sdk.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "nvedit.h"
//#include "environ.h"
//#include "sys_cfg.h"
#include "ra6w1_image.h"

#include "common_def.h"
//#include "command_net.h"
#include "net_common.h"
//#include "da16x_network_common.h"

//#include "da16x_system.h"
#include "lwip/inet.h"

#define SFCOPY_BULKSIZE        4096
#define KEY_NVR_SAVE_MAC       "save_mac"
#define NVRAM_MAC              1
#define NVRAM_START_VALUE      0x00901FFC

#define ENVIRON_DEVICE         NVEDIT_SFLASH

#ifndef RM_MAP_PERSISTANT_W
void ra6w1_environ_lock(unsigned long flag);
#endif

/* External global functions */
extern HANDLE flash_image_open(UINT32 mode, UINT32 imghdr_offset, VOID *locker);
extern UINT32 flash_image_read(HANDLE handler, UINT32 img_offset, VOID *load_addr, UINT32 img_secsize);
extern void flash_image_close(HANDLE handler);


UINT32 flash_image_write(HANDLE handler, UINT32 img_offset, VOID *load_addr, UINT32 img_secsize);


/**
 * ex)
 * print_separate_bar("=", 10, 2);
 *
 * "==========\n\n"
 *
 */
void print_separate_bar(UCHAR text, UCHAR loop_count, UCHAR CR_loop_count)
{
    UCHAR prt_str[260];

    memset(prt_str, 0, 256);

    if ((loop_count + CR_loop_count) + 1 > 260) {
        loop_count = (UCHAR)(260 - (CR_loop_count - 1));
    }

    memset(prt_str, text, loop_count);

    if (CR_loop_count > 0) {
        memset(prt_str + loop_count, '\n', CR_loop_count);
    }
    PRINTF("%s", prt_str);
}

int hexaInt_strlen(int value)
{
    char tmp_str[9];
    memset(tmp_str, 0, 9);
    sprintf(tmp_str, "%x", value);
    return strlen(tmp_str);
}

#if defined (__SUPPORT_IPV6__) && defined (XIP_CACHE_BOOT)
static unsigned int _parseDecimal ( const char** pchCursor )
{
    unsigned int nVal = 0;
    char chNow;

    while ( chNow = **pchCursor, chNow >= '0' && chNow <= '9' ) {
        //shift digit in
        nVal *= 10;
        nVal += chNow - '0';

        ++*pchCursor;
    }

    return nVal;
}

static unsigned int _parseHex ( const char** pchCursor )
{
    unsigned int nVal = 0;
    char chNow;

    while ( chNow = **pchCursor & 0x5f, (chNow >= ('0'&0x5f) && chNow <= ('9'&0x5f)) || (chNow >= 'A' && chNow <= 'F') ) {
        unsigned char nybbleValue;

        chNow -= 0x10;    //scootch digital values down; hex now offset by x31
        nybbleValue = ( chNow > 9 ? chNow - (0x31-0x0a) : chNow );
        //
        //shift nybble in
        nVal <<= 4;
        nVal += nybbleValue;

        ++*pchCursor;
    }

    return nVal;
}

// Parse a textual IPv4 or IPv6 address, optionally with port, into a binary
// array (for the address, in network order), and an optionally provided port.
// Also, indicate which of those forms (4 or 6) was parsed.  Return true on
// success.  ppszText must be a nul-terminated ASCII string.  It will be
// updated to point to the character which terminated parsing (so you can carry
// on with other things.  abyAddr must be 16 bytes.  You can provide NULL for
// abyAddr, nPort, bIsIPv6, if you are not interested in any of those
// informations.  If we request port, but there is no port part, then nPort will
// be set to 0.  There may be no whitespace leading or internal (though this may
// be used to terminate a successful parse.
// Note:  the binary address and integer port are in network order.
int ParseIPv4OrIPv6 ( const char** ppszText, unsigned char* abyAddr, int* pnPort, int* pbIsIPv6 )
{
    unsigned char* abyAddrLocal;
    unsigned char abyDummyAddr[16];

    // Find first colon, dot, and open bracket
    const char* pchColon = strchr ( *ppszText, ':' );
    const char* pchDot = strchr ( *ppszText, '.' );
    const char* pchOpenBracket = strchr ( *ppszText, '[' );
    const char* pchCloseBracket = NULL;


    // we'll consider this to (probably) be IPv6 if we find an open
    // bracket, or an absence of dots, or if there is a colon, and it
    // precedes any dots that may or may not be there
    int bIsIPv6local = NULL != pchOpenBracket || NULL == pchDot ||
            ( NULL != pchColon && ( NULL == pchDot || pchColon < pchDot ) );

    //OK, now do a little further sanity check our initial guess...
    if (bIsIPv6local) {
        //if open bracket, then must have close bracket that follows somewhere
        pchCloseBracket = strchr ( *ppszText, ']' );
        if (NULL != pchOpenBracket && (NULL == pchCloseBracket || pchCloseBracket < pchOpenBracket)) {
            return 0;
        }
    } else {    //probably ipv4
        //dots must exist, and precede any colons
        if ( NULL == pchDot || ( NULL != pchColon && pchColon < pchDot ) ) {
            return 0;
        }
    }

    //we figured out this much so far....
    if ( NULL != pbIsIPv6 ) {
        *pbIsIPv6 = bIsIPv6local;
    }

    //especially for IPv6 (where we will be decompressing and validating)
    //we really need to have a working buffer even if the caller didn't
    //care about the results.
    abyAddrLocal = abyAddr;    //prefer to use the caller's
    if ( NULL == abyAddrLocal ) {    //but use a dummy if we must
        abyAddrLocal = abyDummyAddr;
    }

    //OK, there should be no correctly formed strings which are miscategorized,
    //and now any format errors will be found out as we continue parsing
    //according to plan.
    if ( ! bIsIPv6local ) {  //try to parse as IPv4
        //4 dotted quad decimal; optional port if there is a colon
        //since there are just 4, and because the last one can be terminated
        //differently, I'm just going to unroll any potential loop.
        unsigned char* pbyAddrCursor = abyAddrLocal;
        unsigned int nVal;
        const char* pszTextBefore = *ppszText;
        nVal =_parseDecimal ( ppszText );            //get first val
        if ( '.' != **ppszText || nVal > 255 || pszTextBefore == *ppszText )    //must be in range and followed by dot and nonempty
            return 0;
        *(pbyAddrCursor++) = (unsigned char) nVal;    //stick it in addr
        ++(*ppszText);    //past the dot

        pszTextBefore = *ppszText;
        nVal =_parseDecimal ( ppszText );            //get second val
        if ( '.' != **ppszText || nVal > 255 || pszTextBefore == *ppszText )
            return 0;
        *(pbyAddrCursor++) = (unsigned char) nVal;
        ++(*ppszText);    //past the dot

        pszTextBefore = *ppszText;
        nVal =_parseDecimal ( ppszText );            //get third val
        if ( '.' != **ppszText || nVal > 255 || pszTextBefore == *ppszText )
            return 0;
        *(pbyAddrCursor++) = (unsigned char) nVal;
        ++(*ppszText);    //past the dot

        pszTextBefore = *ppszText;
        nVal =_parseDecimal ( ppszText );            //get fourth val
        if ( nVal > 255 || pszTextBefore == *ppszText )    //(we can terminate this one in several ways)
            return 0;
        *(pbyAddrCursor++) = (unsigned char) nVal;

        if ( ':' == **ppszText && NULL != pnPort ) {   //have port part, and we want it
            unsigned short usPortNetwork = 0;    //save value in network order

            ++(*ppszText);    //past the colon
            pszTextBefore = *ppszText;
            nVal =_parseDecimal ( ppszText );
            if ( nVal > 65535 || pszTextBefore == *ppszText ) {
                return 0;
            }

            ((unsigned char*)&usPortNetwork)[0] = ( nVal & 0xff00 ) >> 8;
            ((unsigned char*)&usPortNetwork)[1] = ( nVal & 0xff );
            *pnPort = usPortNetwork;

            return 1;
        } else {   //finished just with ip address
            if ( NULL != pnPort ) {
                *pnPort = 0;    //indicate we have no port part
            }
            return 1;
        }
    } else {   //try to parse as IPv6
        unsigned char* pbyAddrCursor;
        unsigned char* pbyZerosLoc;
        int bIPv4Detected;
        int nIdx;

        //up to 8 16-bit hex quantities, separated by colons, with at most one
        //empty quantity, acting as a stretchy run of zeroes.  optional port
        //if there are brackets followed by colon and decimal port number.
        //A further form allows an ipv4 dotted quad instead of the last two
        //16-bit quantities, but only if in the ipv4 space ::ffff:x:x .

        if ( NULL != pchOpenBracket )    //start past the open bracket, if it exists
            *ppszText = pchOpenBracket + 1;

        pbyAddrCursor = abyAddrLocal;
        pbyZerosLoc = NULL;    //if we find a 'zero compression' location
        bIPv4Detected = 0;
        for ( nIdx = 0; nIdx < 8; ++nIdx ) {   //we've got up to 8 of these, so we will use a loop
            const char* pszTextBefore = *ppszText;
            unsigned nVal =_parseHex ( ppszText );        //get value; these are hex
            if ( pszTextBefore == *ppszText ) {   //if empty, we are zero compressing; note the loc
                if ( NULL != pbyZerosLoc ) {   //there can be only one!
                    //unless it's a terminal empty field, then this is OK, it just means we're done with the host part
                    if ( pbyZerosLoc == pbyAddrCursor ) {
                        --nIdx;
                        break;
                    }
                    return 0;    //otherwise, it's a format error
                }

                if ( ':' != **ppszText ) {    //empty field can only be via :
                    return 0;
                }
                if ( 0 == nIdx ) {   //leading zero compression requires an extra peek, and adjustment
                    ++(*ppszText);
                    if ( ':' != **ppszText ) {
                        return 0;
                    }
                }

                pbyZerosLoc = pbyAddrCursor;
                ++(*ppszText);
            } else {
                if ( '.' == **ppszText ) {   //special case of ipv4 convenience notation
                    //who knows how to parse ipv4?  we do!
                    const char* pszTextlocal = pszTextBefore;    //back it up
                    unsigned char abyAddrlocal[16];
                    int bIsIPv6local;
                    int bParseResultlocal = ParseIPv4OrIPv6 ( &pszTextlocal, abyAddrlocal, NULL, &bIsIPv6local );

                    *ppszText = pszTextlocal;    //success or fail, remember the terminating char
                    if ( ! bParseResultlocal || bIsIPv6local ) {   //must parse and must be ipv4
                        return 0;
                    }

                    //transfer addrlocal into the present location
                    *(pbyAddrCursor++) = abyAddrlocal[0];
                    *(pbyAddrCursor++) = abyAddrlocal[1];
                    *(pbyAddrCursor++) = abyAddrlocal[2];
                    *(pbyAddrCursor++) = abyAddrlocal[3];
                    ++nIdx;    //pretend like we took another short, since the ipv4 effectively is two shorts
                    bIPv4Detected = 1;    //remember how we got here for further validation later
                    break;    //totally done with address
                }

                if ( nVal > 65535 )    //must be 16 bit quantity
                    return 0;

                *(pbyAddrCursor++) = nVal >> 8;        //transfer in network order
                *(pbyAddrCursor++) = nVal & 0xff;
                if ( ':' == **ppszText ) {   //typical case inside; carry on
                    ++(*ppszText);
                } else {   //some other terminating character; done with this parsing parts
                    break;
                }
            }
        }

        //handle any zero compression we found
        if ( NULL != pbyZerosLoc ) {
            int nHead = (int)( pbyZerosLoc - abyAddrLocal );    //how much before zero compression
            int nTail = nIdx * 2 - (int)( pbyZerosLoc - abyAddrLocal );    //how much after zero compression
            int nZeros = 16 - nTail - nHead;        //how much zeros
            memmove ( &abyAddrLocal[16-nTail], pbyZerosLoc, nTail );    //scootch stuff down
            memset ( pbyZerosLoc, 0, nZeros );        //clear the compressed zeros
        }

        //validation of ipv4 subspace ::ffff:x.x
        if ( bIPv4Detected ) {
            static const unsigned char abyPfx[] = { 0,0, 0,0, 0,0, 0,0, 0,0, 0xff,0xff };
            if ( 0 != memcmp ( abyAddrLocal, abyPfx, sizeof(abyPfx) ) )
                return 0;
        }

        //close bracket
        if ( NULL != pchOpenBracket ) {
            if ( ']' != **ppszText )
                return 0;
            ++(*ppszText);
        }

        if ( ':' == **ppszText && NULL != pnPort ) {   //have port part, and we want it
            const char* pszTextBefore;
            unsigned int nVal;
            unsigned short usPortNetwork = 0;    //save value in network order

            ++(*ppszText);    //past the colon
            pszTextBefore = *ppszText;
            pszTextBefore = *ppszText;
            nVal =_parseDecimal ( ppszText );
            if ( nVal > 65535 || pszTextBefore == *ppszText )
                return 0;
            ((unsigned char*)&usPortNetwork)[0] = ( nVal & 0xff00 ) >> 8;
            ((unsigned char*)&usPortNetwork)[1] = ( nVal & 0xff );
            *pnPort = usPortNetwork;
            return 1;
        } else {   //finished just with ip address
            if ( NULL != pnPort ) {
                *pnPort = 0;    //indicate we have no port part
            }
            return 1;
        }
    }
}


//simple version if we want don't care about knowing how much we ate
int ParseIPv4OrIPv6_2 ( const char* pszText, unsigned char* abyAddr, int* pnPort, int* pbIsIPv6 )
{
    const char* pszTextLocal = pszText;
    return ParseIPv4OrIPv6 ( &pszTextLocal, abyAddr, pnPort, pbIsIPv6);
}

/*
simple Type : 4  Display short address with 4-digit '0'
ex)  FE80::A1E6:0000:E145:F6E0

simple Type : 1  Display short address with 1-digit '0'
ex)  FE80::A1E6:0:E145:F6E0

simple Type : 0 : Display all 4-digit '0'
ex)  FE80:0000:0001:0000:A1E6:0000:E145:F6E0
*/
static void bin2ipstr ( unsigned char* pbyBin, int nLen, int simple, unsigned char* ip_str)
{
    int i;
    UINT ipv6_uint[8];
    int idx = 0;

    if (nLen == 16) {
        for ( i = 0; i < nLen/2; ++i ) {
            ipv6_uint[i] = (pbyBin[i*2] << 8) | pbyBin[(i*2)+1];
        }

        if (simple) {
            for ( i = 0; i < nLen/2; ++i ) {
                /* PRINT Value  */
                if (   (i > 0 && ipv6_uint[i-1] == 0x0 && ipv6_uint[i] == 0x0)
                    || (i < (nLen/2)-1 && ipv6_uint[i] == 0x0 && ipv6_uint[i+1] == 0x0)) {
                    /* SKIP */ ;
                } else {
                    sprintf ((char*)ip_str+idx, (simple == 4) ? "%04x" : "%x", ipv6_uint[i]);
                    idx = idx + (simple == 4 ? 4 : hexaInt_strlen(ipv6_uint[i]));
                    //PRINTF ("(%d)", i);
                    //PRINTF ("inx(%d)=%x(len=%d)\n", i, ipv6_uint[i], hexaInt_strlen(ipv6_uint[i]));
                }

                /* PRINT ":" */
                if (i > 0) {
                    if (   ((ipv6_uint[i-1] == 0x0) && (ipv6_uint[i] == 0x0))
                        || (i < (nLen/2)-1 && (ipv6_uint[i] == 0x0) && (ipv6_uint[i+1] == 0x0))) {
                        if ((i < (nLen/2)-1 && ipv6_uint[i+1] != 0x0) || (ipv6_uint[i] == 0x0 && i == 0)) {
                            strcat((char*)ip_str, ":");
                            idx++;
                            //PRINTF("{%d}", i);
                        }
                        /* SKIP */
                    } else if (i < (nLen/2)-1) {
                        strcat((char*)ip_str, ":");
                        idx++;
                        //PRINTF("{%d}", i);
                    }
                } else {
                    /* i==0: no previous element; check next-zero condition only */
                    if (i < (nLen/2)-1 && (ipv6_uint[i] == 0x0) && (ipv6_uint[i+1] == 0x0)) {
                        strcat((char*)ip_str, ":");
                        idx++;
                        //PRINTF("{%d}", i);
                        /* SKIP */
                    } else if (i < (nLen/2)-1) {
                        strcat((char*)ip_str, ":");
                        idx++;
                        //PRINTF("{%d}", i);
                    }
                }
            }
        } else {            /* FULL Address */
            for (i = 0; i < (nLen / 2); i++) {
                sprintf ((char*)ip_str+idx, "%04x", ipv6_uint[i]);
                idx = idx + 4;

                if (i < 7) {
                    strcat((char*)ip_str, ":");
                    idx++;
                }
            }
        }
    } else {    /* IPv4 Address */
        sprintf ((char*)ip_str, "%d.%d.%d.%d", pbyBin[0], pbyBin[1], pbyBin[2], pbyBin[3]);
    }
}

/* LONG[4] ==> UINT[8] */
void ipv6long2int(ULONG *ipv6_long, UINT *ipv6_int)
{
    ipv6_int[0] = ipv6_long[0] >> 16;
    ipv6_int[1] = ipv6_long[0] & 0x0000FFFF;
    ipv6_int[2] = ipv6_long[1] >> 16;
    ipv6_int[3] = ipv6_long[1] & 0x0000FFFF;
    ipv6_int[4] = ipv6_long[2] >> 16;
    ipv6_int[5] = ipv6_long[2] & 0x0000FFFF;
    ipv6_int[6] = ipv6_long[3] >> 16;
    ipv6_int[7] = ipv6_long[3] & 0x0000FFFF;
}


/* UCHAR[16] ==> LONG[4] */
void ipv6uchar2Long(UCHAR *ipv6_uchar, ULONG *ipv6_long)
{
    for (int i = 0; i < 4; i++ ) {
        ipv6_long[i] = (ipv6_uchar[i*4] << 24)
                    | (ipv6_uchar[(i*4)+1] << 16)
                    | (ipv6_uchar[(i*4)+2] << 8)
                    | (ipv6_uchar[(i*4)+3]);
    }
}

/* LONG[4] ==> UCHAR[16] */
void ipv6Long2uchar(ULONG *ipv6_long, UCHAR *ipv6_uchar)
{
    ipv6_uchar[0] = ipv6_long[0] >> 24;
    ipv6_uchar[1] = ipv6_long[0] >> 16 & 0x000000FF;
    ipv6_uchar[2] = ipv6_long[0] >> 8  & 0x000000FF;
    ipv6_uchar[3] = ipv6_long[0]       & 0x000000FF;

    ipv6_uchar[4] = ipv6_long[1] >> 24;
    ipv6_uchar[5] = ipv6_long[1] >> 16 & 0x000000FF;
    ipv6_uchar[6] = ipv6_long[1] >> 8  & 0x000000FF;
    ipv6_uchar[7] = ipv6_long[1]       & 0x000000FF;

    ipv6_uchar[8]  = ipv6_long[2] >> 24;
    ipv6_uchar[9]  = ipv6_long[2] >> 16 & 0x000000FF;
    ipv6_uchar[10] = ipv6_long[2] >> 8  & 0x000000FF;
    ipv6_uchar[11] = ipv6_long[2]       & 0x000000FF;

    ipv6_uchar[12] = ipv6_long[3] >> 24;
    ipv6_uchar[13] = ipv6_long[3] >> 16 & 0x000000FF;
    ipv6_uchar[14] = ipv6_long[3] >> 8  & 0x000000FF;
    ipv6_uchar[15] = ipv6_long[3]       & 0x000000FF;
}


/* LONG[4] ==> String */
void ipv6long2str(ULONG *ipv6_long,  UCHAR* ipv6_str)
{
    UCHAR ipv6_uchar[16];

    ipv6Long2uchar(ipv6_long, ipv6_uchar);
    bin2ipstr(ipv6_uchar,16, 1, ipv6_str);
}

/* Stirng ==> LONG[4] */
int ParseIPv6ToLong ( const char* pszText, ULONG* ipv6addr, int* pnPort)

{
    unsigned char abyAddr[16];
    int bIsIPv6;
    int nPort;
    int bSuccess;

    const char* pszTextCursor = pszText;
    bSuccess = ParseIPv4OrIPv6 ( &pszTextCursor, abyAddr, &nPort, &bIsIPv6 );

    if (!bSuccess || !bIsIPv6) {
        return 0;
    }

    if (ipv6addr) {
        ipv6uchar2Long(abyAddr, ipv6addr);
    }

    return 1;
}
#endif    /* __SUPPORT_IPV6__ && XIP_CACHE_BOOT*/

/**
 * x^n square root function
 */
#if 0 //azure_check[[::
long pow_long(long x, int order)
{
    long result = 1;
    int bit = 1;

    while (bit<=order) {
        if (bit&order)result *= x;
        x *= x;
        bit <<= 1;
    }

    return result;
}
#endif //azure_check]]


//returns the amount of bits used in a mask
int calcbits(long mask)
{
    int b, bits = 0;

    for (b = 0; b < 32; b++) {
        if (mask & (long) pow_long(2, b))
            bits++;
    }

    return bits;
}
#if 0 //azure_check[[::


/*** Modified by Bright ***/
//returns the ip notation (eg 127.0.0.1) from a long
void longtoip(long ip, char *ipbuf)
{
    int w, x, y, z;

    memset(ipbuf, 0, 16);
    w = (ip >> 24) & 0xFF;
    x = (ip >> 16) & 0xFF;
    y = (ip >> 8) & 0xFF;
    z = ip & 0xFF;
    sprintf(ipbuf, "%d.%d.%d.%d", w, x, y, z);
}
#endif //azure_check]]

//returns the long for an ip eg: 127.0.0.1
long iptolong(char *ip)
{
    char tmp[16];
    long ipa = 0;
    int len = (int) strlen(ip), byte, bcnt = 4 * 8;
    int to = 0;

    while (to < len) {
        memset(tmp, 0, 16);

        while (to < len) {
            if (ip[to] == '.')
            break;

            tmp[strlen(tmp)] = ip[to];
            to++;
        }

        bcnt -= 8;
        byte = atoi(tmp);
        ipa += (long) (byte << bcnt);
        to++;
    }

    return ipa;
}


//returns the digit of an ip address. eg: digit 2 of 193.19.136.1 = 19
int getipdigit(long ipaddress, int digit)
{
    char *ips, *ipe;
    int c;
    int status;

    ips = pvPortMalloc(16);

    memset(ips, 0, 16);

    longtoip(ipaddress, ips);

    if (digit > 4 || digit < 1) {
        vPortFree(ips);

        return 0;
    }

    for (c = 1; c < digit; c++) {
        while (*ips != '.') {
            ips++;
        }

        ips++;
    }

    ipe = ips;

    while (*ipe != '.' && *ipe != 0) {
        ipe++;
    }

    *ipe = 0;

    status = atoi(ips);

    vPortFree(ips);

    return status;
}

#if 0 //azure_check[[::


/*checks the ip format and value. returns 1 for ok, 0 for not ok */
int isvalidip(char *theip)
{
    int x, pcnt = 0;
    long newip;
    char ipdig[32];

    memset(ipdig, 0, 32);
    if ( strlen(theip) < 7 ) { // minimum ip address lengh: 7
        return pdFALSE;        // ex) "1.0.0.0": 7
    }

    for (x = 0; x <= (int) strlen(theip); x++) {
        if (ISDIGIT(theip[x])) {
            ipdig[strlen(ipdig)] = theip[x];
        } else {
            if (strlen(ipdig) == 0) {
                return pdFALSE;
            }

            if (theip[x] != '.' && theip[x] != 0) {
                return pdFALSE;
            }

            if (atoi(ipdig) > 255 || atoi(ipdig) < 0) {
                return pdFALSE;
            }

            pcnt++;

            if (pcnt > 4) {
                return pdFALSE;
            }
            memset(ipdig, 0, 32);
        }

        if (strlen(ipdig) > 3) {
            return pdFALSE;
        }
    }

    newip = iptolong(theip);

    if (newip == 0 || newip == -1 || getipdigit(newip, 1) == 0) {
        return pdFALSE;
    }

    return pdTRUE;
}

/* check validity of ip first and then check ip class : only class A, B, and C allowed */
int is_in_valid_ip_class(char *theip)
{
    int ret = pdTRUE;
    unsigned long ip_addr = 0;

    if(isvalidip(theip)) {
        ip_addr = (unsigned long)iptolong(theip);
    } else {
        return pdFALSE;
    }

    if (IN_CLASSA(ip_addr) || IN_CLASSB(ip_addr) || IN_CLASSC(ip_addr)) {
        ret = pdTRUE;
    } else {
        ret = pdFALSE;
    }

    return ret;
}

/*checks the subnetmask format and value. returns 1 for ok, 0 for not ok */

int isvalidmask(char *theip)
{
    long themask;
    int mbits, b = 31;
    themask = iptolong(theip);

    if (themask == 0 || themask == -1) {
        return pdFALSE;
    }

    mbits = calcbits(themask);

    if (mbits > (32 - 2)) {
        return pdFALSE;
    }

    //test consecutive bits with value 1
    while ((themask & (long)pow_long(2, b)) && b >= 0) {
        b--;
    }

    //test consecutive bits with value 0
    while (!(themask & (long)pow_long(2, b)) && b >= 0) {
        b--;
    }

    //if all the bits were not tested then invalid mask. ok = 111110000000... error: 1111100001000...
    if (b != -1) {
        return pdFALSE;
    }

    return pdTRUE;
}


/**
 * Broadcast IP address of Subnet Mask
 */

long subnetBCIP(long ip, long subnet)
{
    long bits = subnet ^ 0xffffffff;
    long endip = (ip | bits);

    return endip;
}


/**
 * Start IP address of Subnet Mask
 */

long subnetRangeFirstIP(long ip, long subnet)
{
    long startip = (ip & subnet) + 1;

    return startip;
}

/**
 * End IP address of Subnet Mask
 */
long subnetRangeLastIP(long ip, long subnet)
{
    return subnetBCIP(ip, subnet) - 1;
}



/**
 * API to check the validity of Subnet mask
 */
int isvalidIPsubnetRange(long ip, long subnetip, long subnet)
{
    int    status;
    long    firstIP;
    long    lastIP;

    firstIP = subnetRangeFirstIP(subnetip, subnet);
    lastIP = subnetRangeLastIP(subnetip, subnet);

    if (firstIP <= ip && lastIP >= ip) {
        status = pdTRUE;
    } else {
        status = pdFALSE;
    }

    return status;
}


/**
 * API to check the validity of IP address range
 */
int isvalidIPrange(long ip, long firstIP, long lastIP)
{
    int    status;
    if (firstIP <= ip && lastIP >= ip) {
        status = pdTRUE;
    } else {
        status = pdFALSE;
    }

    return status;
}
#endif //azure_check]]

int isvalid_domain(char *str)
{
    unsigned char count = 0;

    for (unsigned int i = 0; i < strlen(str); i++) {

        if( str[i] == '.' ) {
            count++;
        }
    }

    if ((is_in_valid_ip_class(str) && count == 3)) {
        return 2;
    } else if (count > 0) {
        return 1;
    }

    return pdFALSE;
}


void ulltoa(unsigned long long value, char *buf, int radix)
{
    char    tmp[64 + 1];        /* Lowest radix is 2, so 64-bits plus a null */
    char    *p1 = tmp, *p2;
    static const char xlat[] = "0123456789abcdefghijklmnopqrstuvwxyz";

    if(radix < 2 || radix > 36) {
        return;
    }

    do {
        *p1++ = xlat[value % (unsigned)radix];
    } while((value /= (unsigned)radix));

    for(p2 = buf; p1 != tmp; *p2++ = *--p1) {
        /* nothing to do */
    }

    *p2 = '\0';
}

char *lltoa(long long value, char *buf, int radix)
{
    char *save = buf;

    if(radix == 10 && value < 0) {
        *buf++ = '-';
        value = -value;
    }

    ulltoa(value, buf, radix);

    return buf ? save : 0;
}

int coilToInt(double x) {
    if (x >= 0) {
        return (int) (x + 0.99);
    }

    return (int) (x - 0.99);
}


int roundToInt(double x) {
    if (x >= 0) {
        return (int) (x + 0.5);
    }

    return (int) (x - 0.5);
}


int compare_macaddr(char* macaddr1, char* macaddr2)
{
    int len1, len2;
    char tmp_macstr1[13];
    char tmp_macstr2[13];

    len1 = strlen(macaddr1);
    len2 = strlen(macaddr2);



    if (len1 != 12 && len1 != 17 && len2 != 12 && len2 != 17) {
        return 0;
    }

    if (len1 == 17) {
        memset(tmp_macstr1, 0, 13);
        sprintf(tmp_macstr1, "%c%c%c%c%c%c%c%c%c%c%c%c",
            macaddr1[0],  macaddr1[1],
            macaddr1[3],  macaddr1[4],
            macaddr1[6],  macaddr1[7],
            macaddr1[9],  macaddr1[10],
            macaddr1[12], macaddr1[13],
            macaddr1[15], macaddr1[16]);
    } else {
        strcpy(tmp_macstr1, macaddr1);
    }

    if (len2 == 17) {
        memset(tmp_macstr2, 0, 13);
        sprintf(tmp_macstr2, "%c%c%c%c%c%c%c%c%c%c%c%c",
            macaddr2[0],  macaddr2[1],
            macaddr2[3],  macaddr2[4],
            macaddr2[6],  macaddr2[7],
            macaddr2[9],  macaddr2[10],
            macaddr2[12], macaddr2[13],
            macaddr2[15], macaddr2[16]);
    } else {
        strcpy(tmp_macstr2, macaddr2);
    }

    if (strncasecmp(tmp_macstr1, tmp_macstr2, 12) == 0) { /* Same Mac Address */
        return 1;
    }
    return 0;
}
#if 0 //azure_check[[::
int gen_ssid(char* prefix, int iface, int quotation, char* ssid, int size)
{
    ULONG macmsw, maclsw;

    if ((quotation ? (strlen(prefix)+7+2) : (strlen(prefix)+7)) > (unsigned int)size) {
        if (strlen(prefix) >= (MAX_SSID_LEN)) {
            sprintf(ssid, "%s%s%s", quotation ? "\"" : "", prefix, quotation ? "\"" : "");
            return 0;
        } else {
            return -1;
        }
    }

    getMacAddrMswLsw(iface, &macmsw, &maclsw);

    snprintf(ssid, (size > (MAX_SSID_LEN+2)) ? (quotation ? (MAX_SSID_LEN+2):MAX_SSID_LEN):(size), "%s%s_%02lX%02lX%02lX%s",
        quotation ? "\"" : "",
        prefix,
        ((maclsw >> 16)&0x0ff),
        ((maclsw >> 8)&0x0ff),
        ((maclsw >> 0)&0x0ff),
        quotation ? "\"" : "");

    return 0;
}
#endif // azure_check]]

int gen_default_psk(char *default_psk)
{
    sprintf(default_psk, "12345678");

    return 0;
}

//APP_CORE
UINT writeDataToMCU(UINT offset, UINT tot_len, UINT r_len, UINT *srcMemAddr, UINT size)
{
#if defined (__SUPPORT_ATCMD__) && (defined (__SUPPORT_AZURE_IOT__) || defined (__SUPPORT_AWS_IOT__))
    extern UINT app_writeDataToMCU(UINT offset, UINT tot_len, UINT r_len, UINT *srcMemAddr, UINT size);
    UINT res;
    res = app_writeDataToMCU(offset, tot_len, r_len, srcMemAddr, size);
    return res;
#else
    DA16X_UNUSED_ARG(offset);
    DA16X_UNUSED_ARG(tot_len);
    DA16X_UNUSED_ARG(r_len);
    DA16X_UNUSED_ARG(srcMemAddr);
    DA16X_UNUSED_ARG(size);
    return 0;
#endif
}
#if 0 //azure_check[[::
#ifndef RM_MAP_PERSISTANT_W
UINT writeDataToFlash(UINT sflash_addr, UCHAR *wr_buf, UINT wr_size)
{
    HANDLE  sflash_handler;
    UINT    wr_buf_addr;
    UINT    offset;
    INT     remained_wr_size;
    UCHAR   *sf_sector_buf = NULL;
    UINT    status = TRUE;

    sf_sector_buf = (UCHAR *)pvPortMalloc(SFCOPY_BULKSIZE);

    if (sf_sector_buf == NULL) {
        PRINTF("- SFLASH write error : pvPortMalloc fail...\n");

        return FALSE;
    }

    sflash_handler = flash_image_open((sizeof(UINT32)*8), sflash_addr, (VOID *)&da16x_environ_lock);

aligned_addr :
    wr_buf_addr = (UINT)wr_buf;
    remained_wr_size = wr_size;

    /* Check Serial-Flash sector alignment */
    if ((sflash_addr % SF_SECTOR_SZ) == 0) {
        for (offset = 0; offset < wr_size; offset += SF_SECTOR_SZ) {
            /* Make write data for 4KB sector */
            memset(sf_sector_buf, 0, SF_SECTOR_SZ);

            if (remained_wr_size < SF_SECTOR_SZ) {
                /* Pre-read to write */
                status = flash_image_read(sflash_handler,
                                            (sflash_addr + offset),
                                            (VOID *)sf_sector_buf,
                                            SF_SECTOR_SZ);
                if (status == 0) {
                    // Read Error
                    PRINTF("- SFLASH write : pre_read error : addr=0x%x\n", (sflash_addr + offset));
                    status = FALSE;
                    goto sflash_wr_end;
                }

                /* Copy remained write data */
                memcpy(sf_sector_buf, (void *)(wr_buf_addr + offset), remained_wr_size);
            } else {
                memcpy(sf_sector_buf, (void *)(wr_buf_addr + offset), SF_SECTOR_SZ);
            }

            remained_wr_size -= SF_SECTOR_SZ;

            flash_image_write(sflash_handler,
                        (sflash_addr + offset),
                        (VOID *)sf_sector_buf,
                        SF_SECTOR_SZ);

            status = TRUE;
        }
    } else {
        UINT    sflash_aligned_addr;

        offset = (sflash_addr % SF_SECTOR_SZ);
        sflash_aligned_addr = sflash_addr - offset;

        /* Make write data for 4KB sector */
        memset(sf_sector_buf, 0, SF_SECTOR_SZ);

        /* Pre-read to write */
        status = flash_image_read(sflash_handler, sflash_aligned_addr, (VOID *)sf_sector_buf, SF_SECTOR_SZ);
        if (status == 0) {
            // Read Error
            PRINTF("- SFLASH write : pre_read error : addr=0x%x\n", sflash_aligned_addr);

            status = FALSE;
            goto sflash_wr_end;
        }

        /* Write first sector */
        if (wr_size > SF_SECTOR_SZ - offset) {
            memcpy(&sf_sector_buf[offset], wr_buf, SF_SECTOR_SZ - offset);
        } else {
            memcpy(&sf_sector_buf[offset], wr_buf, wr_size);
        }

        flash_image_write(sflash_handler, sflash_aligned_addr, (VOID *)sf_sector_buf, SF_SECTOR_SZ);

        remained_wr_size -= (SF_SECTOR_SZ - offset);

        status = TRUE;

        /* Check remained write data */
        if (remained_wr_size > 0) {
            wr_buf = (UCHAR *)(&wr_buf[SF_SECTOR_SZ - offset]);
            wr_size -= (SF_SECTOR_SZ - offset);

            /* Next aligned sector address */
            sflash_addr = sflash_aligned_addr + SF_SECTOR_SZ;

            goto aligned_addr;
        } else {
            goto sflash_wr_end;
        }
    }

sflash_wr_end :
    flash_image_close(sflash_handler);
    vPortFree(sf_sector_buf);

    return status;
}
#endif
#endif //azure_check]]
int flash_nvram_check(HANDLE handler, int addr, int value)
{
    int     tmp_magic = 0;
    UINT32  status = 0;

    if (handler != 0) {
        status = flash_image_read(handler, addr, (VOID *)&tmp_magic, sizeof(int));
        if (status > 0) {
            ;
        }

        if (da16xfunc_ntohl(tmp_magic) == (UINT32)value) {
            return 0;
        } else {
            return 1;
        }
    } else {
        PRINTF(ANSI_COLOR_RED " [%s] Fail : NVRAM check error \n" ANSI_COLOR_DEFULT, __func__);
        return 1;
    }
}
#ifndef RM_MAP_PERSISTANT_W
int copy_nvram_flash(UINT dest, UINT src, UINT len)
{
    HANDLE  handler;
    UINT32  srcaddr, dstaddr;
    UINT32  length, status, offset;
    UINT8   *imgbuffer;

    srcaddr = src & (~(SFCOPY_BULKSIZE-1));
    dstaddr = dest & (~(SFCOPY_BULKSIZE-1));
    length  = len;

    imgbuffer = (UINT8 *)pvPortMalloc(SFCOPY_BULKSIZE);

    if (imgbuffer == NULL) {
        PRINTF("[%s] Fail : Allcate memory(%ld)\n", __func__, SFCOPY_BULKSIZE);
        return 1;
    }

    handler = (HANDLE)flash_image_open((sizeof(UINT32)*8), src, (VOID *)&da16x_environ_lock);
    if (handler == NULL) {
        PRINTF("[%s] Fail : To get Handler\n", __func__);
        vPortFree(imgbuffer);

        return 1;
    }

    for(offset = 0; offset < length; offset += SFCOPY_BULKSIZE ){
        status = flash_image_read(handler,
                (srcaddr+offset),
                imgbuffer,
                SFCOPY_BULKSIZE );

        if (status > 0) {
            status = flash_image_write(handler, (dstaddr+offset), imgbuffer, SFCOPY_BULKSIZE );
            if (status == 0) {
                PRINTF(ANSI_COLOR_RED " sflash write error \n" ANSI_COLOR_DEFULT);
            }
        }
        else {
            PRINTF(ANSI_COLOR_RED " sflash read error \n" ANSI_COLOR_DEFULT);
        }
    }

    vPortFree(imgbuffer);

    flash_image_close(handler);

    return 0;
}

int backup_NVRAM(void)
{
    return (copy_nvram_flash(SFLASH_NVRAM_BACKUP, SFLASH_NVRAM_ADDR, SFLASH_NVRAM_BACKUP_SZ));
}

int restore_NVRAM(void)
{
    da16x_environ_lock(TRUE);
    SYS_NVEDIT_IOCTL( NVEDIT_CMD_CLEAR, NULL);
    da16x_environ_lock(FALSE);

    return (copy_nvram_flash(SFLASH_NVRAM_ADDR, SFLASH_NVRAM_BACKUP, SFLASH_NVRAM_BACKUP_SZ));
}

int recovery_NVRAM(void)
{
    int    status = 0;

    da16x_environ_lock(TRUE);

    /* nvcfg update nor */
    status = SYS_NVCONFIG_UPDATE(NVEDIT_SFLASH);

    if (!status) {
        PRINTF("> Failed to configure NVRAM !!!\n");
    }

    da16x_environ_lock(FALSE);

    return da16x_clearenv(ENVIRON_DEVICE, "app");
}

/* Compare between nvram0 and nvram1.
 * Return: 0: same, 1: different. -1: Error
 */
int compare_NVRAM(void)
{
#ifdef  SUPPORT_SFLASH_DEVICE
    HANDLE handler = NULL;
    UINT32 addr_nvram0, addr_nvram1;
    UINT32 offset;
    UINT8 *nvram0_buf, *nvram1_buf;
    int status = 0;

    addr_nvram0 = SFLASH_NVRAM_ADDR & (~(SFCOPY_BULKSIZE-1));
    addr_nvram1 = SFLASH_NVRAM_BACKUP & (~(SFCOPY_BULKSIZE-1));

    nvram0_buf = (UINT8 *)pvPortMalloc(SFCOPY_BULKSIZE);
    if (nvram0_buf == NULL) {
        PRINTF("[%s] Failed to allocate memory(#0)\n", __func__);
        status = -1;
        return status;
    }

    nvram1_buf = (UINT8 *)pvPortMalloc(SFCOPY_BULKSIZE);
    if (nvram1_buf == NULL) {
        PRINTF("[%s] Failed to allocate memory(#1)\n", __func__);
        status = -1;
        goto free_buf0;
    }

    handler = (HANDLE)flash_image_open((sizeof(UINT32)*8), SFLASH_NVRAM_ADDR, (VOID *)&da16x_environ_lock);
    if (handler == NULL) {
        PRINTF("[%s] Failed to get handler\n", __func__);
        status = -1;
        goto free_buff1;
    }

    for (offset = 0; offset < SFLASH_NVRAM_BACKUP_SZ; offset += SFCOPY_BULKSIZE) {
        status = flash_image_read(handler, (addr_nvram0 + offset), nvram0_buf, SFCOPY_BULKSIZE);
        if (status == 0) {
            PRINTF(ANSI_COLOR_RED "> Failed to read NVRAM(#0)\n" ANSI_COLOR_DEFULT);
            status = -1;
            goto free_buff1;
        }

        status = flash_image_read(handler, (addr_nvram1 + offset), nvram1_buf,
                SFCOPY_BULKSIZE);
        if (status == 0) {
            PRINTF(ANSI_COLOR_RED "> Failed to read NVRAM(#1)\n" ANSI_COLOR_DEFULT);
            status = -1;
            goto free_buff1;
        }

        status = memcmp(nvram0_buf, nvram1_buf, SFCOPY_BULKSIZE);
        if (status != 0)
        {
            status = 1;
            break;
        }
    }

free_buff1:
    vPortFree(nvram1_buf);
free_buf0:
    vPortFree(nvram0_buf);
    flash_image_close(handler);

    return status;
#else
    return 0;
#endif
}

/*
 * Check Integrity of NVRAM data - Header / CRC of data.
 */
static int check_nvram_load_result(void)
{
    int ret = 0;
    UINT32 ioctldata[5];

    ioctldata[0] = NVEDIT_SFLASH;
    ioctldata[1] = SFLASH_UNIT_0;

    da16x_environ_lock(TRUE);
    SYS_NVEDIT_IOCTL(NVEDIT_CMD_CLEAR, NULL);
    ret = SYS_NVEDIT_IOCTL(NVEDIT_CMD_LOAD, ioctldata);
    da16x_environ_lock(FALSE);

    return ret;
}
#endif
/*
 * Check NVRAM status and recover NVRAM from backup NVRAM area.
 * It's not multithread-safe.
 */
#if 0 //azure_check[[::
void check_and_restore_nvram(void)
{
    /* Check integrity of nvram #0. */
    if (check_nvram_load_result() != TRUE) {
        /* Copy the backup data(#1) to nvram #0, and then check the data. */
        copy_nvram_flash(SFLASH_NVRAM_ADDR, SFLASH_NVRAM_BACKUP, SFLASH_NVRAM_BACKUP_SZ);

        if (check_nvram_load_result() != TRUE) {
            recovery_NVRAM();
        }
    }
}
#endif //azure_check]]
/*
 * Calculate number of digit ex) 1234 -> 4
 */
UINT getDigitNum(ULONG num)
{
    UINT count = 0;

    while(num != 0) {
        num = num/10;
        count++;
    }

    return count;
}


/* EOF */
