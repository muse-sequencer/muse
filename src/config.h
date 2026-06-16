//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2011 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef _MUSE_CONFIG_H_
#define _MUSE_CONFIG_H_

/* #undef POSIX_TIMER_SUPPORT */
#define ALSA_SUPPORT
#define HAVE_RTAUDIO
/* #undef DSSI_ALSA_COMPAT_SUPPORT */
/* #undef HAVE_LASH */
#define HAVE_LRDF
#define OSC_SUPPORT
#define DSSI_SUPPORT
#define LV2_SUPPORT
/* #undef LV2_MAKE_PATH_SUPPORT */
/* #undef LV2_USE_PLUGIN_CACHE */
#define PYTHON_SUPPORT
#define MIDNAM_SUPPORT
#define HAVE_INSTPATCH
/* #undef HAVE_GTK2 */
/* #undef VST_SUPPORT */
#define VST_NATIVE_SUPPORT
#define VST_VESTIGE_SUPPORT
/* #undef USE_SSE */
#define RUBBERBAND_SUPPORT
/* #undef ZITA_RESAMPLER_SUPPORT */
#define HAVE_EXP10
#define HAVE_EXP10F
#define HAVE_EXP10L
#define ALLOW_LEFT_HIDDEN_EVENTS
/* #undef HAVE_ISTRINGSTREAM_HEXFLOAT */
/* #undef HAVE_M_PI */

#define VERSION             "4.2.1"
#define GITSTRING           "master | 4.2.1-99-gdaad0ee1-dirty | 2026-03-15 17:51:43 -0400"
#define ORGANIZATION_NAME   "MusE"
#define ORGANIZATION_DOMAIN "muse-sequencer.github.io"
#define ORGANIZATION_URL    "https://muse-sequencer.github.io/"
#define ORGANIZATION_HELP_URL "https://muse-sequencer.github.io/docs/"
#define ORGANIZATION_CODE_REPO_URL "https://github.com/muse-sequencer/muse/"
#define PACKAGE_NAME        "MusE"
#define APP_DISPLAY_NAME    "MusE"
#define APP_DESCRIPTION     "MusE: Linux Music Editor"
#define DOCDIR              "/usr/local/share/doc/muse-4.2"
#define SHAREDIR            "/usr/local/share/muse-4.2"
#define LIBDIR              "/usr/local/lib/muse-4.2"
#define BINDIR              "/usr/local/bin"
#define VST_SDK_QUIRK_DEF   ""
#ifdef _WIN32 // in liblo windows code is in WIN32 definition
  #ifndef WIN32
  #define WIN32
  #endif
#endif
#endif // _MUSE_CONFIG_H_
