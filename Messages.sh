#!/bin/sh
# SPDX-FileCopyrightText: 2026 Tobias Kaminski
# SPDX-License-Identifier: MIT
#
# Extracts the translatable strings into hidsight.pot, as KDE's scripty
# runs it: $XGETTEXT and $podir come from the caller. The desktop file and
# the metainfo are extracted by scripty on its own.

$XGETTEXT $(find src qml -name '*.cpp' -o -name '*.h' -o -name '*.qml' | sort) -o "$podir"/hidsight.pot
