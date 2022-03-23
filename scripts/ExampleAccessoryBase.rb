# generate_example_accessories.rb
#
# Purpose: Generates all files accociated with the LJM examples.
#
# Arguments: --verbose (default is silent)
#
# Test: Run generate_example_accessories_test.rb.
#   See generate_example_accessories_test.rb for details.

require 'pathname'
require 'fileutils'

class ExampleAccessoryBase
  attr_accessor :verbose
  attr_accessor :visual_studio
  attr_accessor :work_dir

  attr_accessor :ABS_PATH
  attr_accessor :REL_TO_EXAMPLES
  attr_accessor :EXAMPLES_MASTER_LIST
  attr_accessor :VS_MASTER_LIST
  attr_accessor :MAKE_ALL
  attr_accessor :MAKE_ALL_LOCATION
  attr_accessor :MAKE
  attr_accessor :SCONS

  @@REL_TO_EXAMPLES = '..'
  @@REL_TO_TEMPLATES = 'template'
  @@EXAMPLES_MASTER_LIST = "#{@@REL_TO_EXAMPLES}/EXAMPLES_MASTER_LIST"
  @@VS_MASTER_LIST = "#{@@REL_TO_EXAMPLES}/VS_EXAMPLES_MASTER_LIST"
  @@MAKE_ALL = 'make_all.sh'
  @@MAKE_ALL_LOCATION = "#{@@REL_TO_EXAMPLES}/#{@@MAKE_ALL}"
  @@MAKE = 'make.sh'
  @@SCONS = 'SConstruct'

  def initialize
    @verbose = false
    @visual_studio = true
    @work_dir = nil
    @pop_dir = nil
    @ABS_PATH = File.expand_path File.dirname(__FILE__)
  end

  def get_examples_structure()
    parse_examples_structure(@@EXAMPLES_MASTER_LIST)
  end

  def parse_examples_structure(rel_file_location, hash={})
    abs_loc = File.expand_path(rel_file_location)
    dir_hash = {}
    File.open(abs_loc) do |file|
      file.each_line do |line|
        line.strip!

        if not hash[:get_all] and not (line.end_with? '.c' or line.end_with? '.cpp')
          next
        end

        dir = line.split('/')
        example = dir.pop
        if dir.size == 0
          dir = ['.']
        end
        path = dir[0]

        for iter in 1...dir.size
          path += "/" + dir[iter]
        end

        if !dir_hash[path]
          dir_hash[path] = []
        end

        dir_hash[path].push(example)

      end
    end
    return dir_hash
  end

  def verbose_puts(str)
    if @verbose
      puts str
    end
  end

  def file_exist?(path)
    Pathname.new(path).exist?
  end

  # Change to the given directory
  def cd_to(dir)
    Dir.chdir dir
  end

  # Moves to the work directory if it is not nil, saving the current
  # directory first
  def move_to_work_dir()
    if not @work_dir.nil?
      @pop_dir = Dir.pwd
      cd_to(@work_dir)
    end
  end

  # Moves to the pop directory (the directory previous to the work directory)
  # if it is not nil
  def move_to_pop_dir()
    if not @pop_dir.nil?
      cd_to(@pop_dir)
    end
  end

  def verbose_puts_examples_structure(dir_hash)
    if not @verbose
      return
    end

    verbose_puts "Examples structure:"
    dir_hash.each_key do |key|
      verbose_puts "  #{key}"
      verbose_puts "    #{dir_hash[key]}"
      verbose_puts ""
    end
  end
end
