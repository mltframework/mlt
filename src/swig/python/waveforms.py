#!/usr/bin/env python
# -*- coding: utf-8 -*-
import mlt
from PIL import Image

mlt.Factory.init()
profile = mlt.Profile()
prod = mlt.Producer(profile, 'test.wav')
size = (320, 240)
for i in range(0, prod.get_length()):
  frm = prod.get_frame()
  wav = frm.get_waveform(size[0], size[1])
  img = Image.fromstring('L', size, wav)
  img.save('test-%04d.pgm' % (i))
