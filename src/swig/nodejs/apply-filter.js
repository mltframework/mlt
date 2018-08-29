#!/usr/bin/env node

const path = require('path')

const mlt = require('./node-gyp-build/Release/mlt.node')

const {
    Factory,
    Profile,
    Producer,
    Filter,
    Consumer
} = mlt;

Factory.init('')

let profile = new Profile('hdv_720_25p')
let producer = new Producer(profile, 'avformat', process.argv[2])

if (!producer.is_valid()) {
    console.log(`Unable to open producer`)
    process.exit(1)
}

producer.set_in_and_out(100, 200)

let videoOutputFilePath = path.join(__dirname, 'output.mp4')
console.log('output:', videoOutputFilePath)

let consumer = new Consumer(profile, 'avformat', videoOutputFilePath)
consumer.set('rescale', 'none')
consumer.set('vcodec', 'libx264')
consumer.set('acodec', 'aac')

let filter = new Filter(profile, 'charcoal')
producer.attach(filter)

consumer.connect(producer)
consumer.start()

check()

function check() {
    process.stdout.write('.')
    if (consumer.is_stopped()) {
        consumer.stop()
        Factory.close()
        return
    }
    setTimeout(check, 1000)
}


