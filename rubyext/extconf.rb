#!/usr/bin/env ruby

require 'mkmf'

module_name = "stridx"
extension_name = 'stridx'

$CXXFLAGS << " -std=c++17 -Wall -Wno-unused-variable -O3" 

have_library( 'stdc++');

dir_config(extension_name)       # The destination
create_makefile(extension_name)  # Create Makefile

