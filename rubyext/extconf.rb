#!/usr/bin/env ruby
#

require 'mkmf'

module_name = "stridx"
extension_name = 'stridx'

$CXXFLAGS << " -Wall -Wno-unused-variable -O3 -fopenmp" 

have_library( 'stdc++');
have_library( 'gomp' );

dir_config(extension_name)       # The destination
create_makefile(extension_name)  # Create Makefile

