
teapot_version "1.0"

define_project "Transform Flow Visualisation" do |project|
	project.add_author "Samuel Williams"
	project.license = "MIT License"
	
	project.version = "0.2.0"
end

define_target "transform-flow-visualisation" do |target|
	target.build do |environment|
		source_root = target.package.path + 'source'
		
		copy headers: source_root.glob('TransformFlow-Visualisation/**/*.{h,hpp}')
		copy files: source_root.glob('TransformFlow-Visualisation/Resources**/*'), prefix: 'share/TransformFlow-Visualisation'
		
		build executable: "TransformFlow", source_files: source_root.glob('TransformFlow-Visualisation/**/*.cpp')
	end
	
	target.depends "Build/Files"
	target.depends "Build/Clang"
	
	target.depends :platform
	target.depends "Language/C++11"
	
	target.depends "Library/Dream/Client"
	target.depends "Library/TransformFlow"
	
	target.provides "Application/TransformFlowVisualisation"
end

define_configuration "transform-flow-visualisation" do |configuration|
	configuration.group do
		configuration[:source] = "https://github.com/dream-framework"
		
		configuration.require "platforms"
		
		configuration.require "dream-client"
		
		configuration.require "opencv"
	end
	
	configuration.group do
		configuration[:source] = "https://github.com/HITLabNZ"
		
		configuration.require "transform-flow"
	end
	
	configuration[:run] = ["Library/TransformFlow"]
end
