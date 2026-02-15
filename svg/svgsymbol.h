#pragma once


#include "svgstructuretypes.h"

namespace waavs {
	//===========================================
	// SVGSymbolNode
	// Notes:  An SVGSymbol can create its own local coordinate system
	// if a viewBox is specified.  We need to take into account the aspectRatio
	// as well to be totally correct.
	// As an enhancement, we'd like to take into account style information
	// such as allowing the width and height to be specified in an style
	// sheet or inline style attribute
	//===========================================
	struct SVGSymbolNode : public SVGGraphicsElement	
	{
		static void registerFactory()
		{
			registerContainerNodeByName(svgtag::tag_symbol(),
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGSymbolNode>(groot);
					node->loadFromXmlPull(iter, groot);

					return node;
				});
		}

		DocViewportState fDocVP{};		// only using; par, hasViewBox, viewBox
        bool fHasDocVP{ false };

		// Optional Reference Point
		SVGLengthValue fRefX{};
		SVGLengthValue fRefY{};
		bool fHasRefXY{ false };


		BLPoint fSymbolContentTranslation{};

		SVGSymbolNode(IAmGroot* root)
			: SVGGraphicsElement()
		{
			setIsStructural(true);
			setIsVisible(false);
		}
		


		virtual void fixupSelfStyleAttributes(IAmGroot*) override
		{
			// PreserveAspectRatio + viewBox
			loadDocViewportState(fDocVP, fAttributes);
			fHasDocVP = true;

			// refX/refY (SVG2 adds these on <symbol>; if you’re using them)
			fRefX = parseLengthAttr(getAttributeByName("refX"));
			fRefY = parseLengthAttr(getAttributeByName("refY"));
			fHasRefXY = fRefX.isSet() || fRefY.isSet();
		}

		void bindSelfToContext(IRenderSVG*, IAmGroot*) override
		{
			// No persistent “binding” needed for symbol viewport.
			// The instance viewport is provided by <use> at draw time.
		}


		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			if (!fHasDocVP)
				return;

			// Instance viewport should already be established by <use>.
			// In your current <use> pattern, this is typically {0,0,w,h}.
			const BLRect instanceVP = ctx->viewport();
			if (instanceVP.w <= 0.0 || instanceVP.h <= 0.0)
				return;

			const double dpi = groot ? groot->dpi() : 96.0;
			const BLFont* fontOpt = &ctx->getFont();

			SVGViewportState vp{};
			// For symbol: treat as "not top-level"; x/y don’t apply anyway.
			// We only care about mapping symbol viewBox -> instance viewport.
			if (!resolveViewState(instanceVP, fDocVP, /*isTopLevel*/ false, dpi, fontOpt, vp))
				return;

			// Apply mapping from symbol user space (viewBox) into instance viewport.
			ctx->applyTransform(vp.viewBoxToViewportXform);

			// Nearest viewport for children (percentage lengths inside the symbol):
			// After applying transform, symbol’s local user space is vp.fViewBox.
			ctx->setViewport(vp.fViewBox);

			// Optional: refX/refY support (only if you decide to use it now)
			// A common policy is to translate so that (refX,refY) becomes the origin.
			// Reference length for percentages: use viewBox w/h.
			if (fHasRefXY)
			{
				LengthResolveCtx cx = makeLengthCtxUser(vp.fViewBox.w, 0.0, dpi, fontOpt);
				LengthResolveCtx cy = makeLengthCtxUser(vp.fViewBox.h, 0.0, dpi, fontOpt);

				const double rx = resolveLengthOr(fRefX, cx, 0.0);
				const double ry = resolveLengthOr(fRefY, cy, 0.0);

				if (rx != 0.0 || ry != 0.0)
					ctx->translate(-rx, -ry);
			}
		}
	};

}
