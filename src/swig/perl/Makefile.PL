#!/bin/env perl
use ExtUtils::MakeMaker;

my $CXX = $ENV{'CXX'} || 'g++';

system( "ln -sf ../mltpp.i ." );
system( "swig -c++ -I../../mlt++ `pkg-config mlt-framework --cflags` -perl5 mltpp.i" ); 
WriteMakefile(
	'NAME'    => 'mlt',
	'CC'      => '${CXX} `pkg-config mlt-framework --cflags` -I../../mlt++',
	'OPTIMIZE' => '-O2 -g -pipe -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -m32 -march=i386',
	'LIBS'    => ['-L../../mlt++ -lmlt++'],
	'OBJECT'  => 'mlt_wrap.o',
	'DESTDIR' => $ENV{'DESTDIR'},
);

