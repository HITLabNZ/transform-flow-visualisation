
add_application 'Transform Flow' do
	compile_executable 'transform-flow-visualisation' do
		def source_files(environment)
			FileList[root, "TransformFlow-Visualisation/**/*.cpp"]
		end
	end
	
	copy_files do
		def source_files(environment)
			FileList[root + "TransformFlow-Visualisation/Resources", '**/*']
		end
	end
end
