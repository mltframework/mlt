#!/bin/env perl
use ExtUtils::MakeMaker;

my $CXX = $ENV{'CXX'} || 'g++';

system( "ln -sf ../mlt.i" );
system( "swig -c++ -I../../mlt++ -I../.. -perl5 mlt.i" ); 
WriteMakefile(
	'NAME'    => 'mlt',
	'CC'      => '${CXX} -fPIC ${CXXFLAGS} -I../..',
	'OPTIMIZE' => '-O2 -g -pipe -Wp,-D_FORTIFY_SOURCE=2 -fexceptions',
	'LIBS'    => ['-L../../mlt++ -lmlt++'],
	'OBJECT'  => 'mlt_wrap.o',
	'DESTDIR' => $ENV{'DESTDIR'},
);

