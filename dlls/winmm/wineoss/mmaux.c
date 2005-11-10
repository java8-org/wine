/*
 * Sample AUXILIARY Wine Driver
 *
 * Copyright 1994 Martin Ayotte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fcntl.h>
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "mmddk.h"
#include "oss.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmaux);

#ifdef HAVE_OSS

#define MIXER_DEV "/dev/mixer"

static int	NumDev = 6;

/*-----------------------------------------------------------------------*/

static	int	AUXDRV_Init(void)
{
    int	mixer;

    if ((mixer = open(MIXER_DEV, O_RDWR)) < 0) {
	WARN("mixer device not available !\n");
	NumDev = 0;
    } else {
	close(mixer);
	NumDev = 6;
    }
    return NumDev;
}

/**************************************************************************
 * 				AUX_GetDevCaps			[internal]
 */
static DWORD AUX_GetDevCaps(WORD wDevID, LPAUXCAPSW lpCaps, DWORD dwSize)
{
    int 	mixer, volume;
    static const WCHAR ini[] = {'O','S','S',' ','A','u','x',0};

    TRACE("(%04X, %p, %lu);\n", wDevID, lpCaps, dwSize);
    if (lpCaps == NULL) return MMSYSERR_NOTENABLED;
    if ((mixer = open(MIXER_DEV, O_RDWR)) < 0) {
	WARN("mixer device not available !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (ioctl(mixer, SOUND_MIXER_READ_LINE, &volume) == -1) {
	close(mixer);
	WARN("unable to read mixer !\n");
	return MMSYSERR_NOTENABLED;
    }
    close(mixer);
    lpCaps->wMid = 0xAA;
    lpCaps->wPid = 0x55;
    lpCaps->vDriverVersion = 0x0100;
    strcpyW(lpCaps->szPname, ini);
    lpCaps->wTechnology = AUXCAPS_CDAUDIO;
    lpCaps->dwSupport = AUXCAPS_VOLUME | AUXCAPS_LRVOLUME;

    return MMSYSERR_NOERROR;
}


/**************************************************************************
 * 				AUX_GetVolume			[internal]
 */
static DWORD AUX_GetVolume(WORD wDevID, LPDWORD lpdwVol)
{
    int 	mixer, volume, left, right, cmd;

    TRACE("(%04X, %p);\n", wDevID, lpdwVol);
    if (lpdwVol == NULL) return MMSYSERR_NOTENABLED;
    if ((mixer = open(MIXER_DEV, O_RDWR)) < 0) {
	WARN("mixer device not available !\n");
	return MMSYSERR_NOTENABLED;
    }
    switch(wDevID) {
    case 0:
	TRACE("SOUND_MIXER_READ_PCM !\n");
	cmd = SOUND_MIXER_READ_PCM;
	break;
    case 1:
	TRACE("SOUND_MIXER_READ_SYNTH !\n");
	cmd = SOUND_MIXER_READ_SYNTH;
	break;
    case 2:
	TRACE("SOUND_MIXER_READ_CD !\n");
	cmd = SOUND_MIXER_READ_CD;
	break;
    case 3:
	TRACE("SOUND_MIXER_READ_LINE !\n");
	cmd = SOUND_MIXER_READ_LINE;
	break;
    case 4:
	TRACE("SOUND_MIXER_READ_MIC !\n");
	cmd = SOUND_MIXER_READ_MIC;
	break;
    case 5:
	TRACE("SOUND_MIXER_READ_VOLUME !\n");
	cmd = SOUND_MIXER_READ_VOLUME;
	break;
    default:
	WARN("invalid device id=%04X !\n", wDevID);
	return MMSYSERR_NOTENABLED;
    }
    if (ioctl(mixer, cmd, &volume) == -1) {
	WARN("unable to read mixer !\n");
	return MMSYSERR_NOTENABLED;
    }
    close(mixer);
    left  = LOBYTE(LOWORD(volume));
    right = HIBYTE(LOWORD(volume));
    TRACE("left=%d right=%d !\n", left, right);
    *lpdwVol = MAKELONG((left * 0xFFFFL) / 100, (right * 0xFFFFL) / 100);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				AUX_SetVolume			[internal]
 */
static DWORD AUX_SetVolume(WORD wDevID, DWORD dwParam)
{
    int 	mixer;
    int		volume, left, right;
    int		cmd;

    TRACE("(%04X, %08lX);\n", wDevID, dwParam);

    left   = (LOWORD(dwParam) * 100) >> 16;
    right  = (HIWORD(dwParam) * 100) >> 16;
    volume = (right << 8) | left;

    if ((mixer = open(MIXER_DEV, O_RDWR)) < 0) {
	WARN("mixer device not available !\n");
	return MMSYSERR_NOTENABLED;
    }

    switch(wDevID) {
    case 0:
	TRACE("SOUND_MIXER_WRITE_PCM !\n");
	cmd = SOUND_MIXER_WRITE_PCM;
	break;
    case 1:
	TRACE("SOUND_MIXER_WRITE_SYNTH !\n");
	cmd = SOUND_MIXER_WRITE_SYNTH;
	break;
    case 2:
	TRACE("SOUND_MIXER_WRITE_CD !\n");
	cmd = SOUND_MIXER_WRITE_CD;
	break;
    case 3:
	TRACE("SOUND_MIXER_WRITE_LINE !\n");
	cmd = SOUND_MIXER_WRITE_LINE;
	break;
    case 4:
	TRACE("SOUND_MIXER_WRITE_MIC !\n");
	cmd = SOUND_MIXER_WRITE_MIC;
	break;
    case 5:
	TRACE("SOUND_MIXER_WRITE_VOLUME !\n");
	cmd = SOUND_MIXER_WRITE_VOLUME;
	break;
    default:
	WARN("invalid device id=%04X !\n", wDevID);
	return MMSYSERR_NOTENABLED;
    }
    if (ioctl(mixer, cmd, &volume) == -1) {
	WARN("unable to set mixer !\n");
	return MMSYSERR_NOTENABLED;
    }
    close(mixer);
    return MMSYSERR_NOERROR;
}

#endif

/**************************************************************************
 *		auxMessage (WINEOSS.2)
 */
DWORD WINAPI OSS_auxMessage(UINT wDevID, UINT wMsg, DWORD dwUser,
			    DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%04X, %04X, %08lX, %08lX, %08lX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);

#ifdef HAVE_OSS
    switch (wMsg) {
    case DRVM_INIT:
	AUXDRV_Init();
	/* fall through */
    case DRVM_EXIT:
    case DRVM_ENABLE:
    case DRVM_DISABLE:
	/* FIXME: Pretend this is supported */
	return 0;
    case AUXDM_GETDEVCAPS:
	return AUX_GetDevCaps(wDevID, (LPAUXCAPSW)dwParam1, dwParam2);
    case AUXDM_GETNUMDEVS:
	TRACE("return %d;\n", NumDev);
	return NumDev;
    case AUXDM_GETVOLUME:
	return AUX_GetVolume(wDevID, (LPDWORD)dwParam1);
    case AUXDM_SETVOLUME:
	return AUX_SetVolume(wDevID, dwParam1);
    default:
	WARN("unknown message !\n");
    }
    return MMSYSERR_NOTSUPPORTED;
#else
    return MMSYSERR_NOTENABLED;
#endif
}
