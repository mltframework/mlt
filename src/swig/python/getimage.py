#!/usr/bin/env python
# -*- coding: utf-8 -*-
import mlt
import sys
from PIL import Image

# setup
mlt.Factory.init()
profile = mlt.Profile('square_pal_wide')
prod = mlt.Producer(profile, sys.argv[1])

# This builds a profile from the attributes of the producer: auto-profile.
profile.from_producer(prod)

# Ensure the image is square pixels - optional.
profile.set_width(int(profile.width() * profile.sar()))
profile.set_sample_aspect(1, 0)

# Seek to 10% and get a Mlt frame.
prod.seek(int(prod.get_length() * 0.1))
frame = prod.get_frame()

# And make sure we deinterlace if input is interlaced - optional.
frame.set("consumer_deinterlace", 1)

# Now we are ready to get the image and save it.
size = (profile.width(), profile.height())
rgb = frame.get_image(mlt.mlt_image_rgb24, *size)
img = Image.fromstring('RGB', size, rgb)
img.save(sys.argv[1] + '.png')
