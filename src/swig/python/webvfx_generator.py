#!/usr/bin/env python
# -*- coding: utf-8 -*-

# webvfx_generator.py
# Copyright (C) 2013 Dan Dennedy <dan@dennedy.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

# Import required modules
import mlt
import time
import sys
import tornado.ioloop
import tornado.web
import shutil
import tempfile
import os
import os.path

# Start the mlt system
mlt.mlt_log_set_level(40) # verbose
mlt.Factory.init()

# Establish a pipeline
profile = mlt.Profile("atsc_1080i_5994")
#profile = mlt.Profile('square_ntsc_wide')
profile.set_explicit(1)
tractor = mlt.Tractor()
tractor.set('eof', 'loop')
playlist = mlt.Playlist()
playlist.append(mlt.Producer(profile, 'color:'))

# Setup the consumer
consumer = 'decklink:0'
if len(sys.argv) > 1:
  consumer = sys.argv[1]
consumer = mlt.Consumer(profile, consumer)
consumer.connect(playlist)
#consumer.set("real_time", -2)
consumer.start()

def switch(resource):
  global playlist
  resource = resource
  playlist.lock()
  playlist.append(mlt.Producer(profile, str(resource)))
  playlist.remove(0)
  playlist.unlock()

state = {}
state['tempdir'] = None

class MainHandler(tornado.web.RequestHandler):
  def get(self):
    resource = self.get_argument('url', None)
    if resource:
      global state

      self.write('Playing %s\n' % (resource))
      switch(resource)

      olddir = state['tempdir']
      if olddir:
        shutil.rmtree(olddir, True)
      state['tempdir'] = None

    else:
      self.write('''
<p>POST a bunch of files to / to change the output.</p>
<p>Or GET / with query string parameter "url" to display something from the network.</p>
''')

  def post(self):
    if len(self.request.files) == 0:
      self.write('POST a bunch of files to / to change the output')
    else:
      global state
      olddir = state['tempdir']
      resource = None
      state['tempdir'] = tempfile.mkdtemp()
      for key, items in self.request.files.iteritems():
        for item in items:
          path = os.path.dirname(key)
          fn = item.filename
          if path: 
            if not os.path.exists(os.path.join(state['tempdir'], path)):
              os.makedirs(os.path.join(state['tempdir'], path))
            fn = os.path.join(path, fn)
          fn = os.path.join(state['tempdir'], fn)
          if not path and fn.endswith('.html') or fn.endswith('.qml'):
            resource = fn
          with open(fn, 'w') as fo:
            fo.write(item.body)
          self.write("Uploaded %s\n" % (fn))
      if resource:
        self.write('Playing %s\n' % (resource))
        switch(resource)
      if olddir:
        shutil.rmtree(olddir, True)

application = tornado.web.Application([
    (r"/", MainHandler),
])

application.listen(8888)
try:
  tornado.ioloop.IOLoop.instance().start()
except:
  pass

consumer.stop()
if state['tempdir']:
  shutil.rmtree(state['tempdir'], True)
