#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import gi
import sys
import mlt7 as mlt

gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, Gdk, GObject, GLib

VIDEO = 'video.mp4'

class VideoPlayer(Gtk.Window):
    def __init__(self, filename):
        super().__init__(title="MLT Video Player")
        self.set_default_size(800, 600)
        self.connect("destroy", self.on_destroy)

        # Initialize the mlt system
        mlt.Factory().init()

        # Create a default profile
        self.profile = mlt.Profile()

        # Create a producer for the video
        self.producer = mlt.Producer(self.profile, filename)
        if self.producer.is_valid():
            self.profile.from_producer(self.producer)
            self.producer = mlt.Producer(self.profile, filename)

        # Create the consumer for rendering using SDL2
        self.consumer = mlt.Consumer(self.profile, "sdl2")

        # GTK Layout
        self.layout = Gtk.VBox(spacing=6)
        self.add(self.layout)

        # Create the drawing area for the video
        self.drawing_area = Gtk.DrawingArea()
        self.drawing_area.set_size_request(800, 450)
        self.drawing_area.connect("size-allocate",self.drawing_area_resized)
        self.layout.pack_start(self.drawing_area, True, True, 0)
        self.drawing_area.realize()
        print(int(self.drawing_area.get_window().get_xid()))
        self.consumer.set('window_id', str(self.drawing_area.get_window().get_xid()))

        if not self.consumer.is_valid():
            print("Failed to open the sdl2 consumer")
            sys.exit(1)

        # Create a multitrack playlist and set it to the tractor
        self.playlist = mlt.Playlist(self.profile)
        self.playlist.append(self.producer)

        self.tractor = mlt.Tractor()
        self.tractor.set_track(self.playlist, 0)

        # Connect the producer to the consumer
        self.consumer.connect(self.tractor)

        # Control Buttons
        controls_box = Gtk.HBox(spacing=6)
        self.layout.pack_start(controls_box, False, False, 0)

        # Play button
        self.play_button = Gtk.Button.new_with_label("Play")
        self.play_button.connect("clicked", self.play_video)
        controls_box.pack_start(self.play_button, False, False, 0)

        # Pause button
        self.pause_button = Gtk.Button.new_with_label("Pause")
        self.pause_button.connect("clicked", self.pause_video)
        controls_box.pack_start(self.pause_button, False, False, 0)

        # Stop button
        self.stop_button = Gtk.Button.new_with_label("Stop")
        self.stop_button.connect("clicked", self.stop_video)
        controls_box.pack_start(self.stop_button, False, False, 0)

        # Slider
        self.timeline_slider = Gtk.Scale.new_with_range(Gtk.Orientation.HORIZONTAL, 0, 1000, 1)
        self.timeline_slider.set_draw_value(False)
        self.slider_handler_id = self.timeline_slider.connect("value-changed", self.on_slider_changed)
        self.timeline_slider.connect("button-release-event", self.on_slider_button_release)
        controls_box.pack_start(self.timeline_slider, True, True, 0)

        # Timer to update the slider
        self.update_timer = GLib.timeout_add(1000 // 24, self.update_slider)

        # Start the consumer
        self.consumer.start()
        self.pause_video()

    def drawing_area_resized(self, widget, _):
        self.consumer.set('window_width', widget.get_allocated_width())
        self.consumer.set('window_height', widget.get_allocated_height())

    def update_slider(self):
        position = self.consumer.position()
        playtime = self.tractor.get_playtime()
        if playtime > 0:
            value = int((position / playtime) * 1000)
            self.timeline_slider.handler_block(self.slider_handler_id)
            self.timeline_slider.set_value(value)
            self.timeline_slider.handler_unblock(self.slider_handler_id)
        return True

    def play_video(self, button):
        self.tractor.set_speed(1)
        self.consumer.set('volume', 1)

    def pause_video(self, button=None):
        if not self.consumer.is_stopped():
            self.tractor.set_speed(0)
            self.consumer.set('volume', 0)

    def stop_video(self, button):
        self.tractor.seek(0)
        self.tractor.set_speed(0)
        self.timeline_slider.set_value(0)

    def on_slider_changed(self, slider):
        value = slider.get_value()
        self.consumer.set('volume', 0)
        playtime = self.tractor.get_playtime()
        self.tractor.seek(int((value / 1000.0) * playtime))

    def on_slider_button_release(self, slider, _):
        if not self.tractor.get_speed() == 0:
            self.consumer.set('volume', 1)

    def on_destroy(self, widget):
        self.consumer.stop()
        mlt.Factory().close()
        Gtk.main_quit()

if __name__ == "__main__":
    # Initialize GTK
    GObject.threads_init()
    Gtk.init(sys.argv)

    # Create and run the video player
    player = VideoPlayer(sys.argv[1] if len(sys.argv) > 1 else VIDEO)
    player.show_all()
    Gtk.main()
