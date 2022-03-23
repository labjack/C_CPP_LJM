# generate_example_accessories_test.rb
#
# Purpose: Tests generate_example_accessories.rb
#
# Run:
#   $ rspec generate_example_accessories_test.rb
#
# Requirements: rspec
# $ gem install rspec


# Exit if not called from rspec
if __FILE__ == $0
  puts "Usage:"
  puts "  $ rspec #{__FILE__}"
  exit 1
end


require File.dirname(__FILE__) + '/../generate_example_accessories.rb'

class MockFile
  attr_accessor :lines

  def initialize
    @lines = []
  end

  def write(arg)
    @lines << arg
  end
end

describe ExampleAccessoryGenerator, "#get_examples_structure" do
  it "calls parse_examples_structure" do
    gen = ExampleAccessoryGenerator.new
    test_examples = ['ex1', 'ex2']
    test_structure = {'dir' => test_examples}
    expect(gen).to receive(:parse_examples_structure).with(kind_of(String)).and_return(test_structure)

    result = gen.get_examples_structure
    expect(result['dir']).to eq(test_examples)
  end
end

describe ExampleAccessoryGenerator, "#parse_examples_structure" do
  it "works with test example list" do
    gen = ExampleAccessoryGenerator.new
    test_structure = gen.parse_examples_structure("test/TEST_EXAMPLE_LIST")
    expect(test_structure.keys.size).to eq(3)
    expect(test_structure['.'].size).to eq(2)
    expect(test_structure['.']).to eq(['eAddresses.c', 'example.cpp'])
    expect(test_structure['test'].size).to eq(1)
    expect(test_structure['test']).to eq(['test_example.c'])
    expect(test_structure['foo/bar']).to eq(['other.c'])
  end
end

describe ExampleAccessoryGenerator, "#get_vs_subpath" do
  it "does not include . as a path" do
    test_result = ExampleAccessoryGenerator.new.get_vs_subpath('.', 'test_example')
    expect(test_result).to eq('visual_studio_2008/test_example')
  end

  it "includes category in path" do
    test_result = ExampleAccessoryGenerator.new.get_vs_subpath('test', 'test_example')
    expect(test_result).to eq('visual_studio_2008/test/test_example')
  end
end

describe ExampleAccessoryGenerator, "#generate_vs_files" do
  it "gets uid, creates dir, and opens files to write" do
    gen = ExampleAccessoryGenerator.new
    expect(gen).to receive(:get_uid).twice().and_call_original

    expect(FileUtils).to receive(:mkdir_p) do |dir|
      expect(dir).to eq("../visual_studio_2008/foo/test_example")
    end
    expect(File).to receive(:open).ordered do |filename, mode|
      expect(filename).to eq("../visual_studio_2008/foo/test_example/test_example.sln")
      expect(mode).to eq('w')
    end
    expect(File).to receive(:open).ordered do |filename, mode|
      expect(filename).to eq("../visual_studio_2008/foo/test_example/test_example.vcproj")
      expect(mode).to eq('w')
    end

    gen.generate_vs('foo', 'test_example', '.c')
  end
end

describe ExampleAccessoryGenerator, "#process_for_visual_studio" do
  it "adds example entries to vs export list, checks for existing files" do
    gen = ExampleAccessoryGenerator.new

    allow(File).to receive(:open).with(
      include("LabJackM/Visual Studio Examples/test_example/test_example.vcproj"),
      "w"
    )
    expect(File).to receive(:open) do |filename, mode|
      expect(filename).to eq("../VS_EXAMPLES_MASTER_LIST")
      expect(mode).to eq('a')
    end
    expect(gen).to receive(:file_exist?).with(
      "../visual_studio_2008/foo/test_example/test_example.sln").and_return true
    expect(gen).to receive(:file_exist?).with(
      "../visual_studio_2008/foo/test_example/test_example.vcproj").and_return true
    expect(gen).to_not receive(:generate_vs)

    gen.process_for_visual_studio('foo', 'test_example', '.c')
  end

  it "adds example entries to vs export list, generates files" do
    gen = ExampleAccessoryGenerator.new
    mock_file = MockFile.new

    allow(File).to receive(:open).with(
      include("LabJackM/Visual Studio Examples/test_example/test_example.vcproj"),
      "w"
    )
    expect(File).to receive(:open).with("../VS_EXAMPLES_MASTER_LIST", 'a').and_yield(mock_file)
    allow(gen).to receive(:file_exist?).with(kind_of(String)).and_return false
    expect(gen).to receive(:generate_vs)

    gen.process_for_visual_studio('foo', 'test_example', '.c')
    expect(mock_file.lines.size()).to eq(2)
    expect(mock_file.lines.include? "visual_studio_2008/foo/test_example/test_example.vcproj")
    expect(mock_file.lines.include? "visual_studio_2008/foo/test_example/test_example.sln")
  end
end

describe ExampleAccessoryGenerator, "#generate_all" do
  it "does ALL THE THINGS" do
    gen = ExampleAccessoryGenerator.new

    expect(gen).to receive(:move_to_work_dir)

    test_hash = {
      '.' => ['ex0.c'],
      'foo' => ['ex1.c', 'ex2.cpp']
    }
    test_dirs = ['.', 'foo']
    expect(gen).to receive(:get_examples_structure).and_return test_hash

    allow(gen).to receive(:verbose_puts) # verbose_puts, you're a dumb function and no one cares.

    expect(gen).to receive(:generate_make_all_sh).with(test_dirs)
    expect(gen).to receive(:generate_make_sh).with(test_dirs)

    expect(gen).to receive(:generate_scons).with('.', ['ex0.c'])
    expect(gen).to receive(:generate_scons).with('foo', ['ex1.c', 'ex2.cpp'])

    expect(File).to receive(:open).with('../VS_EXAMPLES_MASTER_LIST', 'w')
    expect(gen).to receive(:generate_vs_files).with('.', ['ex0.c'])
    expect(gen).to receive(:generate_vs_files).with('foo', ['ex1.c', 'ex2.cpp'])

    expect(gen).to receive(:move_to_pop_dir)

    gen.generate_all
  end
end
