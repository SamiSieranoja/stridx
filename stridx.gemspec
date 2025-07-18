Gem::Specification.new do |spec|
  spec.name = "StrIdx"
  spec.version = "0.1.7"
  spec.authors = ["Sami Sieranoja"]
  spec.email = ["sami.sieranoja@gmail.com"]

  spec.summary = %q{StrIdx}
  spec.description = %q{ Fast fuzzy string similarity search and indexing (for filenames)}
  spec.homepage = "https://github.com/SamiSieranoja/stridx"
  spec.metadata["source_code_uri"] = spec.homepage
  spec.metadata["homepage_uri"] = spec.homepage

  spec.files = `git ls-files -z`.split("\x0").reject do |f|
    f.match(%r{^(refcode|spec|features)/})
  end

  spec.bindir = "exe"
  spec.executables = spec.files.grep(%r{^exe/}) { |f| File.basename(f) }
  spec.require_paths = ["lib", "ext"]

  spec.add_development_dependency "bundler", "~> 2.4.21"
  spec.add_development_dependency "rake", "~> 13.1.0"

  spec.add_runtime_dependency "tty-cursor", "~> 0.7.1"
  spec.add_runtime_dependency "tty-prompt", "~> 0.23.1"
  spec.add_runtime_dependency "tty-reader", "~> 0.9.0"
  spec.add_runtime_dependency "tty-screen", "~> 0.8.2"
  spec.add_runtime_dependency "pastel", "~> 0.8.0"
  spec.add_runtime_dependency "daemons", "~> 1.4.1"

  spec.extensions = ["rubyext/extconf.rb"]
  spec.licenses = ["LGPL-2.0+"]
end
