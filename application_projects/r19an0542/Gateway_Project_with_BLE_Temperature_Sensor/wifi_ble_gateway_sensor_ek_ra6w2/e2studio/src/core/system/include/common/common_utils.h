/**
 ****************************************************************************************
 *
 * @file common_util.h
 *
 * @brief Define for System common
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


#ifndef	__COMMON_UTILS_H__
#define	__COMMON_UTILS_H__



/* ANSI CODE */
#define ESCCODE						"\33"

/* Set  Attributes */
#define ANSI_BOLD					"\33[1m"
#define ANSI_UNDERLINE				"\33[4m"
#define ANSI_BLINK					"\33[5m"
#define ANSI_REVERSE				"\33[7m"

#define ANSI_R_BOLD					"\33[21m"
#define ANSI_R_UNDERLINE			"\33[24m"
#define ANSI_R_BLINK				"\33[25m"
#define ANSI_R_REVERSE				"\33[27m"

/* Rset  Attributes */
#define ANSI_NORMAL					"\33[0m"
#define ANSI_RESET_BOLD				"\33[21m"
#define ANSI_RESET_UNDERLINE		"\33[24m"
#define ANSI_RESET_BLINK			"\33[25m"
#define ANSI_RESET_REVERSE			"\33[27m"

/* Control */
#define ANSI_CLEAR					"\33[2J"
#define ANSI_CURON					"\33[?25h"
#define ANSI_CUROFF					"\33[?25l"
#define ANSI_BELL					"\7"
#define ANSI_ERASE					"\33[0J"
#define ANSI_LEFT          		  	"\33[1D"
#define ANSI_RIGHT         		  	"\33[1C"

/* Foreground Color (text) */
#define ANSI_COLOR_BLACK			"\33[30m"
#define ANSI_COLOR_RED				"\33[31m"
#define ANSI_COLOR_GREEN			"\33[32m"
#define ANSI_COLOR_YELLOW			"\33[33m"
#define ANSI_COLOR_BLUE				"\33[34m"
#define ANSI_COLOR_MAGENTA			"\33[35m"
#define ANSI_COLOR_CYAN				"\33[36m"
#define ANSI_COLOR_WHITE			"\33[37m"
#define ANSI_COLOR_LIGHT_RED		"\33[1;31m"
#define ANSI_COLOR_LIGHT_GREEN		"\33[1;32m"
#define ANSI_COLOR_LIGHT_YELLOW		"\33[1;33m"
#define ANSI_COLOR_LIGHT_BLUE		"\33[1;34m"
#define ANSI_COLOR_LIGHT_MAGENTA	"\33[1;35m"
#define ANSI_COLOR_LIGHT_CYAN		"\33[1;36m"
#define ANSI_COLOR_LIGHT_WHITE		"\33[1;37m"
#define ANSI_COLOR_DEFULT			"\33[0;39m"

/* Background Color */
#define ANSI_BCOLOR_BLACK			"\33[40m"
#define ANSI_BCOLOR_RED				"\33[41m"
#define ANSI_BCOLOR_GREEN			"\33[42m"
#define ANSI_BCOLOR_YELLOW			"\33[43m"
#define ANSI_BCOLOR_BLUE			"\33[44m"
#define ANSI_BCOLOR_MAGENTA			"\33[45m"
#define ANSI_BCOLOR_CYAN			"\33[46m"
#define ANSI_BCOLOR_WHITE			"\33[47m"
#define ANSI_BCOLOR_LIGHT_RED		"\33[1;41m"
#define ANSI_BCOLOR_LIGHT_GREEN		"\33[1;42m"
#define ANSI_BCOLOR_LIGHT_YELLOW	"\33[1;43m"
#define ANSI_BCOLOR_LIGHT_BLUE		"\33[1;44m"
#define ANSI_BCOLOR_LIGHT_MAGENTA	"\33[1;45m"
#define ANSI_BCOLOR_LIGHT_CYAN		"\33[1;46m"
#define ANSI_BCOLOR_LIGHT_WHITE		"\33[1:47m"

#define ANSI_BCOLOR_DEFULT			"\33[0;49m"



#define VT_CLEAR					PRINTF("\33[2J")
#define VT_CURPOS(X,Y)				PRINTF("\33[%d;%dH", Y, X)
#define VT_NORMAL					PRINTF("\33[0m")
#define VT_BOLD						PRINTF("\33[1m")
#define VT_BLINK					PRINTF("\33[5m")
#define VT_REVERSE					PRINTF("\33[7m")
#define VT_CURON					PRINTF("\33[?25h")
#define VT_CUROFF					PRINTF("\33[?25l")
#define VT_BELL						PRINTF("\007")
#define VT_ERASE					PRINTF("\33[0J")
#define VT_LEFT						PRINTF("\33[1D")
#define VT_RIGHT					PRINTF("\33[1C")
#define VT_LINECLEAR(X)				VT_CURPOS(1, X); PRINTF("\33[2K")
#define VT_COLORBLACK				PRINTF("\33[30m")
#define VT_COLORRED					PRINTF("\33[31m")
#define VT_COLORGREEN				PRINTF("\33[32m")
#define VT_COLORYELLOW				PRINTF("\33[33m")
#define VT_COLORBLUE				PRINTF("\33[34m")
#define VT_COLORMAGENTA				PRINTF("\33[35m")
#define VT_COLORCYAN				PRINTF("\33[36m")
#define VT_COLORWHITE				PRINTF("\33[37m")
#define VT_COLORDEFULT				PRINTF("\33[39m")
#define VT_BCOLORBLACK				PRINTF("\33[40m")
#define VT_BCOLORRED				PRINTF("\33[41m")
#define VT_BCOLORGREEN				PRINTF("\33[42m")
#define VT_BCOLORYELLOW				PRINTF("\33[43m")
#define VT_BCOLORBLUE				PRINTF("\33[44m")
#define VT_BCOLORMAGENTA			PRINTF("\33[45m")
#define VT_BCOLORCYAN				PRINTF("\33[46m")
#define VT_BCOLORWHITE				PRINTF("\33[47m")
#define VT_BDEFULT					PRINTF("\33[49m")
#define VT_COLOROFF					VT_BOLD;VT_BCOLORBLACK;VT_COLORWHITE


//ISDIGIT onderzoekt of een karakter numeric is
#define ISDIGIT(c)( (c < '0' || c > '9') ? 0 : 1)

/*
ex)
print_separate_bar("=", 10, 2);

"==========\n\n"
*/
void print_separate_bar(UCHAR text, UCHAR loop_count, UCHAR CR_loop_count);

#ifdef	__SUPPORT_IPV6__
static unsigned int _parseDecimal ( const char** pchCursor );
static unsigned int _parseHex ( const char** pchCursor );
#endif // __SUPPORT_IPV6__

//Parse a textual IPv4 or IPv6 address, optionally with port, into a binary
//array (for the address, in network order), and an optionally provided port.
//Also, indicate which of those forms (4 or 6) was parsed.  Return true on
//success.  ppszText must be a nul-terminated ASCII string.  It will be
//updated to point to the character which terminated parsing (so you can carry
//on with other things.  abyAddr must be 16 bytes.  You can provide NULL for
//abyAddr, nPort, bIsIPv6, if you are not interested in any of those
//informations.  If we request port, but there is no port part, then nPort will
//be set to 0.  There may be no whitespace leading or internal (though this may
//be used to terminate a successful parse.

//Note:  the binary address and integer port are in network order.
int ParseIPv4OrIPv6 ( const char** ppszText, unsigned char* abyAddr, int* pnPort, int* pbIsIPv6 );

//simple version if we want don't care about knowing how much we ate
int ParseIPv4OrIPv6_2 ( const char* pszText, unsigned char* abyAddr, int* pnPort, int* pbIsIPv6 );

int ParseIPv6ToLong ( const char* pszText, ULONG* ipv6addr, int* pnPort);


void pntdumpbin ( unsigned char* pbyBin, int nLen, int simple);
void bin2ipstr ( unsigned char* pbyBin, int nLen, int simple, unsigned char* ip_str);

/* LONG[4] ==> UINT[8] */
void ipv6long2int(ULONG *ipv6_long, UINT *ipv6_int);

/* LONG[4] ==> String */
void ipv6long2str(ULONG *ipv6_long,  UCHAR* ipv6_str);

/* UCHAR[16] ==> LONG[4] */
void ipv6uchar2Long(UCHAR *ipv6_uchar, ULONG *ipv6_long);

/* LONG[4] ==> UCHAR[16] */
void ipv6Long2uchar(ULONG *ipv6_long, UCHAR *ipv6_uchar);

int read_nvram_int(const char *name, int *_val);
char *read_nvram_string(const char *name);

int write_nvram_int(const char *name, int val);
int write_nvram_string(const char *name, const char *val);
int delete_nvram_env(const char *name);
int clear_nvram_env(void);

int write_tmp_nvram_int(const char *name, int val);
int write_tmp_nvram_string(const char *name, const char *val);
int delete_tmp_nvram_env(const char *name);
int clear_tmp_nvram_env(void);

int save_tmp_nvram(void);

int da16x_cli_reply(char *cmdline, char *delimit, char *cli_reply);
UINT factory_reset(int reboot_flag);
long pow_long(long x, int order);
int calcbits(long mask);
void longtoip(long ip, char *ipbuf);
long iptolong(char *ip);
int getipdigit(long ipaddress, int digit);
int isvalidip(char *theip);
int is_in_valid_ip_class(char *theip);
int isvalidmask(char *theip);
long subnetRangeLastIP(long ip, long subnet);
long subnetRangeFirstIP(long ip, long subnet);
long subnetBCIP(long ip, long subnet);
int isvalidIPsubnetRange(long ip, long subnetip, long subnet);
int isvalidIPrange(long ip, long firstIP, long lastIP);
int isvalid_domain(char *str);
void ulltoa(unsigned long long value, char *buf, int radix);
char *lltoa(long long value, char *buf, int radix);
int roundToInt(double x);
int coilToInt(double x);
int compare_macaddr(char* macaddr1, char* macaddr2);
int gen_ssid(char* prefix, int iface, int quotation, char* ssid, int size);
int recovery_NVRAM(void);
int copy_nvram_flash(UINT dest, UINT src, UINT len);
#if 0 //CHAND
UINT writeDataToFlash(UINT sflash_addr, UCHAR *wr_buf, UINT wr_size);
#endif //CHAND
//APP_CORE
UINT writeDataToMCU(UINT offset, UINT tot_len, UINT r_len, UINT *srcMemAddr, UINT size);
UINT getDigitNum(UINT num);

#endif	/* __COMMON_UTILS_H__ */

/* EOF */
