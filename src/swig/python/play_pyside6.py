#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import mlt7 as mlt
import sys

from PySide6.QtWidgets import QApplication, QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QSlider, QStyle
from PySide6.QtCore import Qt, QTimer

VIDEO = 'video.mp4'


class VideoPlayer(QWidget):
    def __init__(self, filename):
        super().__init__()
        self.setWindowTitle("MLT Video Player")

        layout = QVBoxLayout()
        class video_viewer(QWidget):
            """The main window (QWidget) class"""
            def __init__(self):
                super().__init__()
                self.setMinimumSize(380, 260)
                self.blockResize = False
            
            def resizeEvent(self, event):
                if not self.blockResize:
                    self.window().consumer.set('window_width', self.width())
                    self.window().consumer.set('window_height', self.height())
                    event.accept()
                self.blockResize = not self.blockResize
                
        self.video_viewer = video_viewer()
        layout.addWidget(self.video_viewer, 1)

        buttons_layout = QHBoxLayout()

        self.play_button = QPushButton()
        self.play_button.setFlat(True)
        self.play_button.setIcon(self.style().standardIcon(QStyle.SP_MediaPlay))
        self.play_button.clicked.connect(self.play_video)
        buttons_layout.addWidget(self.play_button)
        
        self.pause_button = QPushButton()
        self.pause_button.setFlat(True)
        self.pause_button.setIcon(self.style().standardIcon(QStyle.SP_MediaPause))
        self.pause_button.clicked.connect(self.pause_video)
        buttons_layout.addWidget(self.pause_button)

        self.stop_button = QPushButton()
        self.stop_button.setFlat(True)
        self.stop_button.setIcon(self.style().standardIcon(QStyle.SP_MediaStop))
        self.stop_button.clicked.connect(self.stop_video)
        buttons_layout.addWidget(self.stop_button)

        self.timeline_slider = QSlider(Qt.Horizontal)
        self.timeline_slider.setRange(0, 1000)
        self.timeline_slider.sliderPressed.connect(self.stop_update_slider)
        self.timeline_slider.sliderMoved.connect(self.timeline_slider_changed)
        self.timeline_slider.sliderReleased.connect(self.set_volume_back_to_normal)
        buttons_layout.addWidget(self.timeline_slider)

        layout.addLayout(buttons_layout)
        
        self.setLayout(layout)
        
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_slider)
        
        # Start the mlt system
        mlt.Factory().init()

        # Establish a default (usually "dv_pal") profile
        self.profile = mlt.Profile()

        # Create the producer
        self.producer = mlt.Producer(self.profile, filename)

        if self.producer.is_valid():
            # Derive a profile based on the producer
            self.profile.from_producer(self.producer)
            # Reload the producer using the derived profile
            self.producer = mlt.Producer(self.profile, filename)

        # Create the consumer
        self.consumer = mlt.Consumer(self.profile, "sdl2")
        self.consumer.set('window_id', self.video_viewer.winId())

        if not self.consumer.is_valid():
            print("Failed to open the sdl2 consumer")
            sys.exit(1)

        # Setup a multitrack
        self.playlist = mlt.Playlist(self.profile)
        self.playlist.append(self.producer)

        self.tractor = mlt.Tractor()
        self.tractor.set_track(self.playlist, 0)

        # Connect the producer to the consumer
        self.consumer.connect(self.tractor)
        
        # Start the consumer
        self.consumer.start()
        self.pause_video()

    def update_slider(self):
        self.timeline_slider.setValue(int((self.consumer.position() / self.tractor.get_playtime()) * 1000))
    
    def play_video(self):
        self.tractor.set_speed(1)
        self.consumer.set('volume', 1)
        self.timer.start(1000/24)
        
    def pause_video(self):
        if not self.consumer.is_stopped():
            self.tractor.set_speed(0)
            self.consumer.set('volume', 0)
        
    def stop_video(self):
        self.tractor.seek(0)
        self.tractor.set_speed(0)
        self.timeline_slider.setValue(0)
        self.timer.stop()

    def timeline_slider_changed(self, value):
        self.consumer.set('volume', 0)
        self.tractor.seek(int((value / 1000.0) * self.tractor.get_playtime()))

    def stop_update_slider(self):
        self.timer.stop()

    def set_volume_back_to_normal(self):
        if not self.tractor.get_speed() == 0:
            self.consumer.set('volume', 1)
            self.timer.start(1000/24)

    def closeEvent(self, event):
        self.consumer.stop()
        event.accept()
        
if __name__ == "__main__":
    import sys

    app = QApplication(sys.argv)

    player = VideoPlayer(sys.argv[1] if len(sys.argv) > 1 else VIDEO)
    player.show()
    
    sys.exit(app.exec())