//
//  main.cpp
//  Demo
//
//  Created by Samuel Williams on 16/09/11.
//  Copyright 2011 Orion Transfer Ltd. All rights reserved.
//

#include <Dream/Display/Application.h>
#include <Dream/Client/ApplicationDelegate.h>
#include <Dream/Client/Run.h>

#include <Dream/Graphics/ShaderManager.h>
#include <Dream/Graphics/TextureManager.h>

#include <Dream/Renderer/Viewport.h>
#include <Dream/Renderer/BirdsEyeCamera.h>
#include <Dream/Renderer/PointCamera.h>
#include <Dream/Renderer/Projection.h>

#include <Dream/Events/Logger.h>

#include <Dream/Text/Font.h>
#include <Dream/Text/TextBuffer.h>

#include <Dream/Graphics/MeshBuffer.h>
#include <Dream/Graphics/ImageRenderer.h>
#include <Dream/Graphics/WireframeRenderer.h>

#include <Euclid/Geometry/Generate/Planar.h>

#include <Euclid/Numerics/Distribution.h>

#include <TransformFlow/VideoStream.h>
#include <TransformFlow/BasicSensorMotionModel.h>
#include <TransformFlow/HybridMotionModel.h>
#include <TransformFlow/OpticalFlowMotionModel.h>

#include "Renderer/VideoStreamRenderer.h"

namespace TransformFlow
{
	using namespace Dream;
	using namespace Dream::Events;
	using namespace Dream::Resources;
	using namespace Dream::Renderer;
	using namespace Dream::Display;
	using namespace Dream::Graphics;
	using namespace Euclid;
	
	class ImageSequenceScene : public Scene
	{
	protected:
		// These are the locations of default attributes in the shader programs used:
		enum ProgramAttributes {
			POSITION = 0,
			NORMAL = 1,
			COLOR = 2,
			MAPPING = 3
		};
		
		Ref<BirdsEyeCamera> _camera;
		Ref<IProjection> _projection;
		Ref<Viewport> _viewport;
				
		Ref<ShaderManager> _shader_manager;
		Ref<TextureManager> _texture_manager;
		Ref<ImageRenderer> _pixel_buffer_renderer;
		
		Ref<WireframeRenderer> _wireframe_renderer;
		Ref<Program> _wireframe_program;
		
		typedef Geometry::Mesh<> MeshT;
		Ref<MeshBuffer<MeshT>> _grid_mesh_buffer;
		
		Ref<VideoStream> _video_stream;
		Ref<Renderer::VideoStreamRenderer> _video_stream_renderer;

		// Text rendering
		Ref<Text::Font> _font;
		Ref<Text::TextBuffer> _text_buffer;
		Ref<ImageRenderer> _text_renderer;
		Ref<Program> _text_program;

		std::size_t _tracking_point_sequence;

		void dump_tracking_points();
		void reset_tracking_points();
		void evaluate_tracking_points_bearing();

	public:
		virtual ~ImageSequenceScene ();
		
		void set_video_stream(Ptr<VideoStream> video_stream) { _video_stream = video_stream; }
		
		virtual void will_become_current (ISceneManager *);
		virtual void will_revoke_current (ISceneManager *);
		
		virtual bool event (const EventInput & input);
		virtual bool button (const ButtonInput & input);
		virtual bool motion (const MotionInput & input);
		virtual bool resize (const ResizeInput & input);
		
		virtual void render_frame_for_time (TimeT time);
	};
	
	ImageSequenceScene::~ImageSequenceScene ()
	{
	}
		
	void ImageSequenceScene::will_become_current(ISceneManager * manager) {
		Scene::will_become_current(manager);
		
		glEnable(GL_DEPTH_TEST);
		
		check_graphics_error();
		
		_tracking_point_sequence = 0;
		
		_camera = new BirdsEyeCamera;
		_camera->set_distance(10);
		_camera->set_multiplier(Vec3(0.1, 0.1, 0.01));
		_camera->set_up(Vec3(0, 0, 1));

		_projection = new PerspectiveProjection(R90 * 0.7, 1, 1024 * 12);
		_viewport = new Viewport(_camera, _projection);
		_viewport->set_bounds(AlignedBox<2>(ZERO, manager->display_context()->size()));
		
		_shader_manager = new ShaderManager;
		_texture_manager = new TextureManager;
		_pixel_buffer_renderer = new ImageRenderer(_texture_manager);
		
		TextureParameters parameters;
		parameters.generate_mip_maps = true;
		parameters.min_filter = GL_LINEAR_MIPMAP_LINEAR;
		parameters.mag_filter = GL_LINEAR;
		parameters.target = GL_TEXTURE_2D;
		
		Ref<RendererState> renderer_state = new RendererState;
		renderer_state->viewport = _viewport;
		renderer_state->texture_manager = _texture_manager;
		renderer_state->shader_manager = _shader_manager;
		renderer_state->resource_loader = manager->resource_loader();
		_video_stream_renderer = new Renderer::VideoStreamRenderer(renderer_state);

		{
			_wireframe_renderer = new WireframeRenderer;
			
			_wireframe_program = renderer_state->load_program("Shaders/wireframe");
			_wireframe_program->set_attribute_location("position", WireframeRenderer::POSITION);
			_wireframe_program->link();
			
			auto binding = _wireframe_program->binding();
			binding.set_uniform("major_color", Vec4(1.0, 1.0, 1.0, 1.0));
		}
		
		{
			using namespace Geometry;
			
			Shared<MeshT> mesh = new MeshT;
			mesh->layout = LINES;
			Generate::grid(*mesh, 32, 2.0);
			
			_grid_mesh_buffer = new MeshBuffer<MeshT>();
			_grid_mesh_buffer->set_mesh(mesh);
			
			{
				auto binding = _grid_mesh_buffer->vertex_array().binding();
				auto attributes = binding.attach(_grid_mesh_buffer->vertex_buffer());
				attributes[POSITION] = &MeshT::VertexT::position;
			}
		}

		glClearColor(0.0, 0.0, 0.0, 1.0);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		
		{
			using namespace Text;
			
			_font = resource_loader()->load<Font>("Fonts/Monaco");
			_font->set_pixel_size(14);

			_text_buffer = new TextBuffer(_font);
			_text_renderer = new ImageRenderer(renderer_state->texture_manager);
			auto & texture_parameters = _text_renderer->texture_parameters();
			texture_parameters = TextureParameters::NEAREST;
			texture_parameters.internal_format = GL_RG;

			_text_program = renderer_state->load_program("Shaders/text");

			_text_program->set_attribute_location("position", 0);
			_text_program->set_attribute_location("mapping", 1);
			_text_program->link();

			{
				auto binding = _text_program->binding();
				binding.set_texture_unit("diffuse_texture", 0);
				binding.set_uniform("highlight_color", Vec4(IDENTITY));
			}

		}
	}
	
	void ImageSequenceScene::will_revoke_current (ISceneManager * manager)
	{
		Scene::will_revoke_current(manager);
				
		_shader_manager = NULL;
		_texture_manager = NULL;
	}
	
	bool ImageSequenceScene::event(const EventInput &input) {
		// Grab the cursor
		Ref<IContext> context = manager()->display_context();
		
		if (input.event() == EventInput::RESUME) {
			//context->set_cursor_mode(CURSOR_GRAB);
			//logger()->log(LOG_INFO, LogBuffer() << "Cursor Mode: " << context->cursor_mode());
			
			return true;
		} else {
			//context->set_cursor_mode(CURSOR_NORMAL);
			//logger()->log(LOG_INFO, LogBuffer() << "Cursor Mode: " << context->cursor_mode());
			
			return true;
		}
		
		return false;
	}
	
	bool ImageSequenceScene::resize (const ResizeInput & input)
	{
		logger()->log(LOG_INFO, LogBuffer() << "Resizing to " << input.new_size());
		
		_viewport->set_bounds(AlignedBox<2>(ZERO, input.new_size()));
		glViewport(0, 0, input.new_size()[WIDTH], input.new_size()[HEIGHT]);

		{
			auto binding = _text_program->binding();

			auto projection = orthographic_projection_matrix(AlignedBox3(ZERO, input.new_size() << 1.0));

			binding.set_uniform("display_transform", projection);
		}

		check_graphics_error();
		
		return Scene::resize(input);
	}

	static bool camera_motion(BirdsEyeCamera & camera, const MotionInput & input) {
		const Vec3 & d = input.motion();

		if (input.button_pressed_or_dragged(MouseLeftButton)) {
			RealT k = -1.0, i = number(camera.incidence().value).modulo(R360);

			if (i < 0) i += R360;

			// Reverse motion if we are upside down:
			if (camera.reverse() && i > R180 && i < R360)
				k *= -1.0;

			// Find the relative position of the mouse, if it is in the lower half,
			// reverse the rotation.
			Vec2 relative = input.bounds().relative_offset_of(input.current_position().reduce());

			//logger()->log(LOG_DEBUG, LogBuffer() << "Motion: " << d);

			// If mouse button is in lower half of view:
			if (relative[Y] <= 0.5)
				k *= -1.0;

			camera.set_azimuth(camera.azimuth() + degrees(k * d[X] * camera.multiplier()[X]));
			camera.set_incidence(camera.incidence() + degrees(d[Y] * camera.multiplier()[Y]));

			return true;
		} else if (input.key().button() == MouseScroll) {
			camera.set_distance(camera.distance() + (d[Y] * camera.multiplier()[Z]));

			return true;
		} else {
			return false;
		}
	}

	bool ImageSequenceScene::motion (const MotionInput & input) {
		if (input.button_pressed(MouseLeftButton)) {
			if (_video_stream_renderer->select_feature_point(input.current_position().reduce())) {
				return true;
			}
		}
		
		return camera_motion(*_camera, input);
	}
	
	void ImageSequenceScene::dump_tracking_points()
	{
		// Dump the tracking points:
		for (auto & frame : _video_stream_renderer->frame_cache()) {
			if (frame->selected_feature_index == (std::size_t)-1) continue;
			
			auto feature_points = frame->feature_points();
			auto offset = feature_points->offsets().at(frame->selected_feature_index);
		
			std::cerr << frame->video_frame.index << ", " << _tracking_point_sequence << ", " << offset[X] << ", " << offset[Y] << std::endl;
		}
		
		_tracking_point_sequence += 1;
	}
	
	void ImageSequenceScene::reset_tracking_points()
	{
		// Reset the tracking points (breaks the visualisation a wee bit):
		for (auto & frame : _video_stream_renderer->frame_cache()) {
			frame->selected_feature_index = (std::size_t)-1;
		}
	}
	
	static Radians<> bearing_of_global_coordinate(Vec3 coordinate)
	{
		// Project on XY plane:
		coordinate[Z] = 0;
		coordinate = coordinate.normalize();

		// Compute the rotation from north:
		Quat q = rotate<3>({0, 1, 0}, coordinate, {0, 0, 1});
		auto bearing = q.angle() * q.axis().dot({0, 0, -1});
		
		return bearing;
	}
	
	void ImageSequenceScene::evaluate_tracking_points_bearing()
	{
		using namespace Euclid::Numerics;
		
		auto & frame_cache = _video_stream_renderer->frame_cache();
		std::unordered_map<std::size_t, Distribution<>> distributions;
		
		for (auto & frame : frame_cache) {
			for (auto & pair : frame->video_frame.tracking_points) {
				auto & distribution = distributions[pair.first];
				
				// Only support 2D at the moment:
				auto global_coordinate = frame->global_coordinate_of_pixel_coordinate(pair.second.coordinate.reduce());
				
				auto bearing = R2D * bearing_of_global_coordinate(global_coordinate);
				
				if (bearing < 0) bearing += 360.0;
				
				distribution.add_sample(bearing);
			}
		}
		
		for (auto & pair : distributions) {
			auto & d = pair.second;
			
			std::cout << "Tracking Point Index " << pair.first << "(# = " << d.number_of_samples() << ")" << std::endl;
			std::cout << d.minimum() << ", " << d.maximum() << ", " << d.average() << ", " << d.standard_deviation() << ", " << d.standard_error() << std::endl;
			
			for (auto & sample : d.samples())
				std::cout << sample << ", ";
			std::cout << std::endl;
		}
	}
	
	bool ImageSequenceScene::button (const ButtonInput & input)
	{
		if (input.button_pressed('a')) {
			_video_stream_renderer->apply_feature_algorithm();
		} else if (input.button_pressed('o')) {
			Vec2u range = _video_stream_renderer->range();
			
			if (range[0] > 0)
				range[0] -= 1;
			
			_video_stream_renderer->set_range(range);
			return true;
		} else if (input.button_pressed('p')) {
			Vec2u range = _video_stream_renderer->range();
			
			if (range[0] < (_video_stream->frames().size() - 2))
				range[0] += 1;
			
			_video_stream_renderer->set_range(range);
			return true;
		} else if (input.button_pressed('d')) {
			dump_tracking_points();
			
			return true;
		} else if (input.button_pressed('r')) {
			reset_tracking_points();
			
			return true;
		} else if (input.button_pressed('c')) {
			evaluate_tracking_points_bearing();
		}
		
		return false;
	}
	
	void ImageSequenceScene::render_frame_for_time (TimeT time)
	{
		Scene::render_frame_for_time(time);
		
		/// Dequeue any button/motion/resize events and process them now (in this thread).
		manager()->process_pending_events(this);
		
		check_graphics_error();
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		_video_stream_renderer->render_frame_for_time(time, _video_stream);

		{
			auto binding = _text_program->binding();

			bool regenerated;

			StringStreamT buffer;

			{
				auto range = _video_stream_renderer->range();
				auto & first_frame = _video_stream_renderer->frame_cache().at(range[0])->video_frame;

				buffer << "Gravity: " << first_frame.gravity << std::endl;
				buffer << "Bearing: " << first_frame.bearing * R2D << std::endl;
				buffer << "Heading: " << first_frame.heading << std::endl;
				buffer << "Tilt: " << first_frame.tilt.value * R2D << std::endl;
				buffer << "Time Offset: " << first_frame.image_update->time_offset << std::endl;
				//buffer << "Tilt: " << first_image_update.tilt() * R2D << std::endl;

				auto index = this->_video_stream_renderer->selected_feature_point();
				auto & image_update = first_frame.image_update;

				buffer << "Feature Index: " << index << std::endl;

				if (first_frame.feature_points && first_frame.feature_points->offsets().size() > index[1])
				{
					auto feature = first_frame.feature_points->offsets().at(index[1]);
					auto frame_cache = _video_stream_renderer->frame_cache()[range[0]];

					auto global_coordinate = frame_cache->global_coordinate_of_pixel_coordinate(feature);

					auto bearing = bearing_of_global_coordinate(global_coordinate);

					buffer << "Feature Bearing: N+" << (bearing * R2D) << std::endl;
				}

				for (auto & pair : first_frame.tracking_points) {
					buffer << pair.first << ": " << pair.second.coordinate << std::endl;
				}

				if (image_update->image_buffer) {
					buffer << "Frame Size: " << image_update->image_buffer->size() << std::endl;
				}
				
				buffer << "Frame Index: " << range << std::endl;

				auto notes = image_update->notes;

				for (auto & note : notes)
				{
					buffer << note;
				}
			}

			_text_buffer->set_text(buffer.str());

			Ref<Image> text_image = _text_buffer->render_text(regenerated);
			
			if (regenerated) {
				_text_renderer->invalidate(text_image);
			}

			_text_renderer->render(AlignedBox2::from_origin_and_size(ZERO, text_image->size()), text_image, Vec2b(false, false), 0);
		}
		
		check_graphics_error();
	}
	
	typedef std::vector<std::string> ArgumentsT;
	
	class TransformFlowApplicationDelegate : public Object, implements IApplicationDelegate
	{
		protected:
			Ref<ILoader> _loader;
			Ref<IContext> _context;
			
			Path _data_path;
			Ref<MotionModel> _motion_model;
		
		public:
			TransformFlowApplicationDelegate(const Path & runtime_path);
			~TransformFlowApplicationDelegate();
			
			void set_data_path(const Path & data_path) { _data_path = data_path; }
			void set_motion_model(Ptr<MotionModel> motion_model) { _motion_model = motion_model; }
			
			void application_did_finish_launching (IApplication * application);
			
			void application_will_enter_background (IApplication * application);
			void application_did_enter_foreground (IApplication * application);
	};
	
	TransformFlowApplicationDelegate::TransformFlowApplicationDelegate(const Path & runtime_path)
	{
		_loader = Client::default_resource_loader(runtime_path);
		
		log_debug("Loader resource_path", _loader->resource_path());
	}
	
	TransformFlowApplicationDelegate::~TransformFlowApplicationDelegate()
	{
	}
	
	void TransformFlowApplicationDelegate::application_did_finish_launching (IApplication * application)
	{
		Ref<Dictionary> config = new Dictionary;
		_context = application->create_context(config);
		
		Ref<Thread> thread = new Events::Thread;
		
		Ref<SceneManager> scene_manager = new SceneManager(_context, thread->loop(), _loader);

		Ref<Resources::Loader> data_loader = new Resources::Loader(_data_path);
		data_loader->add_loader(new Image::Loader);

		Ref<VideoStream> video_stream = new VideoStream(data_loader, _motion_model);
		
		Ref<ImageSequenceScene> image_sequence_scene = new ImageSequenceScene;
		image_sequence_scene->set_video_stream(video_stream);
		
		scene_manager->push_scene(image_sequence_scene);
		
		_context->start();
	}
		
	void TransformFlowApplicationDelegate::application_will_enter_background (IApplication * application)
	{
		logger()->log(LOG_INFO, "Entering background...");

		_context->stop();
	}
	
	void TransformFlowApplicationDelegate::application_did_enter_foreground (IApplication * application)
	{
		logger()->log(LOG_INFO, "Entering foreground...");
		
		_context->start();
	}
}

int main (int argc, const char * argv[])
{
	using namespace TransformFlow;
	using namespace Dream::Display;
	
	Path runtime_path = Path(argv[0]).parent_path();
	
	if (argc != 3) {
		std::cerr << "Usage: " << Path(argv[0]).last_name_components().basename << " data-path motion-model" << std::endl;
		return -1;
	}
	
	log_debug("Working directory", runtime_path);
	
	Ref<TransformFlowApplicationDelegate> delegate = new TransformFlowApplicationDelegate(runtime_path);
	
	Path data_path = Path(argv[1]);
	StringT motion_model_name = argv[2];
	
	log_debug("Loading data from path", data_path);
	
	Ref<MotionModel> motion_model;
	
	if (motion_model_name == "BasicSensorMotionModel") {
		motion_model = new BasicSensorMotionModel;
	} else if (motion_model_name == "HybridMotionModel") {
		motion_model = new HybridMotionModel;
	} else {
		std::cerr << "Unsupported motion model: " << motion_model_name << std::endl;
	}
	
	delegate->set_data_path(data_path);
	delegate->set_motion_model(motion_model);
	
	Client::run(delegate);
	
    return 0;
}
