#!/usr/bin/env node

const path = require('path')

const mlt = require('./node-gyp-build/Release/mlt.node')

const {
    Factory,
    Profile,
    Producer,
    Consumer
} = mlt;

const videoFile = process.argv[2]
console.log(videoFile)

Factory.init('')

let profile = new Profile('hdv_720_25p')
let producer = new Producer(profile, 'avformat', videoFile)


if (!producer.is_valid()) {
    console.log(`Unable to open producer`)
    process.exit(1)
}

console.log('get_fps', producer.get_fps())

let consumer = new Consumer(profile, 'sdl')

consumer.connect(producer)
consumer.start()

check()

function check() {
    console.log('is_stopped', consumer.is_stopped())
    if (consumer.is_stopped()) {
        consumer.stop()
        Factory.close()
        return
    }
    setTimeout(check, 1000)
}


