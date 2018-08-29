#!/usr/bin/env node

const mlt = require('./node-gyp-build/Release/mlt.node')

const members = Object.keys(mlt)
const maxKeyLength = members.reduce((max, key) => key.length > max ? key.length : max, 0)
const output = members.map(member => [member, mlt[member]])
    .map(([key, val]) => `${padRight(key, maxKeyLength)}\t${isFunction(val) ? '[function]' : val}`)
    .join('\n')
console.log(output)

function padRight(str, len, char = ' ') {
    while (str.length < len) {
        str += char
    }
    return str;
}

function isFunction(val) {
    return typeof val === 'function'
}

function entries(obj) {
    return
}
