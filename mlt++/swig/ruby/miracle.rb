require 'mltpp'

def command 
	puts "command"
end

def push 
	puts "push"
end

miracle = Mltpp::Miracle.new( "miracle-ruby", 5260 )
miracle.start
miracle.execute( "uadd sdl" )
listener = Mltpp::Listener.new( miracle, "command-received", method( :command ) )
listener = Mltpp::Listener.new( miracle, "push-received", method( :push ) )
miracle.wait_for_shutdown

