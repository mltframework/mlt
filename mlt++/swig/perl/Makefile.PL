#!/bin/env perl
use ExtUtils::MakeMaker;
system( "ln -sf ../mltpp.i ." );
system( "swig -c++ -I/usr/local/include/mlt++ `mlt-config --cflags` -perl5 mltpp.i" ); 
WriteMakefile(
	'NAME'    => 'mltpp',
	'CC'      => 'g++ `mlt-config --cflags` -I/usr/local/include/mlt++',
	'LIBS'    => ['-lmlt++'],
	'OBJECT'  => 'mltpp_wrap.o'
);

