
# run_tests.rb - run all my tests in this folder and below
if __FILE__ == $0
   # dir = File.dirname(__FILE__)
   # tests = Dir["#{dir}/**/*_spec.rb"] # anything named *_spec.rb
   #
   # # add additional tests that don't follow conventional naming schemes
   # # %w(anotherfile, onemorefile).each do |f|
   # #   tests << File.join(dir, f) + ".rb"
   # # end
   #
   # puts "Testing: #{tests.join(', ')}"
   #
   # tests.each do |file|
   #   load file, true
   # end

   # TODO: Something that doesn't suck soo much
   def print_and_test(rspec_file)
     puts
     puts "=========================================="
     puts "#{rspec_file}..."
     system "rspec '#{rspec_file}'"
   end

   print_and_test 'test/copy_example_accessories_spec.rb'
   print_and_test 'test/generate_example_accessories_spec.rb'
end


