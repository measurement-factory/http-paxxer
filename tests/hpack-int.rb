#!/usr/bin/ruby

# Copyright (c) 2015, The Measurement Factory.

# Warning: Unaudited code!

# Encoding/decoding for the integer values
# based on HPAC draft-ietf-httpbis-header-compression-10, Section 5.1

def print_usage()
   $stderr.puts "Usage: #{$0} [--pack] [-v <value>] [-p prefix] [-o <output-format>]\n"
   $stderr.puts "       #{$0} --parse [-p prefix]\n"
   $stderr.puts "       #{$0} --help\n"
   $stderr.puts " where:\n"
   $stderr.puts "   -v <value>  - unsigned integer value. If not defined then value will be expected from STDIN.\n"
   $stderr.puts "   -p <prefix> - parameter of encoding/decoding algorithm. May be in range 1..8. Default is 5.\n"
   $stderr.puts "   -o <format> - format of presentation for the output data. May be \"bin\" or \"text\". Default is \"bin\".\n"
   exit -1;
end

$is_packing = true  # default operation
$int_value = -1     # (-1) is not defined value
$bin_out = true     # default output format
$prefix = 5         # default prefix value

def handle_argv()
  i = 0
  size = ARGV.size
  while i < size do
      op = ARGV[i];
      i = i+1
      case op
        when "--pack"
          $is_packing = true
        when "--parse"
          $is_packing = false
        when "-v"
          if i >= size
            print "Expected output format but End-Of-Params found."
            print_usage()
          end
          $int_value = ARGV[i].to_i();
          i = i+1;
          if $int_value < 0
            print "Integer value must be unsigned.\n"
          end
        when "-p"
          if i >= size
            print "Expected output format but End-Of-Params found."
            print_usage()
          end
          $prefix = ARGV[i].to_i()
          i = i+1;
          if ($prefix < 1 || $prefix > 8)
            print "Invalid prefix value: #{$prefix}.\n"
            print_usage();
          end;
        when "-o"
          if i >= size
            print "Expected output format but End-Of-Params found."
            print_usage()
          end
          foo = ARGV[i]
          i = i+1
          case foo
            when "bin"
              $bin_out = true
            when "text"
              $bin_out = false
            else
              print "Unrecognized output format: #{foo}.\n"
              print_usage()
          end
        when "--help"
          print_usage()
        else
          print "Unrecognized option: #{ARGV[i-1]}"
          print_usage()
      end
  end
end

# test for arguments
def print_arguments()
  if $is_packing
    print "PACKING: value = #{$int_value}, prefix = #{$prefix}, binary output = #{$bin_out}\n"
  else
    print "PARSING: prefix = #{$prefix}\n"
  end
end


#        if I < 2^N - 1, encode I on N bits
#        else
#            encode (2^N - 1) on N bits
#            I = I - (2^N - 1)
#            while I >= 128
#                encode (I % 128 + 128) on 8 bits
#                I = I / 128
#            encode I on 8 bits
def encode(value)
  max_prefix_byte = (1 << $prefix) - 1

  res = ""

  if value < max_prefix_byte
    if $bin_out then res << value else res += sprintf "%02x ", value end
  else
    if $bin_out then res << max_prefix_byte else res += sprintf "%02x ", max_prefix_byte end
    i = value - max_prefix_byte
    while (i >= 128) do
      ch = (i % 128) + 128
      if $bin_out then res << ch else res += sprintf "%02x ", ch end
      i /= 128;
    end
    if $bin_out then res << i else res += sprintf "%02x ", i end
  end

  if $bin_out then print "#{res}" else print "#{res}\n" end
  STDOUT.flush
end


#        decode I from the next N bits
#        if I < 2^N - 1, return I
#        else
#            M = 0
#            repeat
#                B = next octet
#                I = I + (B & 127) * 2^M
#                M = M + 7
#            while B & 128 == 128
#        return I
def decode(str)
  max_prefix_byte = (1 << $prefix) - 1
  buffer = []
  str.each_byte do |b| buffer << b end
  size = buffer.size

  i = 0;
  while i < size do
    val = buffer[i] & max_prefix_byte; i += 1
    if val >= max_prefix_byte
      m = 0;
      loop do
        if (i >= size)
          $stderr.puts "premature end of integer encoding at byte ${i}\n"
          exit -2
        end

        ch = buffer[i]; i += 1
	    d  = ch & 127;
		if d > 0
          delta = d << m
          val += delta;
	    end

        break if ((ch & 128) == 0)

        m += 7;
      end
    end
    print "#{val}\n"
    STDOUT.flush
  end
end

handle_argv()
#print_arguments()

if $is_packing
  if $int_value >= 0
    encode($int_value)
  else
    STDIN.each do |x|
      list = x.split(' ')
      list.each do |i|
        encode(i.to_i())
      end
    end
  end
else
  STDIN.each do |str|
    decode(str)
  end
end

exit 0
