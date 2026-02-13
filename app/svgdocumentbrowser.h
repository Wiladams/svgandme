#pragma once


#include "svgcacheddocument.h"
#include "viewnavigator.h"
#include "svgicons.h"


namespace waavs {

	struct SVGBrowsingView : public SVGCachedDocument, public Topic<bool>
	{
        // The default number of threads to use for drawing
        static constexpr int gNumThreads = 4;

		ViewNavigator fNavigator{};

		SVGDocument checkerboardDoc{};
		
		bool fAnimate{ false };
		bool fPerformTransform{ true };
		bool fUseCheckerBackground{ true };
		
		SVGBrowsingView(const BLRect &aframe)
			:SVGCachedDocument(aframe, gNumThreads)
		{
            auto checkerspan = getIconSpan("checkerboard");
			checkerboardDoc.resetFromSpan(checkerspan, aframe.w, aframe.h, 96);

			fNavigator.setFrame(BLRect(0, 0, aframe.w, aframe.h));
			fNavigator.setBounds(BLRect(0, 0, aframe.w, aframe.h));
			fNavigator.subscribe([this](const bool& e) {this->handleChange(); });

		}
		
		void onDocumentLoad() override
		{
			fNavigator.resetNavigator();	// Reset the view
			auto & aframe = frame();
			// Set this up again so we start in a know viewing state
			fNavigator.setFrame(BLRect(0, 0, aframe.w, aframe.h));
			fNavigator.setBounds(BLRect(0, 0, aframe.w, aframe.h));

			auto dbox = fDocument->objectBoundingBox();
			fNavigator.setBounds(dbox);
			
			const BLMatrix2D & tform = fNavigator.sceneToSurfaceTransform();
			setSceneToSurfaceTransform(tform);
		}
		


		
		void drawBackground(IRenderSVG *ctx) override
		{
			if (fUseCheckerBackground)
			{
				checkerboardDoc.draw(ctx, &checkerboardDoc);
			}
			else {
				ctx->background(BLRgba32(0xffffffff));
			}
		}

		void drawForeground(IRenderSVG* ctx) override
		{
			ctx->strokeWidth(4);
			ctx->stroke(BLRgba32(0xffA000A0));
			BLPath apath;
			apath.addRect(0, 0, frame().w, frame().h);
			ctx->strokeShape(apath);

		}

		void handleChange()
		{
			//setBounds(fNavigator.bounds());
			setSceneToSurfaceTransform(fNavigator.sceneToSurfaceTransform());

			setNeedsRedraw(true);
			notify(true);
		}

		void onMouseEvent(const MouseEvent& e)
		{
			MouseEvent le = e;
			le.x -= static_cast<float>(frame().x);
			le.y -= static_cast<float>(frame().y);
			fNavigator.onMouseEvent(le);

			// BUGBUG - The navigation can indicate whether there was a change
			// right here, and we can set needsredraw, and notify
			// which can remove the need for handleChange()
		}

		void onKeyboardEvent(const KeyboardEvent& ke)
		{
			//printf("SVGViewer::onKeyboardEvent: %d\n", ke.key);
			if (ke.activity == KEYRELEASED)
			{
				switch (ke.keyCode)
				{
					// Turn on animation
					case 'A':
						fAnimate = !fAnimate;
						setNeedsRedraw(true);
						notify(true);
					break;

					// Display checkboard background
					case 'C':
						fUseCheckerBackground = !fUseCheckerBackground;
						setNeedsRedraw(true);
						notify(true);
					break;

					// Allow for mouse driven transformation
					case 'T':
						fPerformTransform = !fPerformTransform;
						setNeedsRedraw(true);
						notify(true);
					break;
				}
			}
		}
	};
}
