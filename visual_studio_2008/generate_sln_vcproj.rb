# This script generates .sln files and .vcproj files

require 'erb'
require 'fileutils'
require 'pathname'

def get_uid()
	`uuidgen`.strip().upcase()
end

def get_vs_path(category_path, example_base_name)
	File.join(category_path, example_base_name) # unix path used to create file
end

def get_vs_sln_path(vs_path, example_base_name)
	"./#{vs_path}/#{example_base_name}.sln"
end

def get_vs_vcproj_path(vs_path, example_base_name)
	"./#{vs_path}/#{example_base_name}.vcproj"
end

def generate(category_path, example_base_name, example_extension)
	guid_sln = get_uid()
	guid_vcproj = get_uid()
	vs_path = get_vs_path(category_path, example_base_name)

	relative_path.gsub!('/', '\\')

	relative_path = '..\\..'
	if (not category_path.empty?)
		relative_path += '\\..\\' + category_path # Windows path gets inserted into vcproj
	end
	relative_path += '\\' # Windows path gets inserted into vcproj

	puts [vs_path, relative_path, example_base_name, example_extension].join(' ')

	FileUtils.mkdir_p vs_path

	template = ERB.new File.new("ljm_example.sln.erb", 'r').read
	File.open(get_vs_sln_path(vs_path, example_base_name), 'w') {|f| f.write(template.result(binding)) }

	template = ERB.new File.new("ljm_example.vcproj.erb", 'r').read
	File.open(get_vs_vcproj_path(vs_path, example_base_name), 'w') {|f| f.write(template.result(binding)) }
end

def generate_if_not_exist(category_path, example_base_name, example_extension)
	vs_path = get_vs_path(category_path, example_base_name)
	generate(category_path, example_base_name, example_extension) unless
		Pathname.new(get_vs_sln_path(vs_path, example_base_name)).exist? and
		Pathname.new(get_vs_vcproj_path(vs_path, example_base_name)).exist?
end

