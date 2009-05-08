#!/bin/env perl
use ExtUtils::MakeMaker;

my $CXX = $ENV{'CXX'} || 'g++';

system( "ln -sf ../mltpp.i ." );
system( "swig -c++ -I../../src `pkg-config mlt-framework --cflags` -perl5 mltpp.i" ); 
WriteMakefile(
	'NAME'    => 'mltpp',
	'CC'      => '${CXX} `pkg-config mlt-framework --cflags` -I../../src',
	'OPTIMIZE' => '-O2 -g -pipe -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -m32 -march=i386',
	'LIBS'    => ['-L../../src -lmlt++'],
	'OBJECT'  => 'mltpp_wrap.o',
	'DESTDIR' => $ENV{'DESTDIR'},
);

