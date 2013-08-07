#!/usr/bin/env ruby
require 'erb'
require 'yaml'
require 'mlt'

$repo = Mlt::Factory::init

$optional_params = [
  'minimum',
  'maximum',
  'default',
  'unit',
  'scale',
  'format',
  'widget'
]
template = %q{%%META:TOPICPARENT{name="Plugins<%= type_title %>s"}%
<noautolink>
---+ <%= type_title %>: <%= yml['identifier'] %> 
%%TOC%
---++ Plugin Information
title: <%= yml['title'] %> %BR%
% if yml['tags']
media types:
%   yml['tags'].each do |x|
<%= x %> 
%   end
%%BR%
% end
description: <%= ERB::Util.h(yml['description']) %> %BR%
version: <%= yml['version'] %> %BR%
creator: <%= yml['creator'] %> %BR%
% yml['contributor'] and yml['contributor'].each do |x|
contributor: <%= x %> %BR%
% end 
<%= "license: #{yml['license']} %BR%\n" if yml['license'] %>
<%= "URL: [[#{yml['url']}]] %BR%\n" if yml['url'] %>

% if yml['notes']
---++ Notes
%   yml['notes'].each do |x|
<%= ERB::Util.h(x) %>
%   end
% end

% if yml['bugs']
---++ Bugs
%   yml['bugs'].each do |x|
   * <%= x %>
%   end
% end

% if yml['parameters']
---++ Parameters
%   yml['parameters'].each do |param|
---+++ <%= param['identifier'] %>
<%= "title: #{param['title']} %BR%\n" if param['title'] %>
<%= "description: #{ERB::Util.h(param['description'])} %BR%\n" if param['description'] %>
type: <%= param['type'] %> %BR%
readonly: <%= param['readonly'] or 'no' %> %BR%
required: <%= param['required'] or 'no' %> %BR%
%     $optional_params.each do |key|
<%= "#{key}: #{param[key]} %BR%\n" if param[key] %>
%     end
%     if param['values']
values:
%       param['values'].each do |value|
   * <%= value %>
%       end
%     end 

%   end
% end
</noautolink>
}

$processor = ERB.new(template, 0, "%<>")


def output(mlt_type, services, type_title)
  index = File.open("Plugins#{type_title}s.txt", 'w')
  index.puts '%META:TOPICPARENT{name="Documentation"}%'
  index.puts '<noautolink>'
  index.puts "---+ #{type_title} Plugins"
  unsorted = []
  (0..(services.count - 1)).each do |i|
    unsorted << services.get_name(i)
  end
  unsorted.sort().each do |name|
    meta = $repo.metadata(mlt_type, name)
    if meta.is_valid
      filename = type_title + name.capitalize.gsub('.', '-')
      puts "Processing #{filename}"
      begin
        yml = YAML.load(meta.serialise_yaml)
        if yml
          File.open(filename + '.txt', 'w') do |f|
            f.puts $processor.result(binding)
          end
        else
          puts "Failed to write file for #{filename}"
        end
        index.puts "   * [[#{filename}][#{name}]]: #{meta.get('title')}\n"
      rescue ArgumentError
          puts "Failed to parse YAML for #{filename}"
      end
    end
  end 
  index.puts '</noautolink>'
  index.close
end

[
  [Mlt::Consumer_type, $repo.consumers, 'Consumer'],
  [Mlt::Filter_type, $repo.filters, 'Filter'],
  [Mlt::Producer_type, $repo.producers, 'Producer'],
  [Mlt::Transition_type, $repo.transitions, 'Transition']
].each {|x| output *x}
