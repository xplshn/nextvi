#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vi.h"

/* the number of utf-8 characters in s */
int uc_slen(char *s)
{
	int n;
	for (n = 0; *s; n++)
		s = uc_end(s) + 1;
	return n;
}

/* find the beginning of the character at s[i] */
char *uc_beg(char *beg, char *s)
{
	while (s > beg && (((unsigned char) *s) & 0xc0) == 0x80)
		s--;
	return s;
}

/* find the end of the character at s[i] */
char *uc_end(char *s)
{
	if (!*s || !((unsigned char) *s & 0x80))
		return s;
	if (((unsigned char) *s & 0xc0) == 0xc0)
		s++;
	while (((unsigned char) *s & 0xc0) == 0x80)
		s++;
	return s - 1;
}

/* return a pointer to the character following s */
char *uc_next(char *s)
{
	s = uc_end(s);
	return *s ? s + 1 : s;
}

/* return a pointer to the character preceding s */
char *uc_prev(char *beg, char *s)
{
	return s == beg ? beg : uc_beg(beg, s - 1);
}

char *uc_lastline(char *s)
{
	char *r = strrchr(s, '\n');
	return r ? r + 1 : s;
}

/* allocate and return an array for the characters in s */
char **uc_chop(char *s, int *n)
{
	char **chrs;
	int i;
	*n = uc_slen(s);
	chrs = malloc((*n + 1) * sizeof(chrs[0]));
	for (i = 0; i < *n + 1; i++) {
		chrs[i] = s;
		s = uc_next(s);
	}
	return chrs;
}

char *uc_chr(char *s, int off)
{
	int i = 0;
	while (s && *s) {
		if (i++ == off)
			return s;
		s = uc_next(s);
	}
	return s && (off < 0 || i == off) ? s : "";
}

/* the number of characters between s and s + off */
int uc_off(char *s, int off)
{
	char *e = s + off;
	int i;
	for (i = 0; s < e && *s; i++)
		s = uc_next(s);
	return i;
}

char *uc_sub(char *s, int beg, int end)
{
	char *sbeg = uc_chr(s, beg);
	char *send = uc_chr(s, end);
	int len = sbeg && send && sbeg <= send ? send - sbeg : 0;
	char *r = malloc(len + 1);
	memcpy(r, sbeg, len);
	r[len] = '\0';
	return r;
}

char *uc_dup(char *s)
{
	char *r = malloc(strlen(s) + 1);
	return r ? strcpy(r, s) : NULL;
}

int uc_isspace(char *s)
{
	int c = s ? (unsigned char) *s : 0;
	return c <= 0x7f && isspace(c);
}

int uc_isprint(char *s)
{
	int c = s ? (unsigned char) *s : 0;
	return c > 0x7f || isprint(c);
}

int uc_isalpha(char *s)
{
	int c = s ? (unsigned char) *s : 0;
	return c > 0x7f || isalpha(c);
}

int uc_isdigit(char *s)
{
	int c = s ? (unsigned char) *s : 0;
	return c <= 0x7f && isdigit(c);
}

int uc_kind(char *c)
{
	if (uc_isspace(c))
		return 0;
	if (uc_isalpha(c) || uc_isdigit(c) || c[0] == '_')
		return 1;
	return 2;
}

#define UC_R2L(ch)	(((ch) & 0xff00) == 0x0600 || \
			((ch) & 0xfffc) == 0x200c || \
			((ch) & 0xff00) == 0xfb00 || \
			((ch) & 0xff00) == 0xfc00 || \
			((ch) & 0xff00) == 0xfe00)

/* sorted list of characters that can be shaped */
static struct achar {
	unsigned c;		/* utf-8 code */
	unsigned s;		/* single form */
	unsigned i;		/* initial form */
	unsigned m;		/* medial form */
	unsigned f;		/* final form */
} achars[] = {
	{0x0621, 0xfe80},				/* hamza */
	{0x0622, 0xfe81, 0, 0, 0xfe82},			/* alef madda */
	{0x0623, 0xfe83, 0, 0, 0xfe84},			/* alef hamza above */
	{0x0624, 0xfe85, 0, 0, 0xfe86},			/* waw hamza */
	{0x0625, 0xfe87, 0, 0, 0xfe88},			/* alef hamza below */
	{0x0626, 0xfe89, 0xfe8b, 0xfe8c, 0xfe8a},	/* yeh hamza */
	{0x0627, 0xfe8d, 0, 0, 0xfe8e},			/* alef */
	{0x0628, 0xfe8f, 0xfe91, 0xfe92, 0xfe90},	/* beh */
	{0x0629, 0xfe93, 0, 0, 0xfe94},			/* teh marbuta */
	{0x062a, 0xfe95, 0xfe97, 0xfe98, 0xfe96},	/* teh */
	{0x062b, 0xfe99, 0xfe9b, 0xfe9c, 0xfe9a},	/* theh */
	{0x062c, 0xfe9d, 0xfe9f, 0xfea0, 0xfe9e},	/* jeem */
	{0x062d, 0xfea1, 0xfea3, 0xfea4, 0xfea2},	/* hah */
	{0x062e, 0xfea5, 0xfea7, 0xfea8, 0xfea6},	/* khah */
	{0x062f, 0xfea9, 0, 0, 0xfeaa},			/* dal */
	{0x0630, 0xfeab, 0, 0, 0xfeac},			/* thal */
	{0x0631, 0xfead, 0, 0, 0xfeae},			/* reh */
	{0x0632, 0xfeaf, 0, 0, 0xfeb0},			/* zain */
	{0x0633, 0xfeb1, 0xfeb3, 0xfeb4, 0xfeb2},	/* seen */
	{0x0634, 0xfeb5, 0xfeb7, 0xfeb8, 0xfeb6},	/* sheen */
	{0x0635, 0xfeb9, 0xfebb, 0xfebc, 0xfeba},	/* sad */
	{0x0636, 0xfebd, 0xfebf, 0xfec0, 0xfebe},	/* dad */
	{0x0637, 0xfec1, 0xfec3, 0xfec4, 0xfec2},	/* tah */
	{0x0638, 0xfec5, 0xfec7, 0xfec8, 0xfec6},	/* zah */
	{0x0639, 0xfec9, 0xfecb, 0xfecc, 0xfeca},	/* ain */
	{0x063a, 0xfecd, 0xfecf, 0xfed0, 0xfece},	/* ghain */
	{0x0640, 0x640, 0x640, 0x640},			/* tatweel */
	{0x0641, 0xfed1, 0xfed3, 0xfed4, 0xfed2},	/* feh */
	{0x0642, 0xfed5, 0xfed7, 0xfed8, 0xfed6},	/* qaf */
	{0x0643, 0xfed9, 0xfedb, 0xfedc, 0xfeda},	/* kaf */
	{0x0644, 0xfedd, 0xfedf, 0xfee0, 0xfede},	/* lam */
	{0x0645, 0xfee1, 0xfee3, 0xfee4, 0xfee2},	/* meem */
	{0x0646, 0xfee5, 0xfee7, 0xfee8, 0xfee6},	/* noon */
	{0x0647, 0xfee9, 0xfeeb, 0xfeec, 0xfeea},	/* heh */
	{0x0648, 0xfeed, 0, 0, 0xfeee},			/* waw */
	{0x0649, 0xfeef, 0, 0, 0xfef0},			/* alef maksura */
	{0x064a, 0xfef1, 0xfef3, 0xfef4, 0xfef2},	/* yeh */
	{0x067e, 0xfb56, 0xfb58, 0xfb59, 0xfb57},	/* peh */
	{0x0686, 0xfb7a, 0xfb7c, 0xfb7d, 0xfb7b},	/* tcheh */
	{0x0698, 0xfb8a, 0, 0, 0xfb8b},			/* jeh */
	{0x06a9, 0xfb8e, 0xfb90, 0xfb91, 0xfb8f},	/* fkaf */
	{0x06af, 0xfb92, 0xfb94, 0xfb95, 0xfb93},	/* gaf */
	{0x06cc, 0xfbfc, 0xfbfe, 0xfbff, 0xfbfd},	/* fyeh */
	{0x200c},					/* ZWNJ */
	{0x200d, 0, 0x200d, 0x200d},			/* ZWJ */
};

static struct achar *find_achar(int c)
{
	int h, m, l;
	h = LEN(achars);
	l = 0;
	/* using binary search to find c */
	while (l < h) {
		m = (h + l) >> 1;
		if (achars[m].c == c)
			return &achars[m];
		if (c < achars[m].c)
			h = m;
		else
			l = m + 1;
	}
	return NULL;
}

static int can_join(int c1, int c2)
{
	struct achar *a1 = find_achar(c1);
	struct achar *a2 = find_achar(c2);
	return a1 && a2 && (a1->i || a1->m) && (a2->f || a2->m);
}

static int uc_cshape(int cur, int prev, int next)
{
	int c = cur;
	int join_prev, join_next;
	struct achar *ac = find_achar(c);
	if (!ac)		/* ignore non-Arabic characters */
		return c;
	join_prev = can_join(prev, c);
	join_next = can_join(c, next);
	if (join_prev && join_next)
		c = ac->m;
	if (join_prev && !join_next)
		c = ac->f;
	if (!join_prev && join_next)
		c = ac->i;
	if (!join_prev && !join_next)
		c = ac->c;	/* some fonts do not have a glyph for ac->s */
	return c ? c : cur;
}

/*
 * return nonzero for Arabic combining characters
 *
 * The standard Arabic diacritics:
 * + 0x064b: fathatan
 * + 0x064c: dammatan
 * + 0x064d: kasratan
 * + 0x064e: fatha
 * + 0x064f: damma
 * + 0x0650: kasra
 * + 0x0651: shadda
 * + 0x0652: sukun
 * + 0x0653: madda above
 * + 0x0654: hamza above
 * + 0x0655: hamza below
 * + 0x0670: superscript alef
 */
static int uc_acomb(int c)
{
	return (c >= 0x064b && c <= 0x0655) ||		/* the standard diacritics */
		(c >= 0xfc5e && c <= 0xfc63) ||		/* shadda ligatures */
		c == 0x0670;				/* superscript alef */
}

static void uc_cput(char *d, int c)
{
	int l = 0;
	if (c > 0xffff) {
		*d++ = 0xf0 | (c >> 18);
		l = 3;
	} else if (c > 0x7ff) {
		*d++ = 0xe0 | (c >> 12);
		l = 2;
	} else if (c > 0x7f) {
		*d++ = 0xc0 | (c >> 6);
		l = 1;
	} else {
		*d++ = c;
	}
	while (l--)
		*d++ = 0x80 | ((c >> (l * 6)) & 0x3f);
	*d = '\0';
}

/* shape the given arabic character; returns a static buffer */
char *uc_shape(char *beg, char *s)
{
	static char out[16];
	char *r;
	int tmp, curr, prev = 0, next = 0;
	uc_code(curr, s)
	if (!curr || !UC_R2L(curr))
		return NULL;
	r = s;
	while (r > beg) {
		r = uc_beg(beg, r - 1); uc_code(tmp, r)
		if (!uc_acomb(tmp)) {
			uc_code(prev, r)
			break;
		}
	}
	r = s;
	while (*r) {
		r = uc_next(r); uc_code(tmp, r)
		if (!uc_acomb(tmp)) {
			uc_code(next, r)
			break;
		}
	}
	uc_cput(out, uc_cshape(curr, prev, next));
	return out;
}

static int dwchars[][2] = {
	{0x1100, 0x115f}, {0x11a3, 0x11a7}, {0x11fa, 0x11ff}, {0x2329, 0x232a},
	{0x2e80, 0x2e99}, {0x2e9b, 0x2ef3}, {0x2f00, 0x2fd5}, {0x2ff0, 0x2ffb},
	{0x3000, 0x3029}, {0x3030, 0x303e}, {0x3041, 0x3096}, {0x309b, 0x30ff},
	{0x3105, 0x312d}, {0x3131, 0x318e}, {0x3190, 0x31b7}, {0x31c0, 0x31e3},
	{0x31f0, 0x321e}, {0x3220, 0x3247}, {0x3250, 0x32fe}, {0x3300, 0x4dbf},
	{0x4e00, 0xa48c}, {0xa490, 0xa4c6}, {0xa960, 0xa97c}, {0xac00, 0xd7a3},
	{0xd7b0, 0xd7c6}, {0xd7cb, 0xd7fb}, {0xf900, 0xfaff}, {0xfe10, 0xfe19},
	{0xfe30, 0xfe52}, {0xfe54, 0xfe66}, {0xfe68, 0xfe6b}, {0xff01, 0xff60},
	{0xffe0, 0xffe6}, {0x1f200, 0x1f200}, {0x1f210, 0x1f231}, {0x1f240, 0x1f248},
	{0x20000,0x2ffff},
};

static int zwchars[][2] = {
	{0x0300, 0x036f}, {0x0483, 0x0489}, {0x0591, 0x05bd}, {0x05bf, 0x05bf},
	{0x05c1, 0x05c2}, {0x05c4, 0x05c5}, {0x05c7, 0x05c7}, {0x0610, 0x061a},
	{0x064b, 0x065e}, {0x0670, 0x0670}, {0x06d6, 0x06dc}, {0x06de, 0x06e4},
	{0x06e7, 0x06e8}, {0x06ea, 0x06ed}, {0x0711, 0x0711}, {0x0730, 0x074a},
	{0x07a6, 0x07b0}, {0x07eb, 0x07f3}, {0x0816, 0x0819}, {0x081b, 0x0823},
	{0x0825, 0x0827}, {0x0829, 0x082d}, {0x0900, 0x0903}, {0x093c, 0x093c},
	{0x093e, 0x094e}, {0x0951, 0x0955}, {0x0962, 0x0963}, {0x0981, 0x0983},
	{0x09bc, 0x09bc}, {0x09be, 0x09c4}, {0x09c7, 0x09c8}, {0x09cb, 0x09cd},
	{0x09d7, 0x09d7}, {0x09e2, 0x09e3}, {0x0a01, 0x0a03}, {0x0a3c, 0x0a3c},
	{0x0a3e, 0x0a42}, {0x0a47, 0x0a48}, {0x0a4b, 0x0a4d}, {0x0a51, 0x0a51},
	{0x0a70, 0x0a71}, {0x0a75, 0x0a75}, {0x0a81, 0x0a83}, {0x0abc, 0x0abc},
	{0x0abe, 0x0ac5}, {0x0ac7, 0x0ac9}, {0x0acb, 0x0acd}, {0x0ae2, 0x0ae3},
	{0x0b01, 0x0b03}, {0x0b3c, 0x0b3c}, {0x0b3e, 0x0b44}, {0x0b47, 0x0b48},
	{0x0b4b, 0x0b4d}, {0x0b56, 0x0b57}, {0x0b62, 0x0b63}, {0x0b82, 0x0b82},
	{0x0bbe, 0x0bc2}, {0x0bc6, 0x0bc8}, {0x0bca, 0x0bcd}, {0x0bd7, 0x0bd7},
	{0x0c01, 0x0c03}, {0x0c3e, 0x0c44}, {0x0c46, 0x0c48}, {0x0c4a, 0x0c4d},
	{0x0c55, 0x0c56}, {0x0c62, 0x0c63}, {0x0c82, 0x0c83}, {0x0cbc, 0x0cbc},
	{0x0cbe, 0x0cc4}, {0x0cc6, 0x0cc8}, {0x0cca, 0x0ccd}, {0x0cd5, 0x0cd6},
	{0x0ce2, 0x0ce3}, {0x0d02, 0x0d03}, {0x0d3e, 0x0d44}, {0x0d46, 0x0d48},
	{0x0d4a, 0x0d4d}, {0x0d57, 0x0d57}, {0x0d62, 0x0d63}, {0x0d82, 0x0d83},
	{0x0dca, 0x0dca}, {0x0dcf, 0x0dd4}, {0x0dd6, 0x0dd6}, {0x0dd8, 0x0ddf},
	{0x0df2, 0x0df3}, {0x0e31, 0x0e31}, {0x0e34, 0x0e3a}, {0x0e47, 0x0e4e},
	{0x0eb1, 0x0eb1}, {0x0eb4, 0x0eb9}, {0x0ebb, 0x0ebc}, {0x0ec8, 0x0ecd},
	{0x0f18, 0x0f19}, {0x0f35, 0x0f35}, {0x0f37, 0x0f37}, {0x0f39, 0x0f39},
	{0x0f3e, 0x0f3f}, {0x0f71, 0x0f84}, {0x0f86, 0x0f87}, {0x0f90, 0x0f97},
	{0x0f99, 0x0fbc}, {0x0fc6, 0x0fc6}, {0x102b, 0x103e}, {0x1056, 0x1059},
	{0x105e, 0x1060}, {0x1062, 0x1064}, {0x1067, 0x106d}, {0x1071, 0x1074},
	{0x1082, 0x108d}, {0x108f, 0x108f}, {0x109a, 0x109d}, {0x135f, 0x135f},
	{0x1712, 0x1714}, {0x1732, 0x1734}, {0x1752, 0x1753}, {0x1772, 0x1773},
	{0x17b6, 0x17d3}, {0x17dd, 0x17dd}, {0x180b, 0x180d}, {0x18a9, 0x18a9},
	{0x1920, 0x192b}, {0x1930, 0x193b}, {0x19b0, 0x19c0}, {0x19c8, 0x19c9},
	{0x1a17, 0x1a1b}, {0x1a55, 0x1a5e}, {0x1a60, 0x1a7c}, {0x1a7f, 0x1a7f},
	{0x1b00, 0x1b04}, {0x1b34, 0x1b44}, {0x1b6b, 0x1b73}, {0x1b80, 0x1b82},
	{0x1ba1, 0x1baa}, {0x1c24, 0x1c37}, {0x1cd0, 0x1cd2}, {0x1cd4, 0x1ce8},
	{0x1ced, 0x1ced}, {0x1cf2, 0x1cf2}, {0x1dc0, 0x1de6}, {0x1dfd, 0x1dff},
	{0x200b, 0x200f},
	{0x20d0, 0x20f0}, {0x2cef, 0x2cf1}, {0x2de0, 0x2dff}, {0x302a, 0x302f},
	{0x3099, 0x309a}, {0xa66f, 0xa672}, {0xa67c, 0xa67d}, {0xa6f0, 0xa6f1},
	{0xa802, 0xa802}, {0xa806, 0xa806}, {0xa80b, 0xa80b}, {0xa823, 0xa827},
	{0xa880, 0xa881}, {0xa8b4, 0xa8c4}, {0xa8e0, 0xa8f1}, {0xa926, 0xa92d},
	{0xa947, 0xa953}, {0xa980, 0xa983}, {0xa9b3, 0xa9c0}, {0xaa29, 0xaa36},
	{0xaa43, 0xaa43}, {0xaa4c, 0xaa4d}, {0xaa7b, 0xaa7b}, {0xaab0, 0xaab0},
	{0xaab2, 0xaab4}, {0xaab7, 0xaab8}, {0xaabe, 0xaabf}, {0xaac1, 0xaac1},
	{0xabe3, 0xabea}, {0xabec, 0xabed}, {0xfb1e, 0xfb1e}, {0xfe00, 0xfe0f},
	{0xfe20, 0xfe26}, {0x101fd, 0x101fd}, {0x10a01, 0x10a03}, {0x10a05, 0x10a06},
	{0x10a0c, 0x10a0f}, {0x10a38, 0x10a3a}, {0x10a3f, 0x10a3f}, {0x11080, 0x11082},
	{0x110b0, 0x110ba}, {0x1d165, 0x1d169}, {0x1d16d, 0x1d172}, {0x1d17b, 0x1d182},
	{0x1d185, 0x1d18b}, {0x1d1aa, 0x1d1ad}, {0x1d242, 0x1d244}, {0xe0100, 0xe01ef}
};

static int bchars[][2] = {
	{0x00000, 0x0001f}, {0x00080, 0x0009f}, {0x00300, 0x0036f},
	{0x00379, 0x00379}, {0x00380, 0x00383}, {0x0038d, 0x0038d},
	{0x00483, 0x00489}, {0x00527, 0x00530}, {0x00558, 0x00558},
	{0x00588, 0x00588}, {0x0058c, 0x005bd}, {0x005c1, 0x005c2},
	{0x005c5, 0x005c5}, {0x005c8, 0x005cf}, {0x005ec, 0x005ef},
	{0x005f6, 0x00605}, {0x00611, 0x0061a}, {0x0061d, 0x0061d},
	{0x0064b, 0x0065f}, {0x006d6, 0x006e4}, {0x006e8, 0x006e8},
	{0x006eb, 0x006ed}, {0x0070f, 0x0070f}, {0x00730, 0x0074c},
	{0x007a7, 0x007b0}, {0x007b3, 0x007bf}, {0x007ec, 0x007f3},
	{0x007fc, 0x007ff}, {0x00817, 0x00819}, {0x0081c, 0x00823},
	{0x00826, 0x00827}, {0x0082a, 0x0082f}, {0x00840, 0x00903},
	{0x0093b, 0x0093c}, {0x0093f, 0x0094f}, {0x00952, 0x00957},
	{0x00963, 0x00963}, {0x00974, 0x00978}, {0x00981, 0x00984},
	{0x0098e, 0x0098e}, {0x00992, 0x00992}, {0x009b1, 0x009b1},
	{0x009b4, 0x009b5}, {0x009bb, 0x009bc}, {0x009bf, 0x009cd},
	{0x009d0, 0x009db}, {0x009e2, 0x009e5}, {0x009fd, 0x00a04},
	{0x00a0c, 0x00a0e}, {0x00a12, 0x00a12}, {0x00a31, 0x00a31},
	{0x00a37, 0x00a37}, {0x00a3b, 0x00a58}, {0x00a5f, 0x00a65},
	{0x00a71, 0x00a71}, {0x00a76, 0x00a84}, {0x00a92, 0x00a92},
	{0x00ab1, 0x00ab1}, {0x00aba, 0x00abc}, {0x00abf, 0x00acf},
	{0x00ad2, 0x00adf}, {0x00ae3, 0x00ae5}, {0x00af2, 0x00b04},
	{0x00b0e, 0x00b0e}, {0x00b12, 0x00b12}, {0x00b31, 0x00b31},
	{0x00b3a, 0x00b3c}, {0x00b3f, 0x00b5b}, {0x00b62, 0x00b65},
	{0x00b73, 0x00b82}, {0x00b8b, 0x00b8d}, {0x00b96, 0x00b98},
	{0x00b9d, 0x00b9d}, {0x00ba1, 0x00ba2}, {0x00ba6, 0x00ba7},
	{0x00bac, 0x00bad}, {0x00bbb, 0x00bcf}, {0x00bd2, 0x00be5},
	{0x00bfc, 0x00c04}, {0x00c11, 0x00c11}, {0x00c34, 0x00c34},
	{0x00c3b, 0x00c3c}, {0x00c3f, 0x00c57}, {0x00c5b, 0x00c5f},
	{0x00c63, 0x00c65}, {0x00c71, 0x00c77}, {0x00c81, 0x00c84},
	{0x00c91, 0x00c91}, {0x00cb4, 0x00cb4}, {0x00cbb, 0x00cbc},
	{0x00cbf, 0x00cdd}, {0x00ce2, 0x00ce5}, {0x00cf3, 0x00d04},
	{0x00d11, 0x00d11}, {0x00d3a, 0x00d3c}, {0x00d3f, 0x00d5f},
	{0x00d63, 0x00d65}, {0x00d77, 0x00d78}, {0x00d81, 0x00d84},
	{0x00d98, 0x00d99}, {0x00dbc, 0x00dbc}, {0x00dbf, 0x00dbf},
	{0x00dc8, 0x00df3}, {0x00df6, 0x00e00}, {0x00e34, 0x00e3e},
	{0x00e48, 0x00e4e}, {0x00e5d, 0x00e80}, {0x00e85, 0x00e86},
	{0x00e8b, 0x00e8c}, {0x00e8f, 0x00e93}, {0x00ea0, 0x00ea0},
	{0x00ea6, 0x00ea6}, {0x00ea9, 0x00ea9}, {0x00eb1, 0x00eb1},
	{0x00eb5, 0x00ebc}, {0x00ebf, 0x00ebf}, {0x00ec7, 0x00ecf},
	{0x00edb, 0x00edb}, {0x00edf, 0x00eff}, {0x00f19, 0x00f19},
	{0x00f37, 0x00f37}, {0x00f3e, 0x00f3f}, {0x00f6d, 0x00f84},
	{0x00f87, 0x00f87}, {0x00f8d, 0x00fbd}, {0x00fcd, 0x00fcd},
	{0x00fda, 0x00fff}, {0x0102c, 0x0103e}, {0x01057, 0x01059},
	{0x0105f, 0x01060}, {0x01063, 0x01064}, {0x01068, 0x0106d},
	{0x01072, 0x01074}, {0x01083, 0x0108d}, {0x0109a, 0x0109d},
	{0x010c7, 0x010cf}, {0x010fe, 0x010ff}, {0x0124e, 0x0124f},
	{0x01259, 0x01259}, {0x0125f, 0x0125f}, {0x0128e, 0x0128f},
	{0x012b6, 0x012b7}, {0x012c1, 0x012c1}, {0x012c7, 0x012c7},
	{0x01311, 0x01311}, {0x01317, 0x01317}, {0x0135c, 0x0135f},
	{0x0137e, 0x0137f}, {0x0139b, 0x0139f}, {0x013f6, 0x013ff},
	{0x0169e, 0x0169f}, {0x016f2, 0x016ff}, {0x01712, 0x0171f},
	{0x01733, 0x01734}, {0x01738, 0x0173f}, {0x01753, 0x0175f},
	{0x01771, 0x0177f}, {0x017b5, 0x017d3}, {0x017de, 0x017df},
	{0x017eb, 0x017ef}, {0x017fb, 0x017ff}, {0x0180c, 0x0180d},
	{0x0181a, 0x0181f}, {0x01879, 0x0187f}, {0x018ab, 0x018af},
	{0x018f7, 0x018ff}, {0x0191e, 0x0193f}, {0x01942, 0x01943},
	{0x0196f, 0x0196f}, {0x01976, 0x0197f}, {0x019ad, 0x019c0},
	{0x019c9, 0x019cf}, {0x019dc, 0x019dd}, {0x01a18, 0x01a1d},
	{0x01a56, 0x01a7f}, {0x01a8b, 0x01a8f}, {0x01a9b, 0x01a9f},
	{0x01aaf, 0x01b04}, {0x01b35, 0x01b44}, {0x01b4d, 0x01b4f},
	{0x01b6c, 0x01b73}, {0x01b7e, 0x01b82}, {0x01ba2, 0x01bad},
	{0x01bbb, 0x01bff}, {0x01c25, 0x01c3a}, {0x01c4b, 0x01c4c},
	{0x01c81, 0x01cd2}, {0x01cd5, 0x01ce8}, {0x01cf2, 0x01cff},
	{0x01dc1, 0x01dff}, {0x01f17, 0x01f17}, {0x01f1f, 0x01f1f},
	{0x01f47, 0x01f47}, {0x01f4f, 0x01f4f}, {0x01f5a, 0x01f5a},
	{0x01f5e, 0x01f5e}, {0x01f7f, 0x01f7f}, {0x01fc5, 0x01fc5},
	{0x01fd5, 0x01fd5}, {0x01ff0, 0x01ff1}, {0x01fff, 0x01fff},
	{0x0200c, 0x0200f}, {0x02029, 0x0202e}, {0x02061, 0x0206f},
	{0x02073, 0x02073}, {0x02095, 0x0209f}, {0x020ba, 0x020ff},
	{0x0218b, 0x0218f}, {0x023ea, 0x023ff}, {0x02428, 0x0243f},
	{0x0244c, 0x0245f}, {0x026e2, 0x026e2}, {0x026e5, 0x026e7},
	{0x02705, 0x02705}, {0x0270b, 0x0270b}, {0x0274c, 0x0274c},
	{0x02753, 0x02755}, {0x02760, 0x02760}, {0x02796, 0x02797},
	{0x027bf, 0x027bf}, {0x027cd, 0x027cf}, {0x02b4e, 0x02b4f},
	{0x02b5b, 0x02bff}, {0x02c5f, 0x02c5f}, {0x02cf0, 0x02cf8},
	{0x02d27, 0x02d2f}, {0x02d67, 0x02d6e}, {0x02d71, 0x02d7f},
	{0x02d98, 0x02d9f}, {0x02daf, 0x02daf}, {0x02dbf, 0x02dbf},
	{0x02dcf, 0x02dcf}, {0x02ddf, 0x02dff}, {0x02e33, 0x02e7f},
	{0x02ef4, 0x02eff}, {0x02fd7, 0x02fef}, {0x02ffd, 0x02fff},
	{0x0302b, 0x0302f}, {0x03097, 0x0309a}, {0x03101, 0x03104},
	{0x0312f, 0x03130}, {0x031b8, 0x031bf}, {0x031e5, 0x031ef},
	{0x032ff, 0x032ff}, {0x04db7, 0x04dbf}, {0x09fcd, 0x09fff},
	{0x0a48e, 0x0a48f}, {0x0a4c8, 0x0a4cf}, {0x0a62d, 0x0a63f},
	{0x0a661, 0x0a661}, {0x0a670, 0x0a672}, {0x0a675, 0x0a67d},
	{0x0a699, 0x0a69f}, {0x0a6f1, 0x0a6f1}, {0x0a6f9, 0x0a6ff},
	{0x0a78e, 0x0a7fa}, {0x0a806, 0x0a806}, {0x0a823, 0x0a827},
	{0x0a82d, 0x0a82f}, {0x0a83b, 0x0a83f}, {0x0a879, 0x0a881},
	{0x0a8b5, 0x0a8cd}, {0x0a8db, 0x0a8f1}, {0x0a8fd, 0x0a8ff},
	{0x0a927, 0x0a92d}, {0x0a948, 0x0a95e}, {0x0a97e, 0x0a983},
	{0x0a9b4, 0x0a9c0}, {0x0a9da, 0x0a9dd}, {0x0a9e1, 0x0a9ff},
	{0x0aa2a, 0x0aa3f}, {0x0aa4c, 0x0aa4f}, {0x0aa5b, 0x0aa5b},
	{0x0aa7c, 0x0aa7f}, {0x0aab2, 0x0aab4}, {0x0aab8, 0x0aab8},
	{0x0aabf, 0x0aabf}, {0x0aac3, 0x0aada}, {0x0aae1, 0x0abbf},
	{0x0abe4, 0x0abea}, {0x0abed, 0x0abef}, {0x0abfb, 0x0abff},
	{0x0d7a5, 0x0d7af}, {0x0d7c8, 0x0d7ca}, {0x0d7fd, 0x0f8ff},
	{0x0fa2f, 0x0fa2f}, {0x0fa6f, 0x0fa6f}, {0x0fadb, 0x0faff},
	{0x0fb08, 0x0fb12}, {0x0fb19, 0x0fb1c}, {0x0fb37, 0x0fb37},
	{0x0fb3f, 0x0fb3f}, {0x0fb45, 0x0fb45}, {0x0fbb3, 0x0fbd2},
	{0x0fd41, 0x0fd4f}, {0x0fd91, 0x0fd91}, {0x0fdc9, 0x0fdef},
	{0x0fdff, 0x0fe0f}, {0x0fe1b, 0x0fe2f}, {0x0fe67, 0x0fe67},
	{0x0fe6d, 0x0fe6f}, {0x0fefd, 0x0ff00}, {0x0ffc0, 0x0ffc1},
	{0x0ffc9, 0x0ffc9}, {0x0ffd1, 0x0ffd1}, {0x0ffd9, 0x0ffd9},
	{0x0ffde, 0x0ffdf}, {0x0ffef, 0x0fffb}, {0x0ffff, 0x0ffff},
	{0x10027, 0x10027}, {0x1003e, 0x1003e}, {0x1004f, 0x1004f},
	{0x1005f, 0x1007f}, {0x100fc, 0x100ff}, {0x10104, 0x10106},
	{0x10135, 0x10136}, {0x1018c, 0x1018f}, {0x1019d, 0x101cf},
	{0x101fe, 0x1027f}, {0x1029e, 0x1029f}, {0x102d2, 0x102ff},
	{0x10324, 0x1032f}, {0x1034c, 0x1037f}, {0x103c4, 0x103c7},
	{0x103d7, 0x103ff}, {0x1049f, 0x1049f}, {0x104ab, 0x107ff},
	{0x10807, 0x10807}, {0x10836, 0x10836}, {0x1083a, 0x1083b},
	{0x1083e, 0x1083e}, {0x10860, 0x108ff}, {0x1091d, 0x1091e},
	{0x1093b, 0x1093e}, {0x10941, 0x109ff}, {0x10a02, 0x10a0f},
	{0x10a18, 0x10a18}, {0x10a35, 0x10a3f}, {0x10a49, 0x10a4f},
	{0x10a5a, 0x10a5f}, {0x10a81, 0x10aff}, {0x10b37, 0x10b38},
	{0x10b57, 0x10b57}, {0x10b74, 0x10b77}, {0x10b81, 0x10bff},
	{0x10c4a, 0x10e5f}, {0x10e80, 0x11082}, {0x110b1, 0x110ba},
	{0x110c2, 0x11fff}, {0x12370, 0x123ff}, {0x12464, 0x1246f},
	{0x12475, 0x12fff}, {0x13430, 0x1cfff}, {0x1d0f7, 0x1d0ff},
	{0x1d128, 0x1d128}, {0x1d166, 0x1d169}, {0x1d16e, 0x1d182},
	{0x1d186, 0x1d18b}, {0x1d1ab, 0x1d1ad}, {0x1d1df, 0x1d1ff},
	{0x1d243, 0x1d244}, {0x1d247, 0x1d2ff}, {0x1d358, 0x1d35f},
	{0x1d373, 0x1d3ff}, {0x1d49d, 0x1d49d}, {0x1d4a1, 0x1d4a1},
	{0x1d4a4, 0x1d4a4}, {0x1d4a8, 0x1d4a8}, {0x1d4ba, 0x1d4ba},
	{0x1d4c4, 0x1d4c4}, {0x1d50b, 0x1d50c}, {0x1d51d, 0x1d51d},
	{0x1d53f, 0x1d53f}, {0x1d547, 0x1d549}, {0x1d6a6, 0x1d6a7},
	{0x1d7cd, 0x1d7cd}, {0x1d801, 0x1efff}, {0x1f02d, 0x1f02f},
	{0x1f095, 0x1f0ff}, {0x1f10c, 0x1f10f}, {0x1f130, 0x1f130},
	{0x1f133, 0x1f13c}, {0x1f140, 0x1f141}, {0x1f144, 0x1f145},
	{0x1f148, 0x1f149}, {0x1f150, 0x1f156}, {0x1f159, 0x1f15e},
	{0x1f161, 0x1f178}, {0x1f17d, 0x1f17e}, {0x1f181, 0x1f189},
	{0x1f18f, 0x1f18f}, {0x1f192, 0x1f1ff}, {0x1f202, 0x1f20f},
	{0x1f233, 0x1f23f}, {0x1f24a, 0x1ffff}, {0x2a6d8, 0x2a6ff},
	{0x2b736, 0x2f7ff}, {0x2fa1f, 0x10ffff},
};

static int find(int c, int tab[][2], int n)
{
	int l = 0;
	int h = n - 1;
	int m;
	if (c < tab[0][0])
		return 0;
	while (l <= h) {
		m = (h + l) / 2;
		if (c >= tab[m][0] && c <= tab[m][1])
			return 1;
		if (c < tab[m][0])
			h = m - 1;
		else
			l = m + 1;
	}
	return 0;
}

/* double-width characters */
static int uc_isdw(int c)
{
	return c >= 0x1100 && find(c, dwchars, LEN(dwchars));
}

/* zero-width and combining characters */
static int uc_iszw(int c)
{
	return c >= 0x0300 && find(c, zwchars, LEN(zwchars));
}

int uc_wid(char *s, int cp)
{
	if (uc_iszw(cp))
		return 0;
	return uc_isdw(cp) ? 2 : 1;
}

/* nonprintable characters */
int uc_isbell(char *s, int cp)
{
	int c = (unsigned char) *s;
	if (c == ' ' || c == '\t' || c == '\n' || (c <= 0x7f && isprint(c)))
		return 0;
	return uc_iszw(cp) || find(cp, bchars, LEN(bchars));
}

/* nonprintable characters */
int uc_iscomb(char *s, int cp)
{
	int c = (unsigned char) *s;
	if (c == ' ' || c == '\t' || c == '\n' || (c <= 0x7f && isprint(c)))
		return 0;
	return uc_acomb(cp);
}
