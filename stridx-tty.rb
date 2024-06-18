#!/usr/bin/env ruby

require "tty-prompt"
require "tty-cursor"
require "tty-reader"
require "pastel"

require "socket"

class StrIdxTTY
  def self.run
    stty = StrIdxTTY.new
    selected = stty.search
    STDOUT.write selected
  end

  def initialize()
    @lines = []
    @selected = ""
    @idx = 0

    @reader = TTY::Reader.new(output: STDERR)
    @pastel = Pastel.new()
    @cursor = TTY::Cursor
  end

  def out(x)
    STDERR.write x
  end

  def search
    out "\n" * 20
    out @cursor.clear_screen
    out "\n" * 20
    @cursor.move_to(0, 0)
    @reader.on(:keypress) { |event|
      handle_event(event)
    }
    @reader.read_line(">> ")

    out @cursor.clear_screen
    return @selected.strip
  end

  def get_res_from_server(query)
    # Define the socket file path
    sock_dir = File.expand_path("~/.stridx")
    sockfn = "#{sock_dir}/sock"

    # Create a new UNIXSocket
    client = UNIXSocket.new(sockfn)

    # Send data to the server
    client.puts "find:#{query}"

    # Read response from the server
    response = client.recv(200 * 200)

    # Close the client connection
    client.close
    return response.lines
  end

  def draw_list()
    @selected = @list[@idx]
    i = 0
    for x in @list
      out @cursor.up(1)
      out @cursor.clear_line
      if i == @idx
        out @pastel.lookup(:bold)
      end
      out x.strip
      out @pastel.lookup(:reset)
      i += 1
    end
  end

  def handle_event(event)
    out @cursor.save
    if event.key.name == :alpha
      query = event.line[3..-1]
      if query.size > 2
        @list = get_res_from_server(query)
        draw_list
      end
    end

    if event.key.name == :up
      @idx += 1 if @idx < @list.size - 1
      draw_list
    elsif event.key.name == :down
      @idx -= 1 if @idx > 0
      draw_list
    end

    out @cursor.up(1)
    out @cursor.clear_line
    out @cursor.restore
  end
end

