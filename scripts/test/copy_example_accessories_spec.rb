# copy_example_accessories_test.rb
#
# Purpose: Tests copy_example_accessories.rb
#
# Run:
#   $ rspec copy_example_accessories_test.rb
#
# Requirements: rspec
# $ gem install rspec


# Exit if not called from rspec
if __FILE__ == $0
  puts "Usage:"
  puts "  $ rspec #{__FILE__}"
  exit 1
end

require File.dirname(__FILE__) + '/../copy_example_accessories.rb'

describe ExampleAccessoryCopier, "#copy_all" do
  it "quits with helpful message if dest is not set" do
    cop = ExampleAccessoryCopier.new
    result = cop.copy_all()
    expect(result["status"] != 0)
    expect(result["message"].include? "--dest=")
    expect(result["message"].include? "not set")
  end

  it "does ALL THE THINGS" do
    cop = ExampleAccessoryCopier.new
    cop.dest = "test_dir"

    expect(cop).to receive(:dest_refers_to_exising_dir?).and_return true

    expect(cop).to receive :move_to_work_dir
    expect(cop).to receive :clear_old_temp_dir
    expect(cop).to receive :create_temp_dir
    files = {
      '.' => ['test_file'],
      'sub' => ['sub_file', 'other_sub_file']
    }
    expect(cop).to receive(:parse_examples_structure).with("../EXAMPLES_MASTER_LIST", :get_all => true).and_return files
    expect(cop).to receive(:copy_to_temp).with('.', 'test_file')
    expect(cop).to receive(:copy_to_temp).with('.', 'make.sh')
    expect(cop).to receive(:copy_to_temp).with('.', 'SConstruct')
    expect(cop).to receive(:copy_to_temp).with('.', 'make_all.sh')
    expect(cop).to receive(:copy_to_temp).with('sub', 'sub_file')
    expect(cop).to receive(:copy_to_temp).with('sub', 'other_sub_file')
    expect(cop).to receive(:copy_to_temp).with('sub', 'make.sh')
    expect(cop).to receive(:copy_to_temp).with('sub', 'SConstruct')
    expect(cop).to receive :copy_temp_to_dest
    expect(cop).to receive :clear_old_temp_dir
    expect(cop).to receive :move_to_pop_dir

    result = cop.copy_all()
    expect(result["status"] == 0)
    expect(result["message"].include? "Success")
  end
end
