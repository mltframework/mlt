#!/usr/bin/env ruby
require 'erb'
require 'yaml'
require './mlt'

$folder = 'markdown'
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
template = %q{---
layout: standard
title: Documentation
wrap_title: "<%= type_title %>: <%= yml['identifier'] %>"
category: plugin
---
* TOC
{:toc}

## Plugin Information

title: <%= yml['title'] %>  
% if yml['tags']
media types:
%   yml['tags'].each do |x|
<%= x + "  " %>
%   end
<%= "\n" %>
% end
description: <%= ERB::Util.h(yml['description']) %>  
version: <%= yml['version'] %>  
creator: <%= yml['creator'] %>  
% yml['contributor'] and yml['contributor'].each do |x|
contributor: <%= x %>  
% end 
<%= "copyright: #{yml['copyright']}  \n" if yml['copyright'] %>
<%= "license: #{yml['license']}  \n" if yml['license'] %>
<%= "URL: [#{yml['url']}](#{yml['url']})  \n" if yml['url'] %>
% if yml['notes']

## Notes

<%= ERB::Util.h(yml['notes']) %>

% end
% if yml['bugs']

## Bugs

%   yml['bugs'].each do |x|
* <%= x %>
%   end

% end
% if yml['parameters']

## Parameters

%   yml['parameters'].each do |param|
### <%= param['identifier'] %>

<%= "title: #{param['title']}  " if param['title'] %>  
%     if param['description']
description:
%       if param['description'].include? "\n"
<pre>
<%= param['description'] %>
</pre>
%       else
<%= "#{ERB::Util.h(param['description'])}  \n" %>
%       end
%     end
type: <%= param['type'] %>  
readonly: <%= param['readonly'] or 'no' %>  
required: <%= param['required'] or 'no' %>  
%     $optional_params.each do |key|
<%= "#{key}: #{param[key]}  \n" if param[key] %>
%     end
%     if param['values']
values:  

%       param['values'].each do |value|
* <%= value %>
%       end
%     end

%   end
% end
}

$processor = ERB.new(template, 0, "%<>")


def output(mlt_type, services, type_title)
  filename = File.join($folder, "Plugins#{type_title}s.md")
  index = File.open(filename, 'w')
  index.puts '---'
  index.puts 'title: Documentation'
  index.puts "wrap_title: #{type_title} Plugins"
  index.puts '---'
  unsorted = []
  (0..(services.count - 1)).each do |i|
    unsorted << services.get_name(i)
  end
  unsorted.sort().each do |name|
    meta = $repo.metadata(mlt_type, name)
    if meta.is_valid
      filename = File.join($folder, type_title + name.capitalize.gsub('.', '-'))
      puts "Processing #{filename}"
      begin
        yml = YAML.load(meta.serialise_yaml)
        if yml
		  File.open(filename + '.md', 'w') do |f|
            f.puts $processor.result(binding)
          end
        else
          puts "Failed to write file for #{filename}"
        end
        filename = type_title + name.capitalize.gsub('.', '-')
        index.puts "* [#{name}](../#{filename}/): #{meta.get('title')}\n"
      rescue SyntaxError
          puts "Failed to parse YAML for #{filename}"
      end
    end
  end 
  index.close
end

Dir.mkdir($folder) if not Dir.exists?($folder)

[
  [Mlt::Consumer_type, $repo.consumers, 'Consumer'],
  [Mlt::Filter_type, $repo.filters, 'Filter'],
  [Mlt::Producer_type, $repo.producers, 'Producer'],
  [Mlt::Transition_type, $repo.transitions, 'Transition']
].each {|x| output *x}
