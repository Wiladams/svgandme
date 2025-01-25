#pragma comment(lib, "blend2d.lib")

#include <filesystem>

#include "apphost.h"
#include "mappedfile.h"
#include "svguiapp.h"
#include "svggraphic.h"

#include "viewnavigator.h"

using namespace waavs;

// Drawing context used for drawing document
IRenderSVG gDrawingContext(nullptr);


// Reference to currently active document
SVGDocumentHandle gDoc{ nullptr };

/*
// A little database of small svg string literals
static ByteSpan getIconSpan(const ByteSpan& name) noexcept
{
	static SVGDB icondb = {
		{"goDown", R"||(
				<svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" viewBox="0 0 22 22">
					<path d="M14,11V5c0-1.105-0.895-2-2-2h-2C8.895,3,8,3.895,8,5v6H3l8,8l8-8H14z"/>
				</svg>
			)||"},

			{"goUp", R"||(
				<svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" viewBox="0 0 22 22">
					<path d="M11,3l-8,8h5v6c0,1.105,0.895,2,2,2h2c1.105,0,2-0.895,2-2v-6h5L11,3z"/>
				</svg>
			)||"},

			{"goLeft", R"||(
				<svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" viewBox="0 0 22 22">
					<path d="M17,8h-6V3l-8,8l8,8v-5h6c1.105,0,2-0.895,2-2v-2C19,8.895,18.105,8,17,8z"/>
				</svg>
			)||"},

			{"goRight", R"||(
				<svg xmlns="http://www.w3.org/2000/svg" width="100%" height="100%" viewBox="0 0 22 22">
					<path d="M5,8h6v5l8-8l-8-8v5H5c-1.105,0-2,0.895-2,2v2C3,7.105,3.895,8,5,8z"/>
				</svg>
			)||"},

			{"floppydisk", R"||(
				<svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" viewBox="0 0 22 22">
					<path d="M6,21h10v-6H6V21z M16,4h-3v3h3V4z M19,1H3C1.895,1,1,1.895,1,3v16 c0,1.105,0.895,2,2,2h1v-6c0-1.105,0.895-2,2-2h10c1.105,0,2,0.895,2,2v6h1c1.105,0,2-0.895,2-2V3C21,1.895,20.105,1,19,1z M17,8 H5V3h12V8z"/>
				</svg>
			)||"},

			{"checkerboard", R"||(
	<svg width = "100%" height = "100%" xmlns = "http://www.w3.org/2000/svg">								
	<!--Define a checkerboard pattern-->															
	<defs>																							
	<pattern id = "checkerboard" width = "20" height = "20" patternUnits = "userSpaceOnUse">		
	<rect width = "10" height = "10" fill = "lightgray" />
	<rect x = "10" width = "10" height = "10" fill = "darkgray" />									
	<rect y = "10" width = "10" height = "10" fill = "darkgray" />									
	<rect x = "10" y = "10" width = "10" height = "10" fill = "lightgray" />						
	</pattern>																					
	</defs>																						
	
	<!--Background with checkerboard pattern-->														
	<rect width = "100%" height = "100%" fill = 'url(#checkerboard)' />								
	</svg>
			)||"},
	};



	auto it = icondb.find(name);
	if (it != icondb.end())
	{
		return it->second;
	}

	// Didn't find one, so return blank
	return {};

}
*/

static const char* goDown_s = "M14,11V5c0-1.105-0.895-2-2-2h-2C8.895,3,8,3.895,8,5v6H3l8,8l8-8H14z";
static const char* goUp_s = "M11,3l-8,8h5v6c0,1.105,0.895,2,2,2h2c1.105,0,2-0.895,2-2v-6h5L11,3z";
static const char* goLeft_s = "M17,8h-6V3l-8,8l8,8v-5h6c1.105,0,2-0.895,2-2v-2C19,8.895,18.105,8,17,8z";
static const char* goRight_s = "M5,8h6v5l8-8l-8-8v5H5c-1.105,0-2,0.895-2,2v2C3,7.105,3.895,8,5,8z";

static void quickDraw(IRenderSVG *ctx)
{
	ctx->renew();
	ctx->push();
	
	// Construct a graphic
	AGraphicShape SDown11(goDown_s);
	AGraphicShape SDown12(goDown_s);
	AGraphicShape SDown13(goDown_s);


	SDown11.setFillStyle(BLVar(BLRgba32(0xffff0000)));
	SDown11.setPreserveAspectRatio("xMinYMin meet");
	SDown11.setFrame({ 0,0,32,50 });
	
	SDown12.setFillStyle(BLVar(BLRgba32(0xffff0000)));
	SDown12.setPreserveAspectRatio("xMidYMin meet");
	SDown12.setFrame({ 50,0,32,50 });

	SDown13.setFillStyle(BLVar(BLRgba32(0xffff0000)));
	SDown13.setPreserveAspectRatio("xMaxYMin meet");
	SDown13.setFrame({ 100,0,32,50 });

	SDown11.draw(ctx);
	SDown12.draw(ctx);
	SDown13.draw(ctx);

	
	//
	// Row 2
	AGraphicShape SDown21(goDown_s);
	AGraphicShape SDown22(goDown_s);
	AGraphicShape SDown23(goDown_s);
	
	SDown21.set("viewBox:0 0 22 22; portal:0 50 44 44; fill:green; preserveAspectRatio:xMinYMid meet; stroke:black");
	SDown22.set("viewBox:0 0 22 22; portal:50 50 44 44; fill:green; preserveAspectRatio:xMidYMid meet; stroke:black");
	SDown23.set("viewBox:0 0 22 22; portal:100 50 44 44; fill:green; preserveAspectRatio:xMidYMid meet; stroke:black");

	SDown21.draw(ctx);
	SDown22.draw(ctx);
	SDown23.draw(ctx);

	
	//
	// Row 3
	AGraphicShape SDown31(goDown_s);
	AGraphicShape SDown32(goDown_s);
	AGraphicShape SDown33(goDown_s);

	SDown31.setAttribute("fill", "blue");
	SDown31.setAttribute("preserveAspectRatio", "xMinYMax meet");
	SDown31.setAttribute("portal", "0, 100, 22, 44");
	SDown31.setAttribute("viewBox", "0 0 22 22");

	SDown32.setAttribute("fill", "blue");
	SDown32.setAttribute("preserveAspectRatio", "xMidYMax meet");
	SDown32.setAttribute("portal", "50, 100, 22, 44");
	SDown32.setAttribute("viewBox", "0 0 22 22");

	SDown33.setAttribute("fill", "blue");
	SDown33.setAttribute("preserveAspectRatio", "xMaxYMax meet");
	SDown33.setAttribute("portal", "100, 100, 22, 44");
	SDown33.setAttribute("viewBox", "0 0 22 22");

	SDown31.draw(ctx);
	SDown32.draw(ctx);
	SDown33.draw(ctx);

	
	ctx->flush();
	ctx->pop();

}

// docFromFilename
//
// Given a filename, parse the svg in the file, and return 
// a shared pointer to it.
//
static SVGDocumentHandle DOMFromFilename(const char* filename)
{
	auto mapped = waavs::MappedFile::create_shared(filename);

	// if the mapped file does not exist, return
	if (mapped == nullptr)
	{
		printf("File not found: %s\n", filename);
		return nullptr;
	}


	ByteSpan aspan(mapped->data(), mapped->size());
	auto aDoc = SVGFactory::createDOM(aspan, &getFontHandler());

	return aDoc;
}


// Event responders
static void onFileDrop(const FileDropEvent& fde)
{
	// assuming there's at least one file that 
	// has been dropped.
	for (int i = 0; i < fde.filenames.size(); i++)
	{
		// Create a new SVGDocument for each file
		// And create a window to display each document
		double startTime = seconds();
		gDoc = DOMFromFilename(fde.filenames[i].c_str());


		double endTime = seconds();
		printf("== fileDrop: SVGFactory::createDOM() took %f seconds\n", endTime - startTime);

		if (gDoc != nullptr)
		{
			// We have loaded the un-processed document
			// Draw into an empty context at least once to resolve references
			// and fix sizes.
			//IRenderSVG ctx(&getFontHandler());
			//ctx.setContainerFrame(BLRect(0, 0, canvasWidth, canvasHeight));
			//gDoc->draw(&ctx, gDoc.get());

			//BLRect objFr = gDoc->getBBox();
			//auto objFr = gDoc->frame();

			//resetView();



			// Set the initial viewport
			//gNavigator.setFrame({ 0, 0, (double)canvasWidth, (double)canvasHeight });
			//gNavigator.setBounds(objFr);

			//handleChange(true);

			break;
		}

	}
}

// Create a routine to respond to frameevents
static void onFrameEvent(const FrameCountEvent& fe)
{
	//printf("frameEvent: %d\n", (int)fe.frameCount);
	//printf("Actual Frame Rate: %d\n", (int)(fe.frameCount / seconds()));
	quickDraw(&gDrawingContext);


	refreshScreenNow();


	//gRecorder.saveFrame();
}


// called once before main loop is running
void setup()
{

	//printf("setup()\n");

	//setupFonts();

	frameRate(15);

	dropFiles();


	//layered();

	// set app window size and title
	createAppWindow(800, 600, "DOM Walker");

	gRecorder.reset(&appFrameBuffer()->getBlend2dImage(), "frame", 15, 0);

	// register to receive various events
	subscribe(onFileDrop);
	subscribe(onFrameEvent);
	//subscribe(onMouseEvent);
	//subscribe(onResizeEvent);
	//subscribe(onKeyboardEvent);

	// clear the buffer to white to start
	//appFrameBuffer()->setAllPixels(vec4b{ 0xFF,0xff,0xff,0x00 });
	BLContextCreateInfo ctxInfo{};
	ctxInfo.threadCount = 4;
	//ctxInfo.threadCount = 0;
	gDrawingContext.begin(appFrameBuffer()->getBlend2dImage(), &ctxInfo);
	gDrawingContext.background(BLRgba32(0xff00ffff));
	
	// Set the initial viewport
	//gViewPort.surfaceFrame({0, 0, (double)canvasWidth, (double)canvasHeight});
	//gNavigator.setFrame({ 0, 0, (double)canvasWidth, (double)canvasHeight });
	//gNavigator.subscribe(handleChange);

	// Load extension elements
	//DisplayCaptureElement::registerFactory();
	//SVGScriptElement::registerFactory();

	//onFrameEvent(FrameCountEvent{ 0 });


}
