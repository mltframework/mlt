#!/bin/sh
java -Djava.library.path=. -cp .:src_swig Play "$@"
