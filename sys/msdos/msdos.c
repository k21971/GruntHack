/*	SCCS Id: @(#)msdos.c	 3.1	 93/05/06		  */
/* Copyright (c) NetHack PC Development Team 1990, 1991, 1992	  */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *  MSDOS system functions.
 *  Many thanks to Don Kneller who originated the DOS port and
 *  contributed much to the cause.
 */

#define NEED_VARARGS
#include "hack.h"

#ifdef MSDOS

#include <dos.h>
#include <ctype.h>

/*
 * MS-DOS functions
 */
#define DIRECT_INPUT    0x07    /* Unfiltered Character Input Without Echo */
#define FATINFO     0x1B    /* Get Default Drive Data */
/* MS-DOS 2.0+: */
#define GETDTA      0x2F    /* Get DTA Address */
#define FREESPACE   0x36    /* Get Drive Allocation Info */
#define GETSWITCHAR 0x3700  /* Get Switch Character */
#define FINDFIRST   0x4E    /* Find First File */
#define FINDNEXT    0x4F    /* Find Next File */
#define SETFILETIME 0x5701  /* Set File Date & Time */
/*
 * BIOS interrupts
 */
#ifdef PC9801
#define KEYBRD_BIOS 0x18
#else
#define KEYBRD_BIOS 0x16
#endif

/*
 * Keyboard BIOS functions
 */
#define READCHAR    0x00    /* Read Character from Keyboard */
#define GETKEYFLAGS 0x02    /* Get Keyboard Flags */

void FDECL(get_cursor,(int *, int *));

#ifdef OVL0

/* direct bios calls are used only when flags.BIOS is set */

static char NDECL(DOSgetch);
static char NDECL(BIOSgetch);
static char * NDECL(getdta);
static unsigned int FDECL(dos_ioctl, (int,int,unsigned));

int
tgetch()
{
	char ch;

	/* BIOSgetch can use the numeric key pad on IBM compatibles. */
	if (flags.BIOS)
		ch = BIOSgetch();
	else
		ch = DOSgetch();
	return ((ch == '\r') ? '\n' : ch);
}



/*
 *  Keyboard translation tables.
 */
#ifdef PC9801
#define KEYPADLO	0x38
#define KEYPADHI	0x50
#else
#define KEYPADLO	0x47
#define KEYPADHI	0x53
#endif

#define PADKEYS 	(KEYPADHI - KEYPADLO + 1)
#define iskeypad(x)	(KEYPADLO <= (x) && (x) <= KEYPADHI)

/*
 * Keypad keys are translated to the normal values below.
 * When flags.BIOS is active, shifted keypad keys are translated to the
 *    shift values below.
 */
static const struct pad {
	char normal, shift, cntrl;
} keypad[PADKEYS] = {
#ifdef PC9801
			{'>', '>', '>'},		/* Ins */
			{'<', '<', '<'},		/* Del */
			{'k', 'K', C('k')},		/* Up */
			{'h', 'H', C('h')},		/* Left */
			{'l', 'L', C('l')},		/* Right */
			{'j', 'J', C('j')},		/* Down */
			{ 0 ,  0 ,  0 },		/* HomeClr */
			{'?', '?', '?' },		/* Help */
			{'m', C('p'), C('p')},		/* - */
			{'/', '/', '/'},		/* / */
			{'y', 'Y', C('y')},		/* 7 */
			{'k', 'K', C('k')},		/* 8 */
			{'u', 'U', C('u')},		/* 9 */
			{'*', '*', '*'},		/* * */
			{'h', 'H', C('h')},		/* 4 */
			{'g', 'g', 'g'},		/* 5 */
			{'l', 'L', C('l')},		/* 6 */
			{'p', 'P', C('p')},		/* + */
			{'b', 'B', C('b')},		/* 1 */
			{'j', 'J', C('j')},		/* 2 */
			{'n', 'N', C('n')},		/* 3 */
			{'=', '=', '='},		/* = */
			{'i', 'I', C('i')},		/* 0 */
			{',' ':', ':'}, 		/* , */
			{'.', '.', '.'} 		/* . */
#else
			{'y', 'Y', C('y')},		/* 7 */
			{'k', 'K', C('k')},		/* 8 */
			{'u', 'U', C('u')},		/* 9 */
			{'m', C('p'), C('p')},		/* - */
			{'h', 'H', C('h')},		/* 4 */
			{'g', 'g', 'g'},		/* 5 */
			{'l', 'L', C('l')},		/* 6 */
			{'p', 'P', C('p')},		/* + */
			{'b', 'B', C('b')},		/* 1 */
			{'j', 'J', C('j')},		/* 2 */
			{'n', 'N', C('n')},		/* 3 */
			{'i', 'I', C('i')},		/* Ins */
			{'.', ':', ':'}			/* Del */
#endif
}, numpad[PADKEYS] = {
#ifdef PC9801
			{'>', '>', '>'},		/* Ins */
			{'<', '<', '<'},		/* Del */
			{'8', M('8'), '8'},		/* Up */
			{'4', M('4'), '4'},		/* Left */
			{'6', M('6'), '6'},		/* Right */
			{'2', M('2'), '2'},		/* Down */
			{ 0 ,  0 ,  0 },		/* HomeClr */
			{'?', '?', '?'},		/* Help */
			{'m', C('p'), C('p')},		/* - */
			{'/', '/', '/'},		/* / */
			{'7', M('7'), '7'},		/* 7 */
			{'8', M('8'), '8'},		/* 8 */
			{'9', M('9'), '9'},		/* 9 */
			{'*', '*', '*'},		/* * */
			{'4', M('4'), '4'},		/* 4 */
			{'g', 'G', 'g'},		/* 5 */
			{'6', M('6'), '6'},		/* 6 */
			{'p', 'P', C('p')},		/* + */
			{'1', M('1'), '1'},		/* 1 */
			{'2', M('2'), '2'},		/* 2 */
			{'3', M('3'), '3'},		/* 3 */
			{'=', '=', '='},		/* = */
			{'i', 'I', C('i')},		/* 0 */
			{',', ':', ':'},		/* , */
			{'.', '.', '.'} 		/* . */
#else
			{'7', M('7'), '7'},		/* 7 */
			{'8', M('8'), '8'},		/* 8 */
			{'9', M('9'), '9'},		/* 9 */
			{'m', C('p'), C('p')},		/* - */
			{'4', M('4'), '4'},		/* 4 */
			{'g', 'G', 'g'},		/* 5 */
			{'6', M('6'), '6'},		/* 6 */
			{'p', 'P', C('p')},		/* + */
			{'1', M('1'), '1'},		/* 1 */
			{'2', M('2'), '2'},		/* 2 */
			{'3', M('3'), '3'},		/* 3 */
			{'i', 'I', C('i')},		/* Ins */
			{'.', ':', ':'}			/* Del */
#endif
};

/*
 * Unlike Ctrl-letter, the Alt-letter keystrokes have no specific ASCII
 * meaning unless assigned one by a keyboard conversion table, so the
 * keyboard BIOS normally does not return a character code when Alt-letter
 * is pressed.	So, to interpret unassigned Alt-letters, we must use a
 * scan code table to translate the scan code into a letter, then set the
 * "meta" bit for it.  -3.
 */
#ifdef PC9801
#define SCANLO		0x5
#else
#define SCANLO		0x10
#endif /* PC9801 */

static const char scanmap[] = { 	/* ... */
#ifdef PC9801
			 0,  0,  0,  0,  0,  0, '-','^','\\','\b',
	'\t','q','w','e','r','t','y','u','i','o','p','@','[', '\n',
	'a','s','d','f','g','h','j','k','l',';',':', ']',
	'z','x','c','v','b','N','m',',','.','/'	/* ... */
#else
	'q','w','e','r','t','y','u','i','o','p','[',']', '\n',
	0, 'a','s','d','f','g','h','j','k','l',';','\'', '`',
	0, '\\', 'z','x','c','v','b','n','m',',','.','?'	/* ... */
#endif /* PC9801 */
};

#define inmap(x)	(SCANLO <= (x) && (x) < SCANLO + SIZE(scanmap))

/*
 * BIOSgetch gets keys directly with a BIOS call.
 */
#ifdef PC9801
#define SHIFT		0x1
#define KANA		0x4
#define GRPH		0x8
#define CTRL		0x10
#else
#define SHIFT		(0x1 | 0x2)
#define CTRL		0x4
#define ALT		0x8
#endif /* PC9801 */

static char
BIOSgetch()
{
	unsigned char scan, shift, ch;
	const struct pad *kpad;

	union REGS regs;

	/* Get scan code.
	 */
	regs.h.ah = READCHAR;
	int86(KEYBRD_BIOS, &regs, &regs);
	ch = regs.h.al;
	scan = regs.h.ah;
	/* Get shift status.
	 */
	regs.h.ah = GETKEYFLAGS;
	int86(KEYBRD_BIOS, &regs, &regs);
	shift = regs.h.al;

	/* Translate keypad keys */
	if (iskeypad(scan)) {
		kpad = flags.num_pad ? numpad : keypad;
		if (shift & SHIFT)
			ch = kpad[scan - KEYPADLO].shift;
		else if (shift & CTRL)
			ch = kpad[scan - KEYPADLO].cntrl;
		else
			ch = kpad[scan - KEYPADLO].normal;
	}
	/* Translate unassigned Alt-letters */
#ifdef PC9801
	if (shift & KANA)
		return 0;
	if ((shift & GRPH) && (ch >= 0x80)) {
#else
	if ((shift & ALT) && !ch) {
#endif
		if (inmap(scan))
			ch = scanmap[scan - SCANLO];
		return (isprint(ch) ? M(ch) : ch);
	}
	return ch;
}

static char
DOSgetch()
{
	union REGS regs;
	char ch;
	struct pad (*kpad)[PADKEYS];

	regs.h.ah = DIRECT_INPUT;
	intdos(&regs, &regs);
	ch = regs.h.al;

#ifdef PC9801
	if (ch < 0)	/* KANA letters and GRPH-shifted letters(?) */
		ch = 0; /* munch it */
#else
	/*
	 * The extended codes for Alt-shifted letters, and unshifted keypad
	 * and function keys, correspond to the scan codes.  So we can still
	 * translate the unshifted cursor keys and Alt-letters.  -3.
	 */
	if (ch == 0) {		/* an extended key */
		regs.h.ah = DIRECT_INPUT;
		intdos(&regs, &regs);	/* get the extended key code */
		ch = regs.h.al;

		if (iskeypad(ch)) {	/* unshifted keypad keys */
			kpad = (void *)(flags.num_pad ? numpad : keypad);
			ch = (*kpad)[ch - KEYPADLO].normal;
		} else if (inmap(ch)) { /* Alt-letters */
			ch = scanmap[ch - SCANLO];
			if (isprint(ch)) ch = M(ch);
		} else ch = 0;		/* munch it */
	}
#endif
	return (ch);
}

char
switchar()
{
	union REGS regs;

	regs.x.ax = GETSWITCHAR;
	intdos(&regs, &regs);
	return regs.h.dl;
}

long
freediskspace(path)
char *path;
{
	union REGS regs;

	regs.h.ah = FREESPACE;
	if (path[0] && path[1] == ':')
		regs.h.dl = (toupper(path[0]) - 'A') + 1;
	else
		regs.h.dl = 0;
	intdos(&regs, &regs);
	if (regs.x.ax == 0xFFFF)
		return -1L;		/* bad drive number */
	else
		return ((long) regs.x.bx * regs.x.cx * regs.x.ax);
}

#ifndef __GO32__
/*
 * Functions to get filenames using wildcards
 */
int
findfirst(path)
char *path;
{
	union REGS regs;
	struct SREGS sregs;

	regs.h.ah = FINDFIRST;
	regs.x.cx = 0;		/* attribute: normal files */
	regs.x.dx = FP_OFF(path);
	sregs.ds = FP_SEG(path);
	intdosx(&regs, &regs, &sregs);
	return !regs.x.cflag;
}

int
findnext() {
	union REGS regs;

	regs.h.ah = FINDNEXT;
	intdos(&regs, &regs);
	return !regs.x.cflag;
}

char *
foundfile_buffer()
{
	return (getdta() + 30);
}


/* Get disk transfer area */
static char *
getdta()
{
	union REGS regs;
	struct SREGS sregs;
	char *ret;

	regs.h.ah = GETDTA;
	intdosx(&regs, &regs, &sregs);
#   ifdef MK_FP
	ret = MK_FP(sregs.es, regs.x.bx);
#   else
	FP_OFF(ret) = regs.x.bx;
	FP_SEG(ret) = sregs.es;
#   endif
	return ret;
}

long
filesize(file)
char *file;
{
	char *dta;

	if (findfirst(file)) {
		dta = getdta();
		return  (* (long *) (dta + 26));
	} else
		return -1L;
}

#endif /* __GO32__ */

/*
 * Chdrive() changes the default drive.
 */
void
chdrive(str)
char *str;
{
#  define SELECTDISK	0x0E
	char *ptr;
	union REGS inregs;
	char drive;

	if ((ptr = index(str, ':')) != NULL) {
		drive = toupper(*(ptr - 1));
		inregs.h.ah = SELECTDISK;
		inregs.h.dl = drive - 'A';
		intdos(&inregs, &inregs);
	}
	return;
}


/* Use the IOCTL DOS function call to change stdin and stdout to raw
 * mode.  For stdin, this prevents MSDOS from trapping ^P, thus
 * freeing us of ^P toggling 'echo to printer'.
 * Thanks to Mark Zbikowski (markz@microsoft.UUCP).
 */

#define DEVICE		0x80
#define RAW		0x20
#define IOCTL		0x44
#define STDIN		fileno(stdin)
#define STDOUT		fileno(stdout)
#define GETBITS		0
#define SETBITS		1

static unsigned	int old_stdin, old_stdout;

void
disable_ctrlP()
{

	if (!flags.rawio) return;

    old_stdin = dos_ioctl(STDIN, GETBITS, 0);
    old_stdout = dos_ioctl(STDOUT, GETBITS, 0);
	if (old_stdin & DEVICE)
        dos_ioctl(STDIN, SETBITS, old_stdin | RAW);
	if (old_stdout & DEVICE)
        dos_ioctl(STDOUT, SETBITS, old_stdout | RAW);
	return;
}

void
enable_ctrlP()
{
	if (!flags.rawio) return;
	if (old_stdin)
        (void) dos_ioctl(STDIN, SETBITS, old_stdin);
	if (old_stdout)
        (void) dos_ioctl(STDOUT, SETBITS, old_stdout);
	return;
}

static unsigned int
dos_ioctl(handle, mode, setvalue)
int handle, mode;
unsigned setvalue;
{
	union REGS regs;

	regs.h.ah = IOCTL;
	regs.h.al = mode;
	regs.x.bx = handle;
	regs.h.dl = setvalue;
	regs.h.dh = 0;			/* Zero out dh */
	intdos(&regs, &regs);
	return (regs.x.dx);
}

# endif /* OVLB */

#endif /* MSDOS */