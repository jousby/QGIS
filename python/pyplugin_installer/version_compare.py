"""
/***************************************************************************
                        Plugin Installer module
                        Plugin version comparison functions
                             -------------------
    Date                 : 2008-11-24
    Copyright            : (C) 2008 by Borys Jurgiel
    Email                : info at borysjurgiel dot pl
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

Here is Python function for comparing version numbers. It's case insensitive
and recognizes all major notations, prefixes (ver. and version), delimiters
(. - and _) and suffixes (alpha, beta, rc, preview and trunk).

Usage: compareVersions(version1, version2)

The function accepts arguments of any type convertible to Unicode string
and returns integer value:
0 - the versions are equal
1 - version 1 is higher
2 - version 2 is higher

-----------------------------------------------------------------------------
HOW DOES IT WORK...
First, both arguments are converted to uppercase Unicode and stripped of
'VERSION' or 'VER.' prefix. Then they are chopped into a list of particular
numeric and alphabetic elements. The dots, dashes and underlines are recognized
as delimiters. Also numbers and non numbers are separated. See example below:

'Ver 0.03-120_rc7foo' is converted to ['0','03','120','RC','7','FOO']

Then every pair of elements, from left to right, is compared as string
or as number to provide the best result (you know, 11>9 but also '03'>'007').
The comparing stops when one of elements is greater. If comparing achieves
the end of the shorter list and the matter is still unresolved, the longer
list is usually recognized as higher, except following suffixes:
ALPHA, BETA, RC, PREVIEW and TRUNK which make the version number lower.
"""

from qgis.core import Qgis

import re
from packaging.version import Version, InvalidVersion

# ------------------------------------------------------------------------ #


def normalizeVersion(s):
    """remove possible prefix from given string and convert to uppercase"""

    prefixes = [
        "VERSION",
        "VER.",
        "VER",
        "V.",
        "V",
        "REVISION",
        "REV.",
        "REV",
        "R.",
        "R",
    ]
    if not s:
        return ""
    s = str(s).upper()
    for i in prefixes:
        if s[: len(i)] == i:
            s = s.replace(i, "")
    s = s.strip()
    return s


# ------------------------------------------------------------------------ #
def compareVersions(a, b):
    """Compare two version numbers. Return 0 if a==b or error, 1 if a>b and 2 if b>a"""
    if not a or not b:
        return 0
    a = normalizeVersion(a)
    b = normalizeVersion(b)

    try:
        # PEP 440 comparison
        va, vb = Version(a), Version(b)
        if va < vb:
            return 2
        elif va > vb:
            return 1
        else:
            return 0
    except InvalidVersion:
        # blunt str comparison
        if a < b:
            return 2
        elif a > b:
            return 1
        else:
            return 0


"""
COMPARE CURRENT QGIS VERSION WITH qgisMinimumVersion AND qgisMaximumVersion
ALLOWED FORMATS ARE: major.minor OR major.minor.bugfix, where each segment must be 0..99
"""


def splitVersion(s):
    """split string into 2 or 3 numerical segments"""
    if not s or type(s) is not str:
        return None
    l = str(s).split(".")
    for c in l:
        if not c.isnumeric():
            return None
        if int(c) > 99:
            return None
    if len(l) not in [2, 3]:
        return None
    return l


def isCompatible(curVer, minVer, maxVer):
    """Compare current QGIS version with qgisMinVersion and qgisMaxVersion"""

    if not minVer or not curVer or not maxVer:
        return False

    minVer = splitVersion(re.sub(r"[^0-9.]+", "", minVer))
    maxVer = splitVersion(re.sub(r"[^0-9.]+", "", maxVer))
    curVer = splitVersion(re.sub(r"[^0-9.]+", "", curVer))

    if not minVer or not curVer or not maxVer:
        return False

    if len(minVer) < 3:
        minVer += ["0"]

    if len(curVer) < 3:
        curVer += ["0"]

    if len(maxVer) < 3:
        maxVer += ["99"]

    minVer = f"{int(minVer[0]):04n}{int(minVer[1]):04n}{int(minVer[2]):04n}"
    maxVer = f"{int(maxVer[0]):04n}{int(maxVer[1]):04n}{int(maxVer[2]):04n}"
    curVer = f"{int(curVer[0]):04n}{int(curVer[1]):04n}{int(curVer[2]):04n}"

    return minVer <= curVer and maxVer >= curVer


def pyQgisVersion():
    """Return current QGIS version number as X.Y.Z for testing plugin compatibility.
    If Y = 99, bump up to (X+1.0.0), so e.g. 2.99 becomes 3.0.0
    This way QGIS X.99 is only compatible with plugins for the upcoming major release.
    """
    x, y, z = re.findall(r"^(\d*).(\d*).(\d*)", Qgis.QGIS_VERSION)[0]
    if y == "99":
        x = str(int(x) + 1)
        y = z = "0"
    return f"{x}.{y}.{z}"
