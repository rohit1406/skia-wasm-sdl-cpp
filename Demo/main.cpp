/*
 * Copyright 2015 Google Inc.
 * Copyright 2020 Peter Verswyvelen 
 * Original file: https://github.com/google/skia/blob/master/example/SkiaSDLExample.cpp
 * This source is governed by a license that can be found in the LICENSE file.
 */

#include "pch.h"
#include "fonts.h"
#include "include/struct_mapping/struct_mapping.h"

#define S1(x) #x
#define S2(x) S1(x)
#define PRINT_LINE puts(__FILE__ " : " S2(__LINE__))

#ifndef SK_DEBUG
extern "C"
{
  // TODO: No idea why we have to define these...
  bool gCheckErrorGL = false;
  bool gLogCallsGL = false;
}
#endif

 /*
  * This application is a simple example of how to combine SDL and Skia it demonstrates:
  *   how to setup gpu rendering to the main window
  *   how to perform cpu-side rendering and draw the result to the gpu-backed screen
  *   draw simple primitives (rectangles)
  *   draw more complex primitives (star)
  */

  // If you want multisampling, uncomment the below lines and set a sample count
static const int kMsaaSampleCount = 0; //4;

// // Skia needs 8 stencil bits
static const int kStencilBits = 8;

class SdlApp
{
public:
	SdlApp();

	virtual ~SdlApp();

protected:
	void* glContext = nullptr;
	SDL_Window* window = nullptr;
	int viewWidth = 0;
	int viewHeight = 0;
};

struct Gradient {
	std::list<std::string> colors;
	std::list<std::string> offsets;
	int angle;
	std::string direction;
	std::string type;
};

struct Properties {
	int x;
	int y;
	int width;
	int height;
};

struct Shape {
	std::string type;
	Properties props;
	std::string value;
	int fontSize;
	std::string fillColor;
	std::string strokeColor;
	int strokeWidth;
	Gradient gradient;
	int letterSpacing;
	std::string fontFamily;
	std::string fontWeight;
};

struct Elements {
	std::list <Shape> elements;
};

class SkiaApp : public SdlApp {
public:
	SkiaApp();
	virtual ~SkiaApp();

	void run();
	void update();
	void initializeData();
	void drawExperiments(SkCanvas* canvas);

	static void update_callback(void* app);

	

private:
	void handle_events();

	// Storage for the user created rectangles. The last one may still be being edited.
	SkTArray<SkRect> fRects = {};

	sk_sp<GrDirectContext> grContext;
	sk_sp<SkSurface> surface;
	sk_sp<SkSurface> cpuSurface;

	sk_sp<SkImage> image;
  sk_sp<SkTypeface> typeface;

	SkPaint paint;
	SkFont font;
	float rotation = 0;

	bool fQuit = false;

	// Json processing data
	Elements elements;
	std::string jsonFileName = "assets/sample_json.json";
	std::string LBL_RECTANGLE= std::string("RECTANGLE");
	std::string LBL_TEXT= std::string("TEXT");
};

static void throw_sdl_error() {
	const char* error = SDL_GetError();
	throw std::runtime_error(std::string("SDL Error: ") + std::string(error));
	SDL_ClearError();
}

SdlApp::SdlApp()
{
	printf("EMSC:: Initializing SdlApp\n");
	uint32_t windowFlags = 0;
	printf("EMSC:: Setting window attributes\n");
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

#if defined(SK_BUILD_FOR_ANDROID) || defined(SK_BUILD_FOR_IOS)
	// For Android/iOS we need to set up for OpenGL ES and we make the window hi res & full screen
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
		SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN_DESKTOP |
		SDL_WINDOW_ALLOW_HIGHDPI;
#else
	// For all other clients we use the core profile and operate in a window
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
#endif
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, kStencilBits);

	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

	// SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	// SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, kMsaaSampleCount);

	/*
	 * In a real application you might want to initialize more subsystems
	 */
	printf("EMSC:: Initializing SDL with SDL_INIT_VIDEO | SDL_INIT_EVENTS\n");
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
		throw_sdl_error();

	// Setup window
	// This code will create a window with the half the resolution as the user's desktop.
	printf("EMSC:: Getting display mode\n");
	// Declare display mode structure to be filled in.
	SDL_DisplayMode displayMode;

	SDL_Init(SDL_INIT_VIDEO);

	// Get current display mode of all displays.
	for (int i = 0; i < SDL_GetNumVideoDisplays(); ++i) {

		int should_be_zero = SDL_GetCurrentDisplayMode(i, &displayMode);

		if (should_be_zero != 0) {
			// In case of error...
			SDL_Log("Could not get display mode for video display #%d: %s", i, SDL_GetError());
			throw_sdl_error();
		}
		else{
			// On success, print the current display mode.
			SDL_Log("Display #%d: current display mode is %dx%dpx @ %dhz.", i, displayMode.w, displayMode.h, displayMode.refresh_rate);
		}
	}
	//if (SDL_GetDesktopDisplayMode(0, &displayMode) != 0)
		//throw_sdl_error();
	printf("EMSC:: Creating Window using SDL\n");
	window = SDL_CreateWindow("SDL Window", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, displayMode.w / 2, displayMode.h / 2, windowFlags);

	if (!window)
		throw_sdl_error();

	// To go fullscreen
	// SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

	// try and setup a GL context
	printf("EMSC:: Creating a GL Context\n");
	glContext = SDL_GL_CreateContext(window);
	if (!glContext)
		throw_sdl_error();

	printf("EMSC:: Making above created GL context as current context\n");
	auto success = SDL_GL_MakeCurrent(window, glContext);
	if (success != 0)
		throw_sdl_error();
	printf("EMSC:: Getting Drawable size\n");
	SDL_GL_GetDrawableSize(window, &viewWidth, &viewHeight);
	printf("EMSC:: View size = %d x %d\n", viewWidth, viewHeight);
}

SdlApp::~SdlApp()
{
	printf("EMSC:: Cleaning GL Context\n");
	if (glContext) {
		SDL_GL_DeleteContext(glContext);
	}
	printf("EMSC:: Destroying window\n");
	//Destroy window
	SDL_DestroyWindow(window);
	printf("EMSC:: Quitting SDL subsystems\n");
	//Quit SDL subsystems
	SDL_Quit();
	printf("EMSC:: Cleaning Done in SdlApp\n");
}

void SkiaApp::run()
{
	while (!fQuit)
	{
		update();
	}
}

void SkiaApp::update()
{
	auto* canvas = surface->getCanvas();
	const char* helpMessage = "Click and drag to create rects.  Press esc to quit.";

	canvas->clear(SK_ColorWHITE);
	handle_events();


	// draw experiments
	drawExperiments(canvas);

	paint.setColor(SK_ColorBLACK);
	canvas->drawString(helpMessage, 100.0f, 100.0f, font, paint);

	SkRandom rand;
	for (int i = 0; i < fRects.count(); i++) {
		paint.setColor(rand.nextU() | 0x44808080);
		canvas->drawRect(fRects[i], paint);
	}

	// draw offscreen canvas
	canvas->save();
	canvas->translate(float(viewWidth) / 2, float(viewHeight) / 2);
	canvas->rotate(rotation++);
	canvas->drawImage(image, -50.0f, -50.0f);
	canvas->restore();

	canvas->flush();
	SDL_GL_SwapWindow(window);
}

void SkiaApp::drawExperiments(SkCanvas* canvas) {
	canvas->save();
	
	canvas->translate(SkIntToScalar(128), SkIntToScalar(128));
	SkRect rect = SkRect::MakeXYWH(-90.5f, -90.5f, 181.0f, 181.0f);
	SkPaint paint;
	paint.setColor(SK_ColorBLUE);
	canvas->drawRect(rect, paint);

	SkPath path;
	path.cubicTo(768, 0, -512, 256, 256, 256);
	paint.setColor(SK_ColorGREEN);
	canvas->drawPath(path, paint);

	// rendering data from file
	std::list <Shape> shapes = elements.elements;
	for (Shape shape : shapes) {
		if (LBL_RECTANGLE.compare(shape.type) == 0) {
			SkRect rectFromFile = SkRect::MakeXYWH(shape.props.x, shape.props.y, shape.props.width, shape.props.height);
			paint.setColor(SK_ColorYELLOW);
			canvas->drawRect(rectFromFile, paint);
		}else if (LBL_TEXT.compare(shape.type) == 0) {
			paint.setColor(SK_ColorBLACK);
			canvas->drawString(shape.value.c_str(), shape.props.x, shape.props.y, font, paint);
		}
	}

	canvas->restore();
}

void SkiaApp::update_callback(void* app)
{
	static_cast<SkiaApp*>(app)->update();
}

void SkiaApp::handle_events() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_MOUSEMOTION:
			if (event.motion.state == SDL_PRESSED) {
				SkRect& rect = fRects.back();
				rect.fRight = static_cast<SkScalar>(event.motion.x);
				rect.fBottom = static_cast<SkScalar>(event.motion.y);
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (event.button.state == SDL_PRESSED) {
				fRects.push_back() = SkRect::MakeLTRB(SkIntToScalar(event.button.x),
					SkIntToScalar(event.button.y),
					SkIntToScalar(event.button.x),
					SkIntToScalar(event.button.y));
			}
			break;
		case SDL_KEYDOWN: {
			SDL_Keycode key = event.key.keysym.sym;
			if (key == SDLK_ESCAPE) {
				fQuit = true;
			}
			break;
		}
		case SDL_QUIT:
			fQuit = true;
			break;
		default:
			break;
		}
	}
}

// Creates a star type shape using a SkPath
static SkPath create_star() {
	static const int kNumPoints = 5;
	SkPath concavePath;
	SkPoint points[kNumPoints] = { {0, SkIntToScalar(-50)} };
	SkMatrix rot;
	rot.setRotate(SkIntToScalar(360) / kNumPoints);
	for (int i = 1; i < kNumPoints; ++i) {
		rot.mapPoints(points + i, points + i - 1, 1);
	}
	concavePath.moveTo(points[0]);
	for (int i = 0; i < kNumPoints; ++i) {
		concavePath.lineTo(points[(2 * i) % kNumPoints]);
	}
	concavePath.setFillType(SkPathFillType::kEvenOdd);
	assert(!concavePath.isConvex());
	concavePath.close();
	return concavePath;
}

void SkiaApp::initializeData() {
	printf("EMSC:: Initializing Data\n");
	std::istringstream json_data(R"json(
{"elements": [
	{
			"type": "RECTANGLE",
			"props": {
					"x": 25,
					"y": 100,
					"width": 200,
					"height": 200
			},
			"fillColor": "none",
			"strokeColor": "black",
			"strokeWidth": 2,
			"gradient": {
                "colors": ["#9B0F02", "#FFFFFF"],
                "offsets": ["30%", "90%"],
                "angle": -1,
                "direction": "",
                "type": "linear"
      }
	},
	{
			"type": "TEXT",
			"props": {
					"x": 25,
					"y": 25
			},
			"value": "World is beautiful!",
			"fontSize": 25,
			"fillColor": "#ff00ff",
			"strokeColor": "none",
			"strokeWidth": 0,
			"gradient": null,
			"letterSpacing": 1,
			"fontFamily": "Lato",
			"fontWeight": "Bold"
	}
]
}
)json");

	std::string jsonDataFromFile;
	std::ifstream is(jsonFileName, std::ios::binary);
	printf("EMSC:: Reading data from file %s\n", jsonFileName.c_str());
	if (is) {
		printf("EMSC:: file %s is present so reading data from it\n", jsonFileName.c_str());
		// get length of file:
		is.seekg(0, std::ios::end);
		long length = is.tellg();
		is.seekg(0, std::ios::beg);
		// allocate memory:
		char* buffer = new char[length];
		// read data as a block:
		is.read(buffer, length);
		// create string stream of memory contents
		//std::istringstream iss(std::string(buffer));
		jsonDataFromFile = std::string(buffer);
		std::cout << buffer;
		// delete temporary buffer
		delete[] buffer;
		// close filestream
		is.close();
	}
	else {
		printf("EMSC:: file %s is not present so reading hardcoded sample data from code\n", jsonFileName.c_str());
		jsonDataFromFile = json_data.str();
	}

	// Mapping JSON root
	printf("EMSC:: Mapping Json properties to Cpp structs\n");
	struct_mapping::reg(&Elements::elements, "elements");
	// Mapping Shapes properties
	printf("EMSC:: Mapping Shapes properties\n");
	struct_mapping::reg(&Shape::type, "type");
	struct_mapping::reg(&Shape::value, "value");
	struct_mapping::reg(&Shape::fontSize, "fontSize");
	struct_mapping::reg(&Shape::fillColor, "fillColor");
	struct_mapping::reg(&Shape::strokeColor, "strokeColor");
	struct_mapping::reg(&Shape::strokeWidth, "strokeWidth");
	struct_mapping::reg(&Shape::letterSpacing, "letterSpacing");
	struct_mapping::reg(&Shape::fontFamily, "fontFamily");
	struct_mapping::reg(&Shape::fontWeight, "fontWeight");
	// Mapping objects in Shape struct
	struct_mapping::reg(&Shape::props, "props");
	struct_mapping::reg(&Shape::gradient, "gradient");

	// Mapping Properties properties
	printf("EMSC:: Mapping Properties properties\n");
	struct_mapping::reg(&Properties::x, "x");
	struct_mapping::reg(&Properties::y, "y");
	struct_mapping::reg(&Properties::width, "width");
	struct_mapping::reg(&Properties::height, "height");

	// Mapping Gradient properties
	printf("EMSC:: Mapping Gradient properties\n");
	struct_mapping::reg(&Gradient::angle, "angle");
	struct_mapping::reg(&Gradient::direction, "direction");
	struct_mapping::reg(&Gradient::type, "type");
	// Mapping collections in Gradient struct
	struct_mapping::reg(&Gradient::colors, "colors");
	struct_mapping::reg(&Gradient::offsets, "offsets");

	printf("EMSC:: parsing json data struct\n");
	std::istringstream streamjsonDataFromFile(jsonDataFromFile);
	struct_mapping::map_json_to_struct(elements, streamjsonDataFromFile);
	printf("EMSC:: parsing json data struct finished\n");
	printf("EMSC:: Reading data from json - elements size is %lu\n", elements.elements.size());
	printf("EMSC:: Data initialization completed\n");
}
SkiaApp::SkiaApp()
{
	initializeData();
	printf("EMSC:: Setting viewport\n");
  glViewport(0, 0, viewWidth, viewHeight);
	glClearColor(1, 1, 1, 1);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	printf("EMSC:: Cleared color, stencil, and depth and stencil buffer bits in SkiaApp initialization\n");
	// setup GrContext
	printf("EMSC:: Getting GrGLMakeNativeInterface for Setting up GrContext\n");
	const auto interface = GrGLMakeNativeInterface();
  assert(interface);

	// setup contexts
  printf("EMSC:: Setting up GrContext\n");
	grContext = GrDirectContext::MakeGL(interface);
	assert(grContext);

  // Wrap the frame buffer object attached to the screen in a Skia render target so Skia can
	// render to it
	printf("EMSC:: Wrapping up the frame buffer object attached to the screen in a Skia render target so Skia can render to it\n");
	GrGLint buffer;
	GR_GL_GetIntegerv(interface.get(), GR_GL_FRAMEBUFFER_BINDING, &buffer);
	GrGLFramebufferInfo info;
	info.fFBOID = (GrGLuint)buffer;

	int contextType;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &contextType);

  SkColorType colorType;

#ifdef SK_BUILD_FOR_WASM
    info.fFormat = GL_RGBA8;
    colorType = kRGBA_8888_SkColorType;
#else
	//SkDebugf("%s", SDL_GetPixelFormatName(windowFormat));
	// TODO: the windowFormat is never any of these?
	uint32_t windowFormat = SDL_GetWindowPixelFormat(window);
	if (SDL_PIXELFORMAT_RGBA8888 == windowFormat) {
		info.fFormat = GR_GL_RGBA8;
		colorType = kRGBA_8888_SkColorType;
	}
	else {
		colorType = kBGRA_8888_SkColorType;
		if (SDL_GL_CONTEXT_PROFILE_ES == contextType) {
			info.fFormat = GR_GL_BGRA8;
		}
		else {
			// We assume the internal format is RGBA8 on desktop GL
			info.fFormat = GR_GL_RGBA8;
		}
	}
#endif

	printf("EMSC:: Creating GrBackendRenderTarget\n");
  GrBackendRenderTarget target(viewWidth, viewHeight, kMsaaSampleCount, kStencilBits, info);

	// setup SkSurface
	printf("EMSC:: Setting up SkSurface\n");
	// To use distance field text, use commented out SkSurfaceProps instead
	// SkSurfaceProps props(SkSurfaceProps::kUseDeviceIndependentFonts_Flag,
	// 	SkSurfaceProps::kLegacyFontHost_InitType);
	SkSurfaceProps props(SkSurfaceProps::kLegacyFontHost_InitType);
	printf("EMSC:: Creating SkSurface from MakeFromBackendRenderTarget\n");
	surface = SkSurface::MakeFromBackendRenderTarget(grContext.get(), target,
		kBottomLeft_GrSurfaceOrigin,
		colorType, nullptr, &props);
  assert(surface);

  printf("EMSC:: Getting canvas from SkSurface\n");
  auto* canvas = surface->getCanvas();
  assert(canvas);
	// canvas->scale((float)dw / displayMode.w, (float)dh / displayMode.h);

	paint.setAntiAlias(true);

	// create a surface for CPU rasterization
	printf("EMSC:: create a surface for CPU rasterization\n");
	cpuSurface = SkSurface::MakeRaster(canvas->imageInfo());
  assert(cpuSurface);

	auto* offscreen = cpuSurface->getCanvas();
  assert(offscreen);

	offscreen->save();
	offscreen->translate(50.0f, 50.0f);
	offscreen->drawPath(create_star(), paint);
	offscreen->restore();

  image = cpuSurface->makeImageSnapshot();
  assert(image);

  printf("EMSC:: Creating typeface for font rendering\n");
  typeface = SkTypeface::MakeFromData(
      SkData::MakeWithoutCopy(dataKarlaRegular, sizeof(dataKarlaRegular)));

  font.setTypeface(typeface);
	font.setSize(24);
	printf("EMSC:: App initialization is completed\n");
}

SkiaApp::~SkiaApp()
{
	printf("EMSC:: Cleaning Done in SkiaApp\n");
}

#if defined(SK_BUILD_FOR_ANDROID)
int SDL_main(int argc, char** argv) {
#else
extern "C" int __cdecl main(int argc, char** argv) {
#endif

#ifdef SK_BUILD_FOR_WASM
  // EmscriptenFullscreenStrategy strategy;
  // strategy.scaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF;
  // strategy.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF;
  // strategy.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT;
  // //emscripten_request_fullscreen_strategy("canvas", 1, &strategy);
  // emscripten_enter_soft_fullscreen("canvas", &strategy);
	
	SkiaApp app;
	emscripten_set_main_loop_arg(SkiaApp::update_callback, &app, 0, true);
#else

	SkiaApp app;
	app.run();
#endif

	return 0;
}

extern "C" {

	int int_sqrt(int x) {
		return sqrt(x);
	}

}

void EMSCRIPTEN_KEEPALIVE clean_stuff() {
	// Clean up the mess...
	// You should use EMSCRIPTEN_KEEPALIVE or
	// add it to EXPORTED_FUNCTIONS in emcc compilation options
	// to make it callable in JS side.
	printf(">>>>>>>>>>>>>>>>>clean_stuff called\n");
}