# copy_example_accessories.rb
#
# Purpose: Copy all files accociated with the LJM examples to a given
#   destination.
#
# Arguments: --dest=<dir> (<dir>'s contents will not be deleted, but may be
#                          overwritten)
#            --verbose (default is silent)
#
# Test: Run copy_example_accessories_test.rb.
#   See copy_example_accessories_test.rb for details.

require File.dirname(__FILE__) + '/ExampleAccessoryBase.rb'

class ExampleAccessoryCopier < ExampleAccessoryBase
  attr_accessor :dest
  attr_accessor :temp_dir

  def initialize()
    super

    @is_windows = false
    @dest = nil
    @temp_dir = "temp"
    @temp_hold = ".#{File.basename(__FILE__)}.hold"
  end

  def clear_old_temp_dir()
    if file_exist? @temp_dir
      FileUtils.remove_entry_secure @temp_hold, :verbose => @verbose
      FileUtils.mv @temp_dir, @temp_hold, :verbose => @verbose
    end
  end

  def create_temp_dir()
    Dir.mkdir(@temp_dir)
  end

  def ensure_dir_for_file(file)
    dest_dir = File.split(file)[0]
    if not File.directory? dest_dir
      FileUtils.mkdir_p dest_dir, :verbose => @verbose
    end
  end

  def copy_to_temp(dir, file)
    pathed_file = File.join(dir, file)
    source = File.join(@@REL_TO_EXAMPLES, pathed_file)
    if not file_exist? source
      raise "File does not exist: #{File.join(Dir.pwd, source)}"
    end

    dest = File.join(@temp_dir, pathed_file)
    ensure_dir_for_file(dest)

    FileUtils.cp(source, dest, :verbose => @verbose)
  end

  def copy_contents_r(dir)
    Dir.entries(dir).each do |file|
      if file == '.' or file == '..'
        next
      end

      pathed_file = File.join(dir, file)
      if File.directory?(pathed_file)
        copy_contents_r pathed_file
      else
        sub_path_array = pathed_file.split(File::SEPARATOR)
        sub_path_array_minus_temp = sub_path_array.slice(1, 10)
        sub_path = sub_path_array_minus_temp.join(File::SEPARATOR)
        ensure_dir_for_file File.join(@abs_dest, sub_path)
        FileUtils.cp_r(
          pathed_file,
          File.join(@abs_dest, sub_path),
          :verbose => @verbose,
          :remove_destination => true
        )
      end
    end
  end

  def copy_temp_to_dest()
    copy_contents_r @temp_dir
  end

  def copy_windows_example_files()
    raise "copy_windows_example_files is not implemented"
  end

  def dest_refers_to_exising_dir?()
    return File.directory? @dest
  end

  def copy_all()
    if @dest == nil
      return {"error" => 1, "message" => "Error: --dest= not set"}
    end

    if not dest_refers_to_exising_dir?()
      return {"error" => 1, "message" => "Error: --dest='#{@dest}' does not refer to an existing directory"}
    end

    # Set up the dest folder abolutely, so that it works when we move to work dir
    @abs_dest = File.expand_path(@dest)

    begin
      move_to_work_dir()

      if File.expand_path(@abs_dest) == File.expand_path(@temp_dir)
        raise "Error: --dest='#{@dest}' is the temporary/staging dir. Use a different dir."
      end

      clear_old_temp_dir()
      create_temp_dir()
      dir_hash = parse_examples_structure(@@EXAMPLES_MASTER_LIST, :get_all => true)

      # Get the regular source files
      dir_hash.each_pair do |dir, files|
        files.each do |file|
          copy_to_temp dir, file
        end
      end

      # Get the special build files
      copy_to_temp '.', 'make_all.sh'

      if @is_windows
        copy_windows_example_files()
      end

      copy_temp_to_dest()
      clear_old_temp_dir()

      verbose_puts_examples_structure(dir_hash)

    ensure
      move_to_pop_dir()
    end
    return {"error" => 0, "message" => "Success: Files copied to #{@dest}"}
  end

  def usage()
    puts "#{__FILE__}: --dest=<destination directory> [--verbose]"
  end
end

if __FILE__ == $0
  copier = ExampleAccessoryCopier.new

  copier.work_dir = copier.ABS_PATH
  ARGV.each do |arg|
    if arg == "--verbose"
      copier.verbose = true
    elsif arg.start_with? "--dest="
      copier.dest = arg.sub(/^--dest=/, '')
    elsif arg == "-h" or arg == "--help"
      copier.usage()
      exit 0
    end
  end

  result = copier.copy_all()
  if (result["error"] != 0)
    puts result["message"]
  end
end
