#pragma once


#include "svgcacheddocument.h"
#include "svgnavigator.h"
#include "svgicons.h"


namespace waavs {

	struct SVGBrowsingView : public SVGCachedDocument, public Topic<bool>
	{
		SVGNavigator fNavigator{};

		SVGDocument checkerboardDoc{ nullptr };
		
		bool fAnimate{ false };
		bool fPerformTransform{ true };
		bool fUseCheckerBackground{ true };
		
		SVGBrowsingView(const BLRect &aframe)
			:SVGCachedDocument(aframe)
		{
			checkerboardDoc.resetFromSpan(getIconSpan("checkerboard"), nullptr, aframe.w, aframe.h, 96);

			fNavigator.surfaceFrame(BLRect(0, 0, aframe.w, aframe.h));
			fNavigator.sceneFrame(BLRect(0, 0, aframe.w, aframe.h));
			fNavigator.subscribe([this](const bool& e) {this->handleChange(); });

		}

		bool contains(const BLPoint& pt) const
		{
			return containsRect(uiFrame(), pt);
		}
		
		void onDocumentLoad() override
		{
			fNavigator.sceneFrame(fDocument->frame());
			sceneToSurfaceTransform(fNavigator.sceneToSurfaceTransform());

		}
		


		
		void drawBackgroundIntoCache(IRenderSVG& ctx) override
		{
			if (fUseCheckerBackground)
			{
				checkerboardDoc.draw(&ctx, &checkerboardDoc);
			}
			else {
				ctx.background(BLRgba32(0xffffffff));
			}
		}

		void handleChange()
		{
			sceneToSurfaceTransform(fNavigator.sceneToSurfaceTransform());
			
			//drawIntoCache();
			needsRedraw(true);
			notify(true);
		}

		void onMouseEvent(const MouseEvent& e)
		{
			MouseEvent le = e;
			le.x -= static_cast<float>(fUIFrame.x);
			le.y -= static_cast<float>(fUIFrame.y);
			fNavigator.onMouseEvent(le);
		}

		void onKeyboardEvent(const KeyboardEvent& ke)
		{
			//printf("SVGViewer::onKeyboardEvent: %d\n", ke.key);
			if (ke.activity == KEYRELEASED)
			{
				switch (ke.keyCode)
				{
				case 'A':
					fAnimate = !fAnimate;
					needsRedraw(true);
					notify(true);
					break;

				case 'C':
					fUseCheckerBackground = !fUseCheckerBackground;
					needsRedraw(true);
					notify(true);
					break;

				case 'T':
					fPerformTransform = !fPerformTransform;
					needsRedraw(true);
					notify(true);
					break;
				}
			}
		}
	};
}
