# encoding: ascii-8bit

# Create the overall gemspec
Gem::Specification.new do |s|
  s.name = 'openc3-cosmos-cfs'
  s.summary = 'OpenC3 openc3-cosmos-cfs plugin'
  s.description = <<-EOF
    openc3-cosmos-cfs plugin for deployment to OpenC3
  EOF
  s.license = 'Apache-2.0'
  s.authors = ['NASA']
  s.email = ['cfs-program@lists.nasa.gov']
  s.homepage = 'https://github.com/nasa/cfs'
  s.platform = Gem::Platform::RUBY

  if ENV['VERSION']
    s.version = ENV['VERSION'].dup
  else
    time = Time.now.strftime("%Y%m%d%H%M%S")
    s.version = '7.0.0' + ".#{time}"
  end
  s.files = Dir.glob("{targets,lib,tools,microservices}/**/*") + %w(Rakefile README.md LICENSE.txt plugin.txt requirements.txt)
end
