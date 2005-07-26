#!/bin/env perl
use ExtUtils::MakeMaker;
system( "ln -sf ../mltpp.i ." );
system( "swig -c++ -I../../src `mlt-config --cflags` -perl5 mltpp.i" ); 
WriteMakefile(
	'NAME'    => 'mltpp',
	'CC'      => 'g++ `mlt-config --cflags` -I../../src',
	'LIBS'    => ['-L../../src -lmlt++'],
	'OBJECT'  => 'mltpp_wrap.o'
);

