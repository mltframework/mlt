#!/usr/bin/env ruby
require 'mlt'

$repo = Mlt::Factory::init

def output(mlt_type, services, type_title)
  index = File.open("Plugins#{type_title}s.txt", 'w')
  index.puts '%META:TOPICPARENT{name="Documentation"}%'
  index.puts "---+ #{type_title} Plugins"
  (0..(services.count - 1)).each {|i|
    name = services.get_name(i)
    meta = $repo.metadata(mlt_type, name)
    if meta.is_valid
      filename = type_title + name.capitalize.gsub('.', '-')
      File.open(filename + '.txt', 'w') do |f|
        f.puts %Q/%META:TOPICPARENT{name="Plugins#{type_title}s"}%/
        f.puts "---+ #{type_title} Plugin"
        f.puts "---++ #{name}"
        f.puts '<verbatim>'
        f.puts meta.serialise_yaml
        f.puts '</verbatim>'
        puts "Wrote file #{filename}.txt"
        index.puts "   * [[#{filename}][#{name}]]: #{meta.get('title')}\n"
      end
    end
  }
  index.close
end

[
  [Mlt::Consumer_type, $repo.consumers, 'Consumer'],
  [Mlt::Filter_type, $repo.filters, 'Filter'],
  [Mlt::Producer_type, $repo.producers, 'Producer'],
  [Mlt::Transition_type, $repo.transitions, 'Transition']
].each {|x| output *x}
