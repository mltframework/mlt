#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Import required modules
import mlt
import time
import sys
import tornado.ioloop
import tornado.web

# Start the mlt system
mlt.mlt_log_set_level(40) # verbose
mlt.Factory.init()

# Establish a pipeline
profile = mlt.Profile("atsc_1080i_5994")
profile.set_explicit(1)
tractor = mlt.Tractor()
tractor.set("eof", "loop")
fg_resource = "decklink:0"
bg_resource = "decklink:1"
if len(sys.argv) > 2:
  fg_resource = sys.argv[1]
  bg_resource = sys.argv[2]
fg = mlt.Producer(profile, fg_resource)
bg = mlt.Producer(profile, bg_resource)
tractor.set_track(bg, 0)
tractor.set_track(fg, 1)
composite = mlt.Transition(profile, "composite")
composite.set("fill", 1)
tractor.plant_transition(composite)

# Setup the consumer
consumer = "decklink:2"
if len(sys.argv) > 3:
  consumer = sys.argv[3]
consumer = mlt.Consumer(profile, consumer)
consumer.connect(tractor)
consumer.set("real_time", -2)
consumer.start()

flip_flop = False
def switch():
  global composite, flip_flop
  frame = fg.frame() + 2
  if flip_flop:
    s = "0=20%%/0:100%%x80%%; %d=20%%/0:100%%x80%%; %d=0/0:100%%x100%%" % (frame, frame + 30)
    composite.set("geometry", s)
  else:
    s = "0=0/0:100%%x100%%; %d=0/0:100%%x100%%; %d=20%%/0:100%%x80%%" % (frame, frame + 30)
    composite.set("geometry", s)
  flip_flop = not flip_flop


def output_form(handler):
  handler.write('<form action="/" method="post"><input type="submit" value="Switch"></form>')

class SwitchHandler(tornado.web.RequestHandler):
  def get(self):
    switch()

class MainHandler(tornado.web.RequestHandler):
  def get(self):
    output_form(self)

  def post(self):
    switch()
    output_form(self)

application = tornado.web.Application([
    (r"/", MainHandler),
    (r"/switch", SwitchHandler)
])

application.listen(8888)
tornado.ioloop.IOLoop.instance().start()
