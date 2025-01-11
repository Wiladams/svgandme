#pragma once


#include "svgcacheddocument.h"
#include "viewnavigator.h"
#include "svgicons.h"


namespace waavs {

	struct SVGBrowsingView : public SVGCachedDocument, public Topic<bool>
	{
		ViewNavigator fNavigator{};

		SVGDocument checkerboardDoc{ nullptr };
		
		bool fAnimate{ false };
		bool fPerformTransform{ true };
		bool fUseCheckerBackground{ false };
		
		SVGBrowsingView(const BLRect &aframe)
			:SVGCachedDocument(aframe)
		{
			checkerboardDoc.resetFromSpan(getIconSpan("checkerboard"), nullptr, aframe.w, aframe.h, 96);

			fNavigator.setFrame(BLRect(0, 0, aframe.w, aframe.h));
			fNavigator.setBounds(BLRect(0, 0, aframe.w, aframe.h));
			fNavigator.subscribe([this](const bool& e) {this->handleChange(); });

		}
		
		void onDocumentLoad() override
		{
			fNavigator.setBounds(fDocument->getBBox());
			setBounds(fDocument->getBBox());
			
			setSceneToSurfaceTransform(fNavigator.sceneToSurfaceTransform());
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
			ctx->strokeRect(BLRect( 0,0,frame().w, frame().h ), BLRgba32(0xffA0A0A0));

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
