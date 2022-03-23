#! /usr/bin/env ruby
# generate_example_accessories.rb
#
# Purpose: Generates all files accociated with the LJM examples.
#
# Arguments: --verbose (default is silent)
#            --no-visual-studio (default is yes)
#
# Test: Run generate_example_accessories_test.rb.
#   See generate_example_accessories_test.rb for details.

require 'erb'
require 'fileutils'
require 'pathname'

require File.join(File.dirname(__FILE__), 'ExampleAccessoryBase.rb')

WINDOWS_ONLY_DIRS = ['more/utilities/dynamic_runtime_linking']

class ExampleAccessoryGenerator < ExampleAccessoryBase
  @@MAKE_ALL_ERB = File.join(@@REL_TO_TEMPLATES, "#{@@MAKE_ALL}.erb")
  @@MAKE_ERB = File.join(@@REL_TO_TEMPLATES, "#{@@MAKE}.erb")
  @@SCONS_ERB = File.join(@@REL_TO_TEMPLATES, "#{@@SCONS}.erb")
  @@LJM_EXAMPLE_SLN_ERB = File.join(@@REL_TO_TEMPLATES, "ljm_example.sln.erb")
  @@LJM_EXAMPLE_VCPROJ_ERB = File.join(@@REL_TO_TEMPLATES, "ljm_example.vcproj.erb")

  @@VISUAL_STUDIO_DIR = 'visual_studio_2008'

  def initialize()
    super
  end

  def get_uid()
  	`uuidgen`.strip().upcase()
  end

  def get_vs_sln_path(vs_path, example_base_name)
  	"#{vs_path}/#{example_base_name}.sln"
  end

  def get_vs_vcproj_path(vs_path, example_base_name)
  	"#{vs_path}/#{example_base_name}.vcproj"
  end

  def get_vs_subpath(vs_path, example_base_name)
    if vs_path == '.'
      return "#{@@VISUAL_STUDIO_DIR}/#{example_base_name}"
    end

    "#{@@VISUAL_STUDIO_DIR}/#{vs_path}/#{example_base_name}"
  end

  def generate_vs(category_path, example_base_name, example_extension)
  	guid_sln = get_uid()
  	guid_vcproj = get_uid()
    if guid_sln == guid_vcproj
      raise 'guid_sln == guid_vcproj'
    end

  	vs_path = append_rel_path(get_vs_subpath(category_path, example_base_name))

  	relative_path = '..\\..'
  	if category_path != '.'
  		relative_path += '\\..\\' + category_path
      category_path.count('/').times do |iter|
        relative_path = '..\\' + relative_path
      end
  	end
  	relative_path += '\\' # Windows path gets inserted into vcproj
    relative_path.gsub!('/', '\\')

  	verbose_puts "    #{[vs_path, relative_path, example_base_name, example_extension].join(' ')}"

    FileUtils.mkdir_p vs_path

    template = ERB.new File.new(@@LJM_EXAMPLE_SLN_ERB, 'r').read
    File.open(get_vs_sln_path(vs_path, example_base_name), 'w') {|f| f.write(template.result(binding)) }

    template = ERB.new File.new(@@LJM_EXAMPLE_VCPROJ_ERB, 'r').read
    File.open(get_vs_vcproj_path(vs_path, example_base_name), 'w') {|f| f.write(template.result(binding)) }
  end

  # Adds the example to the Visual Studio export list
  # Generates the relevant .vcproj and .sln files for the file if they
  #   don't exist.
  def process_for_visual_studio(category_path, example_base_name, example_extension)
  	vs_path = get_vs_subpath(category_path, example_base_name)

    File.open(@@VS_MASTER_LIST, 'a') do |file|
      file.write("#{get_vs_sln_path(vs_path, example_base_name)}\n")
      file.write("#{get_vs_vcproj_path(vs_path, example_base_name)}\n")
    end

    vs_path = append_rel_path(vs_path)
    vs_sln = get_vs_sln_path(vs_path, example_base_name)
    vs_vcproj = get_vs_vcproj_path(vs_path, example_base_name)

    if not file_exist? vs_sln or not file_exist? vs_vcproj
      generate_vs(category_path, example_base_name, example_extension)
    end
  end

  # Gets the relative file path of a file in the examples/ dir.
  # @param path: the dir examples/<path>
  def append_rel_path(path)
    return File.join(@@REL_TO_EXAMPLES, path)
  end

  # Gets the relative file path of a file in the examples/ dir.
  # @param dir: the dir examples/<dir>
  # @param filename: the file examples/<dir>/<filename>
  def get_rel_path(dir, filename)
    rel_dir = append_rel_path(dir)
    return File.join(rel_dir, filename)
  end

  # Filters out WINDOWS_ONLY_DIRS, without modifying `dirs`
  def get_nix_dirs(dirs)
    nix_dirs = dirs.dup
    for win_only_dir in WINDOWS_ONLY_DIRS
      nix_dirs.delete(win_only_dir)
    end
    return nix_dirs
  end

  def generate_make_all_sh(dirs)
    dirs.sort!
    nix_dirs = get_nix_dirs(dirs)
    verbose_puts "  #{@@MAKE_ALL}..."
    template = ERB.new(File.new(@@MAKE_ALL_ERB, 'r').read)
    File.open(@@MAKE_ALL_LOCATION, 'w') {|f| f.write(template.result(binding)) }
  end

  def generate_make_sh(dirs)
    verbose_puts "  #{@@MAKE}..."
    get_nix_dirs(dirs).each do |dir|
      rel_make = get_rel_path(dir, @@MAKE)

      rel_scons_path = ''
      if dir != '.'
        rel_scons_path << '../'
        dir.count('/').times do |iter|
          rel_scons_path = '../' + rel_scons_path
        end
      end

      template = ERB.new(File.new(@@MAKE_ERB, 'r').read)
      File.open(rel_make, 'w') {|f| f.write(template.result(binding)) }
    end
  end

  def generate_scons(dirs, dir_hash)
    for dir in get_nix_dirs(dirs)
      generate_scons_inner(dir, dir_hash[dir])
    end
  end

  def generate_scons_inner(dir, examples)
    rel_scons = get_rel_path(dir, @@SCONS)
    template = ERB.new(File.new(@@SCONS_ERB, 'r').read)
    File.open(rel_scons, 'w') {|f| f.write(template.result(binding)) }
  end

  def generate_vs_files(dir, examples)
    for example in examples.each
      ext = File.extname(example)
      basename = File.basename(example, ext)
      process_for_visual_studio(dir, basename, ext)
    end
  end

  def generate_all()
    begin
      move_to_work_dir()

      dir_hash = get_examples_structure()
      verbose_puts_examples_structure dir_hash

      verbose_puts "Generating..."
      dirs = dir_hash.keys
      generate_make_all_sh(dirs)
      generate_make_sh(dirs)

      verbose_puts "  #{@@SCONS}..."
      generate_scons(dirs, dir_hash)

      if @visual_studio
        verbose_puts "  visual studio .vcproj and .vs files..."

        # Clear out the existing file
        File.open(@@VS_MASTER_LIST, 'w')

        for dir in dirs
          generate_vs_files(dir, dir_hash[dir])
        end
      end
    ensure
      move_to_pop_dir()
    end
  end
end

if __FILE__ == $0
  gen = ExampleAccessoryGenerator.new

  gen.verbose = false
  gen.work_dir = gen.ABS_PATH
  ARGV.each do |arg|
    if arg == "--verbose"
      gen.verbose = true
    end

    if arg == "--no-visual-studio"
        gen.visual_studio = false
    end
  end

  gen.generate_all()
end
