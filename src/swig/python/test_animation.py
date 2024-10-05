#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import print_function
import mlt7 as mlt

p = mlt.Properties()
p.anim_set("foo", "bar", 10)
a = p.get_anim("bar")
print("bar is valid:", a.is_valid())
a = p.get_anim("foo")
print("foo is valid:", a.is_valid())
print("serialize:", a.serialize_cut())
